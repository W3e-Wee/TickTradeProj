/* Latency collection uses caller storage so replay loops remain allocation-free. */
#include "latency.h"

/* Sort an existing sample range to make percentile extraction deterministic. */
static void latency_sort(uint64_t *values, size_t count)
{
    for (size_t i = 1; i < count; ++i) {
        uint64_t value = values[i]; size_t j = i;
        while (j > 0 && values[j - 1] > value) { values[j] = values[j - 1]; --j; }
        values[j] = value;
    }
}

/* Initialize bounded storage before any timed replay work starts. */
int latency_init(LatencySamples *samples, uint64_t *storage, size_t capacity)
{
    if (samples == NULL || storage == NULL || capacity == 0) return 0;
    samples->values = storage; samples->capacity = capacity; samples->count = 0;
    return 1;
}

/* Append one latency sample unless the caller-provided array is full. */
int latency_push(LatencySamples *samples, uint64_t value_ns)
{
    if (samples == NULL || samples->values == NULL || samples->count == samples->capacity) return 0;
    samples->values[samples->count++] = value_ns;
    return 1;
}

/* Summarize samples with nearest-rank p50 and p99 definitions. */
int latency_summarize(LatencySamples *samples, LatencySummary *summary)
{
    long double total = 0.0L;
    if (samples == NULL || summary == NULL || samples->count == 0) return 0;
    latency_sort(samples->values, samples->count);
    for (size_t i = 0; i < samples->count; ++i) total += samples->values[i];
    summary->average_ns = (double)(total / samples->count);
    summary->worst_ns = samples->values[samples->count - 1];
    size_t p50_rank = (samples->count / 100) * 50 +
        ((samples->count % 100) * 50 + 99) / 100;
    size_t p99_rank = (samples->count / 100) * 99 +
        ((samples->count % 100) * 99 + 99) / 100;
    summary->p50_ns = samples->values[p50_rank - 1];
    summary->p99_ns = samples->values[p99_rank - 1];
    return 1;
}
