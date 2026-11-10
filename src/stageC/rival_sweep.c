/* Generate a deterministic crossover sequence and evaluate rival fill delay. */
#include "timing.h"
#include "fast_parser.h"
#include "fixed_point.h"
#include "platform.h"
#include "rival.h"
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct { double price; int signal; uint64_t work_ns; } Sample;

#ifndef DEFAULT_SHORT_WINDOW
#define DEFAULT_SHORT_WINDOW 30 /* Group 05 default; Make may override it. */
#endif
#ifndef DEFAULT_LONG_WINDOW
#define DEFAULT_LONG_WINDOW 60 /* Group 05 default; Make may override it. */
#endif

/* Load all prices, then derive crossover events before the reaction sweep. */
int main(int argc, char **argv)
{
    const char *path = "data/dev_1M.csv";
    const char *output_path = "results/rival/sweep.csv";
    const char *member = "member";
    int short_window = DEFAULT_SHORT_WINDOW, long_window = DEFAULT_LONG_WINDOW;
    for (int i = 1; i < argc; ++i) {
        if (i + 1 >= argc) return 2;
        const char *key = argv[i++], *value = argv[i];
        if (strcmp(key, "--file") == 0) path = value;
        else if (strcmp(key, "--output") == 0) output_path = value;
        else if (strcmp(key, "--member") == 0) member = value;
        else if (strcmp(key, "--short") == 0 || strcmp(key, "--long") == 0) {
            char *end = NULL;
            long parsed = strtol(value, &end, 10);
            if (end == value || *end != '\0' || parsed <= 0 || parsed > 100000) return 2;
            if (strcmp(key, "--short") == 0) short_window = (int)parsed;
            else long_window = (int)parsed;
        }
        else return 2;
    }
    if (long_window < short_window) return 2;
    FILE *file = fopen(path, "rb");
    if (file == NULL) { fprintf(stderr, "rival: cannot open input '%s': %s\n", path, strerror(errno)); return 6; }
    size_t cap = 4096, count = 0;
    Sample *rows = (Sample *)malloc(cap * sizeof *rows);
    if (rows == NULL) { fclose(file); return 6; }
    for (;;) {
        long long ts; double price, qty; int side;
        int status = fast_read_trade(file, &ts, &price, &qty, &side);
        if (status == 0) break;
        if (status < 0) { free(rows); fclose(file); return 6; }
        if (count == cap) {
            if (cap > SIZE_MAX / 2 / sizeof *rows) { free(rows); fclose(file); return 6; }
            cap *= 2;
            Sample *grown = (Sample *)realloc(rows, cap * sizeof *rows);
            if (grown == NULL) { free(rows); fclose(file); return 6; }
            rows = grown;
        }
        rows[count++] = (Sample){price, 0, 0};
    }
    if (ferror(file) || fclose(file) != 0 || count < (size_t)long_window) { free(rows); return 6; }
    int previous = 0;
    int64_t *short_ring = (int64_t *)calloc((size_t)short_window, sizeof *short_ring);
    int64_t *long_ring = (int64_t *)calloc((size_t)long_window, sizeof *long_ring);
    if (short_ring == NULL || long_ring == NULL) {
        free(rows); free(short_ring); free(long_ring); return 6;
    }
    int64_t short_sum = 0, long_sum = 0;
    for (size_t i = 0; i < count; ++i) {
        uint64_t started = now_ns();
        double scaled = rows[i].price * (double)PRICE_SCALE;
        if (!isfinite(scaled) || scaled < 0.5 || scaled >= (double)INT64_MAX) {
            free(rows); free(short_ring); free(long_ring); return 6;
        }
        int64_t price = (int64_t)llround(scaled);
        size_t short_slot = i % (size_t)short_window, long_slot = i % (size_t)long_window;
        if (i >= (size_t)short_window && !fp_checked_add(short_sum, -short_ring[short_slot], &short_sum)) { free(rows); free(short_ring); free(long_ring); return 6; }
        if (i >= (size_t)long_window && !fp_checked_add(long_sum, -long_ring[long_slot], &long_sum)) { free(rows); free(short_ring); free(long_ring); return 6; }
        short_ring[short_slot] = price; long_ring[long_slot] = price;
        if (!fp_checked_add(short_sum, price, &short_sum) ||
            !fp_checked_add(long_sum, price, &long_sum)) {
            free(rows); free(short_ring); free(long_ring); return 6;
        }
        if (i + 1U >= (size_t)long_window) {
            int sign = fp_compare_products((uint64_t)short_sum, (uint64_t)long_window,
                                           (uint64_t)long_sum, (uint64_t)short_window);
            if (previous <= 0 && sign > 0) rows[i].signal = 1;
            else if (previous >= 0 && sign < 0) rows[i].signal = -1;
            previous = sign;
        }
        rows[i].work_ns = now_ns() - started;
    }
    free(short_ring);
    free(long_ring);
    double *prices = (double *)malloc(count * sizeof *prices);
    int *signals = (int *)malloc(count * sizeof *signals);
    uint64_t *work = (uint64_t *)malloc(count * sizeof *work);
    double *points = (double *)malloc(25 * sizeof *points);
    if (prices == NULL || signals == NULL || work == NULL || points == NULL) {
        free(rows); free(prices); free(signals); free(work); free(points); return 6;
    }
    for (size_t i = 0; i < count; ++i) { prices[i] = rows[i].price; signals[i] = rows[i].signal; work[i] = rows[i].work_ns; }
    rival_sweep_points(points, 25);
    FILE *output = fopen(output_path, "wb");
    if (output == NULL) {
        fprintf(stderr, "rival: cannot write output in directory for '%s': %s\n", output_path, strerror(errno));
        free(rows); free(prices); free(signals); free(work); free(points); return 6;
    }
    fprintf(output, "platform,member,reaction_us,pnl,money_lost,trades\n");
    RivalResult baseline = rival_simulate(prices, signals, work, count, points[0]);
    for (size_t i = 0; i < 25; ++i) {
        RivalResult result = rival_simulate(prices, signals, work, count, points[i]);
        fprintf(output, "%s,%s,%.9g,%.9g,%.9g,%zu\n", plat_platform_name(),
                member, points[i], result.pnl, baseline.pnl - result.pnl, result.trades);
    }
    int close_failed = fclose(output) != 0;
    free(rows); free(prices); free(signals); free(work); free(points);
    return close_failed ? 6 : 0;
}
