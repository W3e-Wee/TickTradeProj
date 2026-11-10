/* Stretch v9: preload timestamps, scaled prices and quantities into columns. */
#include "fast_parser.h"
#include "fixed_point.h"
#include "platform.h"
#include "result.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef DEFAULT_SHORT_WINDOW
#define DEFAULT_SHORT_WINDOW 30 /* Group 05 default; Make may override it. */
#endif
#ifndef DEFAULT_LONG_WINDOW
#define DEFAULT_LONG_WINDOW 60 /* Group 05 default; Make may override it. */
#endif
#ifndef ENGINE_BUILD_FLAGS
#define ENGINE_BUILD_FLAGS "Makefile defaults"
#endif

typedef struct {
    long long *ts;
    int64_t *price;
    double *qty;
    size_t count;
    size_t capacity;
} SoaTrades;

/* Grow the three parallel columns together before processing starts. */
static int soa_grow(SoaTrades *rows)
{
    size_t next = rows->capacity == 0 ? 4096 : rows->capacity * 2;
    if (next < rows->capacity || next > SIZE_MAX / sizeof *rows->ts) return 0;
    long long *ts = (long long *)realloc(rows->ts, next * sizeof *ts);
    if (ts == NULL) return 0;
    rows->ts = ts;
    int64_t *price = (int64_t *)realloc(rows->price, next * sizeof *price);
    if (price == NULL) return 0;
    rows->price = price;
    double *qty = (double *)realloc(rows->qty, next * sizeof *qty);
    if (qty == NULL) return 0;
    rows->qty = qty;
    rows->capacity = next;
    return 1;
}

/* Release every SoA column, including partially grown arrays. */
static void soa_release(SoaTrades *rows)
{
    free(rows->ts);
    free(rows->price);
    free(rows->qty);
    memset(rows, 0, sizeof *rows);
}

/* Convert the parser's decimal double to the shared exact decision scale. */
static int soa_scale_price(double price, int64_t *scaled)
{
    double value;
    if (scaled == NULL || !isfinite(price) || price <= 0.0) return 0;
    value = price * (double)PRICE_SCALE;
    if (!isfinite(value) || value < 0.5 || value >= (double)INT64_MAX) return 0;
    *scaled = (int64_t)llround(value);
    return 1;
}

/* Load and process the SoA engine, returning common Stage A result fields. */
int soa_execute(const char *path, int short_window, int long_window, RunResult *result)
{
    SoaTrades rows = {0};
    FILE *file;
    int status = 1;
    uint64_t start = plat_now_ns();
    uint64_t bars = 0, buys = 0, sells = 0;
    uint64_t hash = UINT64_C(14695981039346656037);
    double vwap_num = 0.0, qty_sum = 0.0;
    int64_t short_sum = 0, long_sum = 0;
    int previous_sign = 0, previous_ready = 0;

    if (path == NULL || result == NULL || short_window <= 0 ||
        long_window < short_window || long_window > 1000000) return 0;
    file = fopen(path, "rb");
    if (file == NULL) return 0;
    for (;;) {
        long long ts;
        double price, qty;
        int side;
        int64_t scaled_price;
        status = fast_read_trade(file, &ts, &price, &qty, &side);
        if (status <= 0) break;
        if (!soa_scale_price(price, &scaled_price)) { status = -1; break; }
        if (rows.count == rows.capacity && !soa_grow(&rows)) { status = -1; break; }
        rows.ts[rows.count] = ts;
        rows.price[rows.count] = scaled_price;
        rows.qty[rows.count] = qty;
        ++rows.count;
        (void)side;
    }
    int read_failed = status < 0 || ferror(file);
    int close_failed = fclose(file) != 0;
    uint64_t load_ns = plat_now_ns() - start;
    if (read_failed || close_failed || rows.count == 0) { soa_release(&rows); return 0; }

    int64_t *ring = (int64_t *)calloc((size_t)long_window, sizeof *ring);
    if (ring == NULL) { soa_release(&rows); return 0; }
    uint64_t process_start = plat_now_ns();
    long long active_second = -1;
    for (size_t i = 0; i < rows.count; ++i) {
        size_t long_slot = i % (size_t)long_window;
        int64_t price = rows.price[i];
        int64_t next_sum;
        if (i >= (size_t)short_window) {
            size_t old_short_slot = (i - (size_t)short_window) % (size_t)long_window;
            if (!fp_checked_add(short_sum, -ring[old_short_slot], &next_sum)) goto fail;
            short_sum = next_sum;
        }
        if (!fp_checked_add(short_sum, price, &next_sum)) goto fail;
        short_sum = next_sum;
        if (i >= (size_t)long_window) {
            if (!fp_checked_add(long_sum, -ring[long_slot], &next_sum)) goto fail;
            long_sum = next_sum;
        }
        if (!fp_checked_add(long_sum, price, &next_sum)) goto fail;
        long_sum = next_sum;
        ring[long_slot] = price;

        vwap_num += ((double)price / (double)PRICE_SCALE) * rows.qty[i];
        qty_sum += rows.qty[i];
        long long second = rows.ts[i] / 1000000LL;
        if (active_second != second) { ++bars; active_second = second; }
        if (i + 1U >= (size_t)long_window) {
            int sign = fp_compare_products((uint64_t)short_sum, (uint64_t)long_window,
                                           (uint64_t)long_sum, (uint64_t)short_window);
            int signal = 0;
            if (previous_ready) {
                if (previous_sign <= 0 && sign > 0) signal = 1;
                else if (previous_sign >= 0 && sign < 0) signal = -1;
            }
            if (signal > 0) { ++buys; hash = result_hash_signal(hash, i + 1U, 1); }
            if (signal < 0) { ++sells; hash = result_hash_signal(hash, i + 1U, -1); }
            previous_sign = sign;
            previous_ready = 1;
        }
    }
    uint64_t process_ns = plat_now_ns() - process_start;
    *result = (RunResult){"v9_soa", path, rows.count, short_window, long_window,
        qty_sum == 0.0 ? 0.0 : vwap_num / qty_sum, bars, buys, sells, hash,
        load_ns, process_ns, plat_now_ns() - start, plat_peak_memory_kb()};
    free(ring);
    soa_release(&rows);
    return 1;

fail:
    free(ring);
    soa_release(&rows);
    return 0;
}

#ifndef TICKTRADE_SOA_NO_MAIN
/* Sort repeated timing samples before selecting the median. */
static void soa_sort_times(uint64_t *values, unsigned count)
{
    for (unsigned i = 1; i < count; ++i) {
        uint64_t value = values[i];
        unsigned j = i;
        while (j > 0 && values[j - 1U] > value) { values[j] = values[j - 1U]; --j; }
        values[j] = value;
    }
}

/* Append a benchmark row in the same schema used by the other ladder rungs. */
static int soa_write_bench_csv(const char *path, const char *member,
                               const RunResult *result, unsigned repeats,
                               uint64_t best_ns, uint64_t worst_ns)
{
    static const char header[] = "platform,member,version,file,trades,repeats,median_total_ns,best_total_ns,worst_total_ns,median_process_ns,trades_per_sec,peak_mem_kb,compiler,flags";
    FILE *file = fopen(path, "a+");
    if (file == NULL) return 0;
    if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return 0; }
    long end = ftell(file);
    int written = end > 0;
    if (end < 0 || !result_csv_header_once(file, &written, header)) { fclose(file); return 0; }
    int ok = fprintf(file, "%s,%s,%s,%s,%llu,%u,%llu,%llu,%llu,%llu,%.6f,%llu,\"%s\",\"%s\"\n",
        plat_platform_name(), member, result->version, result->file,
        (unsigned long long)result->trades, repeats,
        (unsigned long long)result->total_ns, (unsigned long long)best_ns,
        (unsigned long long)worst_ns, (unsigned long long)result->process_ns,
        result->total_ns == 0 ? 0.0 : (double)result->trades * 1e9 / result->total_ns,
        (unsigned long long)result->peak_mem_kb, __VERSION__, ENGINE_BUILD_FLAGS) >= 0;
    return fclose(file) == 0 && ok;
}

/* Parse the shared CLI shape and print the v9 RESULT record. */
int main(int argc, char **argv)
{
    const char *path = NULL, *csv_path = NULL, *member = "member";
    int short_window = DEFAULT_SHORT_WINDOW, long_window = DEFAULT_LONG_WINDOW;
    unsigned repeats = 1;
    for (int i = 1; i < argc; ++i) {
        if (i + 1 >= argc) return 2;
        const char *key = argv[i++], *value = argv[i];
        if (strcmp(key, "--file") == 0) path = value;
        else if (strcmp(key, "--short") == 0) short_window = atoi(value);
        else if (strcmp(key, "--long") == 0) long_window = atoi(value);
        else if (strcmp(key, "--csv") == 0) csv_path = value;
        else if (strcmp(key, "--member") == 0) member = value;
        else if (strcmp(key, "--repeat") == 0) {
            char *end = NULL;
            unsigned long parsed = strtoul(value, &end, 10);
            if (end == value || *end != '\0' || parsed == 0 || parsed > 1000UL) return 2;
            repeats = (unsigned)parsed;
        }
        else return 2;
    }
    RunResult result;
    if (csv_path != NULL) {
        uint64_t *totals = (uint64_t *)malloc((size_t)repeats * sizeof *totals);
        uint64_t *processes = (uint64_t *)malloc((size_t)repeats * sizeof *processes);
        RunResult warmup;
        if (totals == NULL || processes == NULL) { free(totals); free(processes); return 6; }
        if (!soa_execute(path, short_window, long_window, &warmup)) { free(totals); free(processes); return 6; }
        uint64_t best = UINT64_MAX, worst = 0;
        for (unsigned i = 0; i < repeats; ++i) {
            if (!soa_execute(path, short_window, long_window, &result)) { free(totals); free(processes); return 6; }
            totals[i] = result.total_ns;
            processes[i] = result.process_ns;
            if (result.total_ns < best) best = result.total_ns;
            if (result.total_ns > worst) worst = result.total_ns;
        }
        soa_sort_times(totals, repeats);
        soa_sort_times(processes, repeats);
        result.total_ns = totals[repeats / 2U];
        result.process_ns = processes[repeats / 2U];
        free(totals);
        free(processes);
        if (!soa_write_bench_csv(csv_path, member, &result, repeats, best, worst)) return 6;
        printf("benchmark warmup=discarded repeats=%u best_ns=%llu median_ns=%llu worst_ns=%llu\n",
               repeats, (unsigned long long)best, (unsigned long long)result.total_ns,
               (unsigned long long)worst);
    } else if (!soa_execute(path, short_window, long_window, &result)) {
        fputs("ERROR: SoA input or processing failed.\n", stderr);
        return 6;
    }
    puts("Tick-to-Trade SoA stretch");
    int ok = result_print(stdout, &result);
    puts(ok ? "PASS" : "FAIL output");
    return ok ? 0 : 6;
}
#endif
