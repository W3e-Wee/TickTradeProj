/* Warm-up, timed repetitions, and median/best/worst benchmark statistics. */
#include "bench.h"
#include "timing.h"

/* Sort unsigned timings in place; insertion sort is efficient for small repeats. */
static void bench_sort(uint64_t *values, size_t count)
{
    for (size_t i = 1; i < count; ++i) {
        uint64_t value = values[i]; size_t j = i;
        while (j > 0 && values[j - 1] > value) { values[j] = values[j - 1]; --j; }
        values[j] = value;
    }
}

/* Time callback invocations without reporting or file output in the interval. */
int bench_measure(BenchRunFn run, void *context, size_t repeats, BenchStats *stats,
                  uint64_t *times, uint64_t *process_times)
{
    uint64_t ignored_process = 0, ignored_trades = 0;
    if (run == NULL || stats == NULL || times == NULL || process_times == NULL || repeats == 0) return 0;
    if (!run(context, &ignored_process, &ignored_trades)) return 0;
    stats->best_ns = UINT64_MAX; stats->worst_ns = 0; stats->trades = 0;
    for (size_t i = 0; i < repeats; ++i) {
        uint64_t process_ns = 0, trades = 0;
        uint64_t start = now_ns();
        int ok = run(context, &process_ns, &trades);
        uint64_t elapsed = now_ns() - start;
        if (!ok) return 0;
        times[i] = elapsed; process_times[i] = process_ns; stats->trades = trades;
        if (elapsed < stats->best_ns) stats->best_ns = elapsed;
        if (elapsed > stats->worst_ns) stats->worst_ns = elapsed;
    }
    bench_sort(times, repeats); bench_sort(process_times, repeats);
    if ((repeats & 1U) != 0) {
        stats->median_ns = times[repeats / 2];
        stats->median_process_ns = process_times[repeats / 2];
    } else {
        size_t middle = repeats / 2;
        stats->median_ns = times[middle - 1] + (times[middle] - times[middle - 1]) / 2U;
        stats->median_process_ns = process_times[middle - 1] +
            (process_times[middle] - process_times[middle - 1]) / 2U;
    }
    return 1;
}

/* Write a preformatted row only after its caller has written the header once. */
int bench_write_row(FILE *stream, int *header_written, const char *row)
{
    if (stream == NULL || header_written == NULL || row == NULL || !*header_written) return 0;
    return fprintf(stream, "%s\n", row) >= 0;
}
