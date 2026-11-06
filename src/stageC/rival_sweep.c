/* Generate a deterministic crossover sequence and evaluate rival fill delay. */
#include "fast_parser.h"
#include "rival.h"
#include "timing.h"
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
    for (int i = 1; i < argc; ++i) {
        if (i + 1 >= argc) return 2;
        const char *key = argv[i++], *value = argv[i];
        if (strcmp(key, "--file") == 0) path = value;
        else if (strcmp(key, "--output") == 0) output_path = value;
        else return 2;
    }
    FILE *file = fopen(path, "rb");
    if (file == NULL) return 6;
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
    for (size_t i = 59; i < count; ++i) {
        uint64_t started = now_ns();
        double short_avg = 0.0, long_avg = 0.0;
        for (size_t j = i - 9; j <= i; ++j) short_avg += rows[j].price;
        for (size_t j = i - 59; j <= i; ++j) long_avg += rows[j].price;
        int sign = short_avg * 60.0 > long_avg * 10.0 ? 1 :
                   short_avg * 60.0 < long_avg * 10.0 ? -1 : 0;
        if (previous <= 0 && sign > 0) rows[i].signal = 1;
        else if (previous >= 0 && sign < 0) rows[i].signal = -1;
        previous = sign;
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
    if (output == NULL) { free(rows); free(prices); free(signals); free(work); free(points); return 6; }
    fprintf(output, "reaction_us,pnl,money_lost,trades\n");
    RivalResult baseline = rival_simulate(prices, signals, work, count, points[0]);
    for (size_t i = 0; i < 25; ++i) {
        RivalResult result = rival_simulate(prices, signals, work, count, points[i]);
        fprintf(output, "%.9g,%.9g,%.9g,%zu\n", points[i], result.pnl,
                baseline.pnl - result.pnl, result.trades);
    }
    int close_failed = fclose(output) != 0;
    free(rows); free(prices); free(signals); free(work); free(points);
    return close_failed ? 6 : 0;
}
