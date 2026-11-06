/* Preallocated replay latency samples and percentile summaries. */
#ifndef TICKTRADE_LATENCY_H
#define TICKTRADE_LATENCY_H
#include <stddef.h>
#include <stdint.h>
typedef struct { uint64_t *values; size_t capacity; size_t count; } LatencySamples;
typedef struct { double average_ns; uint64_t worst_ns, p50_ns, p99_ns; } LatencySummary;
/* Attach caller-allocated storage before replay begins. */
int latency_init(LatencySamples *samples, uint64_t *storage, size_t capacity);
/* Record a sample without allocating or printing in the replay loop. */
int latency_push(LatencySamples *samples, uint64_t value_ns);
/* Sort samples in place and calculate average, worst, p50, and p99. */
int latency_summarize(LatencySamples *samples, LatencySummary *summary);
#endif
