/* Stage 0 recursive-versus-iterative benchmark with checked crossover sizes.
 * Usage: stage0_sum_recursion [--n N] [--reps R] [--repeat R].
 * Exit codes: 0 success, 2 invalid input, 3 out of range, 4 mismatch, 6 I/O.
 */
#include "cli.h"
#include "compat.h"
#include "exit_codes.h"
#include "stage0_math.h"
#include "timing.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define RECURSION_MAX_N 20000U
#define RECURSION_DEFAULT_N 10000U
#define RECURSION_DEFAULT_REPS 20000U
#define RECURSION_DEFAULT_BATCHES 5U
#define RECURSION_MAX_RUNS 1000U
#ifndef GUARD_OPTIMISER
#define GUARD_OPTIMISER 0
#endif
#ifndef OPT_LABEL
#define OPT_LABEL default
#endif
#define STAGE0_STRINGIFY_INNER(value) #value
#define STAGE0_STRINGIFY(value) STAGE0_STRINGIFY_INNER(value)
#define STAGE0_OPT_LABEL STAGE0_STRINGIFY(OPT_LABEL)
static volatile uint64_t recursion_sink;

/* Compute one recursive triangular sum; non-tail recursion prevents loop folding. */
static COMPAT_NOINLINE uint64_t recursion_sum(uint64_t n)
{
    if (n == 0) return 0;
    return n + recursion_sum(n - 1);
}

/* Compute one loop sum while preserving a visible result for the optimizer. */
static COMPAT_NOINLINE uint64_t iterative_sum(uint64_t n)
{
    uint64_t sum = 0;
    for (uint64_t i = 1; i <= n; ++i) sum += i;
#if GUARD_OPTIMISER
    COMPAT_OPTIMIZER_GUARD(sum);
#endif
    recursion_sink ^= sum;
    return sum;
}

/* Benchmark repeated recursive sums in one batch; n is read through volatile. */
static uint64_t recursion_batch(uint64_t n, uint64_t reps)
{
    volatile uint64_t runtime_n = n;
    uint64_t value = 0;
    for (uint64_t i = 0; i < reps; ++i) value ^= recursion_sum(runtime_n);
    recursion_sink ^= value;
    return value;
}

/* Benchmark repeated loop sums in one batch with the same observable input. */
static uint64_t iterative_batch(uint64_t n, uint64_t reps)
{
    volatile uint64_t runtime_n = n;
    uint64_t value = 0;
    for (uint64_t i = 0; i < reps; ++i) value ^= iterative_sum(runtime_n);
    recursion_sink ^= value;
    return value;
}

/* Sort timing batches so the median is deterministic. */
static void sort_times(uint64_t *values, unsigned count)
{
    for (unsigned i = 1; i < count; ++i) {
        uint64_t value = values[i]; unsigned j = i;
        while (j > 0 && values[j - 1] > value) { values[j] = values[j - 1]; --j; }
        values[j] = value;
    }
}

/* Time batch_fn repeatedly with all output deferred until the loop ends. */
static int measure_batches(uint64_t (*batch_fn)(uint64_t, uint64_t), uint64_t n,
                           uint64_t reps, unsigned batches, uint64_t *times)
{
    for (unsigned i = 0; i < batches; ++i) {
        uint64_t start = now_ns();
        (void)batch_fn(n, reps);
        times[i] = now_ns() - start;
    }
    sort_times(times, batches);
    return 1;
}

/* Append recursion comparison results with one CSV heading per empty file. */
static int write_csv(uint64_t n, uint64_t reps, unsigned batches,
                     uint64_t iterative_ns, uint64_t recursive_ns)
{
    FILE *file = fopen("results/bench/stage0_recursion.csv", "a+");
    if (file == NULL) return 0;
    if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return 0; }
    long end = ftell(file);
    if (end < 0) { fclose(file); return 0; }
    if (end == 0 && fprintf(file, "opt_level,mode,n,reps,batches,iterative_median_ns,recursive_median_ns,ratio\n") < 0) { fclose(file); return 0; }
    int ok = fprintf(file, "%s,%s,%" PRIu64 ",%" PRIu64 ",%u,%" PRIu64 ",%" PRIu64 ",%.6f\n",
                     STAGE0_OPT_LABEL, GUARD_OPTIMISER ? "guarded" : "plain", n, reps, batches,
                     iterative_ns, recursive_ns, iterative_ns == 0 ? 0.0 : (double)recursive_ns / iterative_ns) >= 0;
    return fclose(file) == 0 && ok;
}

/* Check one untimed result, benchmark both algorithms, and report their ratio. */
int main(int argc, char **argv)
{
    uint64_t n = RECURSION_DEFAULT_N, reps = RECURSION_DEFAULT_REPS, parsed;
    unsigned batches = RECURSION_DEFAULT_BATCHES;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0) { printf("Usage: %s [--n N] [--reps R] [--repeat R]\n", argv[0]); return EXIT_OK; }
        if (i + 1 >= argc || !cli_parse_u64(argv[i + 1], &parsed)) return EXIT_BAD_INPUT;
        if (strcmp(argv[i], "--n") == 0) { n = parsed; if (n > RECURSION_MAX_N) return EXIT_OUT_OF_RANGE; }
        else if (strcmp(argv[i], "--reps") == 0) { reps = parsed; if (reps == 0) return EXIT_OUT_OF_RANGE; }
        else if (strcmp(argv[i], "--repeat") == 0) { if (parsed == 0 || parsed > RECURSION_MAX_RUNS) return EXIT_OUT_OF_RANGE; batches = (unsigned)parsed; }
        else return EXIT_BAD_INPUT;
        ++i;
    }
    uint64_t expected, loop_check = iterative_sum(n), recursive_check = recursion_sum(n);
    if (!stage0_closed_form(n, &expected)) return EXIT_OUT_OF_RANGE;
    if (loop_check != expected || recursive_check != expected) return EXIT_MISMATCH;
    uint64_t iterative_times[RECURSION_MAX_RUNS], recursive_times[RECURSION_MAX_RUNS];
    (void)measure_batches(iterative_batch, n, reps, batches, iterative_times);
    (void)measure_batches(recursion_batch, n, reps, batches, recursive_times);
    uint64_t iterative_median = iterative_times[batches / 2];
    uint64_t recursive_median = recursive_times[batches / 2];
    printf("iterative_median_ns=%" PRIu64 " recursive_median_ns=%" PRIu64 " ratio=%.6f result=%" PRIu64 "\n",
           iterative_median, recursive_median,
           iterative_median == 0 ? 0.0 : (double)recursive_median / iterative_median, expected);
    if (!write_csv(n, reps, batches, iterative_median, recursive_median)) return EXIT_IO_ERROR;
    return EXIT_OK;
}
