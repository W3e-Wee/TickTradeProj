/* Repeated timing statistics for ladder programs; uses course timing helpers. */
#ifndef TICKTRADE_BENCH_H
#define TICKTRADE_BENCH_H
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
typedef int (*BenchRunFn)(void *context, uint64_t *process_ns, uint64_t *trades);
typedef struct { uint64_t median_ns, best_ns, worst_ns, median_process_ns, trades; } BenchStats;
/* Run one warm-up and count timed callback runs into caller-owned arrays. */
int bench_measure(BenchRunFn run, void *context, size_t repeats, BenchStats *stats,
                  uint64_t *times, uint64_t *process_times);
/* Append a header-once benchmark row matching the shared CSV contract. */
int bench_write_row(FILE *stream, int *header_written, const char *row);
#endif
