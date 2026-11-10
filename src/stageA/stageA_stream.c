/* Stage A streaming example using only the course trade helper API.
 * Usage: stageA_stream --file CSV --short N --long N [--export-signals PATH].
 * Exit codes: 0 success, 2 invalid input, 6 input/output failure.
 */
#include "cli.h"
#include "exit_codes.h"
#include "fixed_point.h"
#include "platform.h"
#include "result.h"
#include "trade_io.h"
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_WINDOW 100000
#ifndef DEFAULT_SHORT_WINDOW
#define DEFAULT_SHORT_WINDOW 30 /* Group 05 default; Make may override it. */
#endif
#ifndef DEFAULT_LONG_WINDOW
#define DEFAULT_LONG_WINDOW 60 /* Group 05 default; Make may override it. */
#endif
/* Write the selected plot slice after computation so disk I/O is untimed. */
static int write_slice(const char *path, const double *price, const double *short_sma,
                       const double *long_sma, const int *signal, const uint64_t *index,
                       size_t length, size_t first)
{
    if (path == NULL || length == 0) return 1;
    FILE *output = fopen(path, "wb");
    if (output == NULL) return 0;
    int ok = 1;
    for (size_t row = 0; row < length; ++row) {
        size_t at = (first + row) % length;
        if (fprintf(output, "%d %.2f %.4f %.4f %d\n", (int)index[at], price[at],
                    short_sma[at], long_sma[at], signal[at]) < 0) { ok = 0; break; }
    }
    if (fclose(output) != 0) ok = 0;
    return ok;
}

/* Parse the shared CLI, stream the helper records, and calculate indicators. */
int main(int argc, char **argv)
{
    const char *path = NULL;
    const char *export_path = NULL;
    int short_window = DEFAULT_SHORT_WINDOW;
    int long_window = DEFAULT_LONG_WINDOW;
    size_t slice_len = 2000;
    double *prices = NULL;
    int64_t *short_fixed = NULL, *long_fixed = NULL;
    double *slice_price = NULL, *slice_short = NULL, *slice_long = NULL;
    uint64_t *slice_index = NULL;
    int *slice_signal = NULL;
    double vwap_num = 0.0, vwap_den = 0.0;
    int previous_fixed_sign = 0, fixed_ready = 0, bar_active = 0;
    int64_t short_sum = 0, long_sum = 0;
    long long current_second = 0;
    uint64_t trades = 0, bars = 0, buys = 0, sells = 0;
    uint64_t hash = UINT64_C(14695981039346656037);
    double open = 0.0, high = 0.0, low = 0.0, close = 0.0;
    uint64_t process_start;
    uint64_t process_ns;
    size_t slice_rows = 0, slice_next = 0;
    uint64_t slice_buys = 0, slice_sells = 0;
    int slice_selected = 0;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s --file CSV [--short N] [--long N] [--export-signals PATH] [--slice-len N]\n", argv[0]);
            return EXIT_OK;
        }
        if ((strcmp(argv[i], "--file") == 0 || strcmp(argv[i], "--short") == 0 ||
             strcmp(argv[i], "--long") == 0 || strcmp(argv[i], "--export-signals") == 0 ||
             strcmp(argv[i], "--slice-len") == 0) && i + 1 < argc) {
            const char *key = argv[i++];
            const char *value = argv[i];
            if (strcmp(key, "--file") == 0) path = value;
            else if (strcmp(key, "--export-signals") == 0) export_path = value;
            else if (strcmp(key, "--slice-len") == 0) {
                int parsed;
                if (!cli_parse_positive_int(value, 100000, &parsed)) return EXIT_BAD_INPUT;
                slice_len = (size_t)parsed;
            }
            else if (strcmp(key, "--short") == 0) {
                if (!cli_parse_positive_int(value, MAX_WINDOW, &short_window)) return EXIT_BAD_INPUT;
            } else if (!cli_parse_positive_int(value, MAX_WINDOW, &long_window)) return EXIT_BAD_INPUT;
        } else {
            fprintf(stderr, "Invalid argument: %s\n", argv[i]);
            return EXIT_BAD_INPUT;
        }
    }
    if (path == NULL || short_window > long_window) {
        fprintf(stderr, "A file and windows with 0 < short <= long are required.\n");
        return EXIT_BAD_INPUT;
    }
    prices = (double *)malloc((size_t)long_window * sizeof *prices);
    short_fixed = (int64_t *)calloc((size_t)short_window, sizeof *short_fixed);
    long_fixed = (int64_t *)calloc((size_t)long_window, sizeof *long_fixed);
    if (prices == NULL || short_fixed == NULL || long_fixed == NULL) {
        fprintf(stderr, "Unable to allocate SMA rings.\n");
        free(prices); free(short_fixed); free(long_fixed); return EXIT_OUT_OF_RANGE;
    }
    if (!open_trades(path)) { fprintf(stderr, "Unable to open input: %s\n", path); free(prices); free(short_fixed); free(long_fixed); return EXIT_IO_ERROR; }
    if (export_path != NULL) {
        slice_price = (double *)malloc(slice_len * sizeof *slice_price);
        slice_short = (double *)malloc(slice_len * sizeof *slice_short);
        slice_long = (double *)malloc(slice_len * sizeof *slice_long);
        slice_signal = (int *)malloc(slice_len * sizeof *slice_signal);
        slice_index = (uint64_t *)malloc(slice_len * sizeof *slice_index);
        if (slice_price == NULL || slice_short == NULL || slice_long == NULL ||
            slice_signal == NULL || slice_index == NULL) {
            fprintf(stderr, "Unable to allocate signal slice.\n");
            close_trades(); free(prices); free(short_fixed); free(long_fixed); free(slice_price); free(slice_short); free(slice_long);
            free(slice_signal); free(slice_index); return EXIT_OUT_OF_RANGE;
        }
    }
    printf("Stage A stream\nfile=%s short=%d long=%d\n", path, short_window, long_window);
    process_start = plat_now_ns();
    while (next_trade()) {
        double price = trade_price(), qty = trade_qty();
        double short_value = 0.0, long_value = 0.0;
        int signal = 0;
        size_t slot = (size_t)(trades % (uint64_t)long_window);
        prices[slot] = price;
        int64_t fixed_price = (int64_t)llround(price * (double)PRICE_SCALE);
        size_t short_slot = (size_t)(trades % (uint64_t)short_window);
        if (trades >= (uint64_t)short_window) short_sum -= short_fixed[short_slot];
        short_fixed[short_slot] = fixed_price; short_sum += fixed_price;
        if (trades >= (uint64_t)long_window) long_sum -= long_fixed[slot];
        long_fixed[slot] = fixed_price; long_sum += fixed_price;
        vwap_num += price * qty;
        vwap_den += qty;
        if (trades + 1 >= (uint64_t)short_window) {
            for (int j = 0; j < short_window; ++j) {
                size_t at = (size_t)((trades + (uint64_t)long_window - (uint64_t)short_window + 1U + (uint64_t)j) % (uint64_t)long_window);
                short_value += prices[at];
            }
            short_value /= (double)short_window;
        }
        if (trades + 1 >= (uint64_t)long_window) {
            for (int j = 0; j < long_window; ++j) {
                size_t at = (size_t)((trades + 1U + (uint64_t)j) % (uint64_t)long_window);
                long_value += prices[at];
            }
            long_value /= (double)long_window;
            int sign = fp_compare_products((uint64_t)short_sum, (uint64_t)long_window,
                                           (uint64_t)long_sum, (uint64_t)short_window);
            if (fixed_ready && previous_fixed_sign <= 0 && sign > 0) signal = 1;
            else if (fixed_ready && previous_fixed_sign >= 0 && sign < 0) signal = -1;
            previous_fixed_sign = sign; fixed_ready = 1;
            if (signal > 0) { ++buys; hash = result_hash_signal(hash, trades + 1U, 1); }
            if (signal < 0) { ++sells; hash = result_hash_signal(hash, trades + 1U, -1); }
            if (export_path != NULL && !slice_selected && trades + 1U > (uint64_t)long_window) {
                size_t at = slice_next;
                if (slice_rows == slice_len) {
                    if (slice_signal[at] > 0) --slice_buys;
                    if (slice_signal[at] < 0) --slice_sells;
                } else { ++slice_rows; }
                slice_price[at] = price; slice_short[at] = short_value; slice_long[at] = long_value;
                slice_signal[at] = signal; slice_index[at] = trades + 1U;
                if (signal > 0) ++slice_buys;
                if (signal < 0) ++slice_sells;
                slice_next = (slice_next + 1U) % slice_len;
                if (slice_rows == slice_len && slice_buys > 0 && slice_sells > 0) slice_selected = 1;
            }
        }
        long long second = trade_time() / 1000000LL;
        if (!bar_active) { current_second = second; open = high = low = close = price; bar_active = 1; }
        else if (second != current_second) { ++bars; current_second = second; open = high = low = close = price; }
        else { if (price > high) high = price; if (price < low) low = price; close = price; }
        ++trades;
    }
    process_ns = plat_now_ns() - process_start;
    if (bar_active) ++bars;
    (void)open; (void)high; (void)low; (void)close;
    close_trades();
    if (export_path != NULL && slice_selected &&
        !write_slice(export_path, slice_price, slice_short, slice_long, slice_signal,
                     slice_index, slice_len, slice_next)) {
        fprintf(stderr, "Signal export write failed.\n");
        free(prices); free(short_fixed); free(long_fixed); free(slice_price); free(slice_short); free(slice_long);
        free(slice_signal); free(slice_index); return EXIT_IO_ERROR;
    }
    free(prices); free(short_fixed); free(long_fixed);
    free(slice_price); free(slice_short); free(slice_long); free(slice_signal); free(slice_index);
    printf("RESULT version=stageA file=%s trades=%llu short=%d long=%d vwap=%.6f bars=%llu buys=%llu sells=%llu signal_hash=%016llx load_ns=0 process_ns=%llu total_ns=%llu peak_mem_kb=%llu\n",
           path, (unsigned long long)trades, short_window, long_window,
           vwap_den == 0.0 ? 0.0 : vwap_num / vwap_den, (unsigned long long)bars,
           (unsigned long long)buys, (unsigned long long)sells, (unsigned long long)hash,
           (unsigned long long)process_ns, (unsigned long long)process_ns,
           (unsigned long long)plat_peak_memory_kb());
    puts("PASS");
    return EXIT_OK;
}
