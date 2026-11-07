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

/* Load all prices, then derive crossover events before the reaction sweep. */
int main(int argc, char **argv)
{
    const char *path = "data/dev_1M.csv";
    const char *output_path = "results/rival/sweep.csv";
    const char *member = "member";
    for (int i = 1; i < argc; ++i) {
        if (i + 1 >= argc) return 2;
        const char *key = argv[i++], *value = argv[i];
        if (strcmp(key, "--file") == 0) path = value;
        else if (strcmp(key, "--output") == 0) output_path = value;
        else if (strcmp(key, "--member") == 0) member = value;
        else return 2;
    }
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
    if (ferror(file) || fclose(file) != 0 || count < 60) { free(rows); return 6; }
    int previous = 0;
    int64_t short_ring[10] = {0}, long_ring[60] = {0};
    int64_t short_sum = 0, long_sum = 0;
    for (size_t i = 0; i < count; ++i) {
        uint64_t started = now_ns();
        double scaled = rows[i].price * (double)PRICE_SCALE;
        if (!isfinite(scaled) || scaled < 0.5 || scaled >= (double)INT64_MAX) {
            free(rows); return 6;
        }
        int64_t price = (int64_t)llround(scaled);
        size_t short_slot = i % 10, long_slot = i % 60;
        if (i >= 10 && !fp_checked_add(short_sum, -short_ring[short_slot], &short_sum)) { free(rows); return 6; }
        if (i >= 60 && !fp_checked_add(long_sum, -long_ring[long_slot], &long_sum)) { free(rows); return 6; }
        short_ring[short_slot] = price; long_ring[long_slot] = price;
        if (!fp_checked_add(short_sum, price, &short_sum) ||
            !fp_checked_add(long_sum, price, &long_sum)) { free(rows); return 6; }
        if (i >= 59) {
            int sign = fp_compare_products((uint64_t)short_sum, 60U,
                                           (uint64_t)long_sum, 10U);
            if (previous <= 0 && sign > 0) rows[i].signal = 1;
            else if (previous >= 0 && sign < 0) rows[i].signal = -1;
            previous = sign;
        }
        rows[i].work_ns = now_ns() - started;
    }
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
