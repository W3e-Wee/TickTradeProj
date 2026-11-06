/* Stretch v9: preload timestamp, price and quantity into separate arrays. */
#include "fast_parser.h"
#include "indicators.h"
#include "platform.h"
#include "result.h"
#include "signal_rule.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { long long *ts; double *price; double *qty; size_t count, cap; } SoaTrades;

/* Grow the three parallel columns together before processing starts. */
static int soa_grow(SoaTrades *rows)
{
    size_t next = rows->cap == 0 ? 4096 : rows->cap * 2;
    if (next < rows->cap || next > SIZE_MAX / sizeof *rows->ts) return 0;
    long long *ts = (long long *)realloc(rows->ts, next * sizeof *ts);
    if (ts == NULL) return 0;
    rows->ts = ts;
    double *price = (double *)realloc(rows->price, next * sizeof *price);
    if (price == NULL) return 0;
    rows->price = price;
    double *qty = (double *)realloc(rows->qty, next * sizeof *qty);
    if (qty == NULL) return 0;
    rows->qty = qty; rows->cap = next;
    return 1;
}

/* Read CSV into columns, process the strategy and emit the common result. */
int main(int argc, char **argv)
{
    const char *path = NULL; int short_window = 10, long_window = 60;
    for (int i = 1; i < argc; ++i) {
        if (i + 1 >= argc) return 2;
        const char *key = argv[i++], *value = argv[i];
        if (strcmp(key, "--file") == 0) path = value;
        else if (strcmp(key, "--short") == 0) short_window = atoi(value);
        else if (strcmp(key, "--long") == 0) long_window = atoi(value);
        else return 2;
    }
    if (path == NULL || short_window <= 0 || long_window < short_window) return 2;
    uint64_t start = plat_now_ns();
    FILE *file = fopen(path, "rb"); if (file == NULL) return 6;
    SoaTrades rows = {0}; int status;
    for (;;) {
        long long ts; double price, qty; int side;
        status = fast_read_trade(file, &ts, &price, &qty, &side);
        if (status <= 0) break;
        if (rows.count == rows.cap && !soa_grow(&rows)) { status = -1; break; }
        rows.ts[rows.count] = ts; rows.price[rows.count] = price; rows.qty[rows.count] = qty;
        ++rows.count;
    }
    int io_failed = status < 0 || ferror(file); int close_failed = fclose(file) != 0;
    uint64_t load_ns = plat_now_ns() - start;
    if (io_failed || close_failed || rows.count == 0) { free(rows.ts); free(rows.price); free(rows.qty); return 6; }
    double *ring = (double *)calloc((size_t)long_window, sizeof *ring);
    if (ring == NULL) { free(rows.ts); free(rows.price); free(rows.qty); return 6; }
    double vwap_num = 0.0, vwap_den = 0.0;
    uint64_t bars = 0, buys = 0, sells = 0, hash = UINT64_C(14695981039346656037);
    long long active_second = -1; uint64_t process_start = plat_now_ns();
    SignalDoubleState crossover = {0};
    for (size_t i = 0; i < rows.count; ++i) {
        ring[i % (size_t)long_window] = rows.price[i];
        vwap_num += rows.price[i] * rows.qty[i]; vwap_den += rows.qty[i];
        long long second = rows.ts[i] / 1000000LL;
        if (active_second != second) { ++bars; active_second = second; }
        if (i + 1U < (size_t)long_window) continue;
        double short_avg = 0.0, long_avg = 0.0;
        for (int j = 0; j < short_window; ++j)
            short_avg += ring[(i + (size_t)long_window - (size_t)short_window + 1U + (size_t)j) % (size_t)long_window];
        for (int j = 0; j < long_window; ++j)
            long_avg += ring[(i + 1U + (size_t)j) % (size_t)long_window];
        short_avg /= short_window; long_avg /= long_window;
        double difference = short_avg - long_avg;
        double epsilon = 0.5 / (100000000.0 * short_window * long_window);
        int signal = signal_double_step(&crossover, difference, epsilon);
        if (signal > 0) { ++buys; hash = result_hash_signal(hash, i + 1U, 1); }
        if (signal < 0) { ++sells; hash = result_hash_signal(hash, i + 1U, -1); }
    }
    uint64_t process_ns = plat_now_ns() - process_start;
    RunResult result = {"v9_soa", path, rows.count, short_window, long_window,
        vwap_den == 0.0 ? 0.0 : vwap_num / vwap_den, bars, buys, sells, hash,
        load_ns, process_ns, plat_now_ns() - start, plat_peak_memory_kb()};
    puts("Tick-to-Trade SoA stretch");
    int ok = result_print(stdout, &result); puts(ok ? "PASS" : "FAIL output");
    free(ring); free(rows.ts); free(rows.price); free(rows.qty);
    return ok ? 0 : 6;
}
