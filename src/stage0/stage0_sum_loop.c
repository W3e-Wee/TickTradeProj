/* Stage 0 loop sum benchmark with optional optimiser guard and LED timing.
 * Usage: stage0_sum_loop [--n N] [--repeat R]; exits 0, 2, 3, 5, or 6.
 */
#include "compat.h"
#include "cli.h"
#include "exit_codes.h"
#include "stage0_math.h"
#include "timing.h"
#include "led.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef OPT_LABEL
#define OPT_LABEL default
#endif
#define STAGE0_STRINGIFY_INNER(value) #value
#define STAGE0_STRINGIFY(value) STAGE0_STRINGIFY_INNER(value)
#define STAGE0_OPT_LABEL STAGE0_STRINGIFY(OPT_LABEL)
#ifndef GUARD_OPTIMISER
#define GUARD_OPTIMISER 0
#endif
#define STAGE0_DEFAULT_N UINT64_C(100000000)
#define STAGE0_MAX_N UINT64_C(6000000000)
#define STAGE0_DEFAULT_REPEATS 5U
#define STAGE0_MAX_REPEATS 1000U
static volatile uint64_t stage0_sink;

/* Sum 1..n using a visible accumulator; n is capped to prevent overflow. */
static COMPAT_NOINLINE uint64_t stage0_sum_loop(uint64_t n)
{
    uint64_t sum = 0;
    for (uint64_t i = 1; i <= n; ++i) sum += i;
#if GUARD_OPTIMISER
    COMPAT_OPTIMIZER_GUARD(sum);
#endif
    stage0_sink = sum;
    return sum;
}

/* Parse a decimal unsigned argument using the shared strict parser. */
static int stage0_parse_u64(const char *text, uint64_t maximum, uint64_t *out)
{
    uint64_t value;
    if (!cli_parse_u64(text, &value) || value > maximum) return 0;
    *out = value;
    return 1;
}

/* Sort the small run array so the middle sample gives the median. */
static void stage0_sort(uint64_t *values, unsigned count)
{
    for (unsigned i = 1; i < count; ++i) {
        uint64_t value = values[i]; unsigned j = i;
        while (j > 0 && values[j - 1] > value) { values[j] = values[j - 1]; --j; }
        values[j] = value;
    }
}

/* Append a Stage 0 row and write the CSV header only for an empty file. */
static int stage0_write_csv(uint64_t n, unsigned repeats, uint64_t best,
                            uint64_t median, uint64_t worst, uint64_t sum)
{
    FILE *file = fopen("results/bench/stage0_sum.csv", "a+");
    if (file == NULL) return 0;
    if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return 0; }
    long end = ftell(file);
    if (end < 0) { fclose(file); return 0; }
    if (end == 0 && fprintf(file, "opt_level,mode,n,repeats,best_ns,median_ns,worst_ns,ns_per_add,sum\n") < 0) { fclose(file); return 0; }
    int ok = fprintf(file, "%s,%s,%" PRIu64 ",%u,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%.6f,%" PRIu64 "\n",
                     STAGE0_OPT_LABEL, GUARD_OPTIMISER ? "guarded" : "plain", n, repeats,
                     best, median, worst, n == 0 ? 0.0 : (double)median / (double)n, sum) >= 0;
    return fclose(file) == 0 && ok;
}

/* Parse optional input, benchmark repeated sums, and validate closed form. */
int main(int argc, char **argv)
{
    uint64_t n = STAGE0_DEFAULT_N, repeats_value = STAGE0_DEFAULT_REPEATS, expected;
    int n_given = 0;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0) { printf("Usage: %s [--n N] [--repeat R]\n", argv[0]); return EXIT_OK; }
        if ((strcmp(argv[i], "--n") == 0 || strcmp(argv[i], "--repeat") == 0) && i + 1 < argc) {
            int is_n = strcmp(argv[i], "--n") == 0;
            if (!stage0_parse_u64(argv[++i], is_n ? STAGE0_MAX_N : STAGE0_MAX_REPEATS, is_n ? &n : &repeats_value)) return EXIT_BAD_INPUT;
            if (is_n) n_given = 1;
        } else if (argv[i][0] != '-') {
            if (!stage0_parse_u64(argv[i], STAGE0_MAX_N, &n) || n_given) return EXIT_BAD_INPUT;
            n_given = 1;
        } else { fprintf(stderr, "Unknown option: %s\n", argv[i]); return EXIT_BAD_INPUT; }
    }
    if (!n_given && argc == 1) {
        char input[64];
        printf("Enter N [default %" PRIu64 "]: ", STAGE0_DEFAULT_N);
        if (fgets(input, sizeof input, stdin) == NULL) return EXIT_BAD_INPUT;
        if (input[0] != '\n' && input[0] != '\r') {
            size_t length = strlen(input); while (length && (input[length-1] == '\n' || input[length-1] == '\r')) input[--length] = '\0';
            if (!stage0_parse_u64(input, STAGE0_MAX_N, &n)) return EXIT_BAD_INPUT;
        }
    }
    if (n == 0 || repeats_value == 0 || repeats_value > STAGE0_MAX_REPEATS) return EXIT_OUT_OF_RANGE;
    unsigned repeats = (unsigned)repeats_value;
    uint64_t times[STAGE0_MAX_REPEATS], result = 0, best = UINT64_MAX, worst = 0;
    if (!stage0_closed_form(n, &expected)) return EXIT_OUT_OF_RANGE;
    led_init();
    for (unsigned i = 0; i < repeats; ++i) {
        led_on(LED_GREEN);
        uint64_t start = now_ns();
        result = stage0_sum_loop(n);
        uint64_t elapsed = now_ns() - start;
        led_off(LED_GREEN);
        times[i] = elapsed;
        if (elapsed < best) best = elapsed;
        if (elapsed > worst) worst = elapsed;
    }
    led_cleanup();
    if (result != expected) { fprintf(stderr, "Closed-form check failed.\n"); return EXIT_SELF_CHECK_FAIL; }
    stage0_sort(times, repeats);
    uint64_t median = times[repeats / 2];
    printf("Stage 0 loop sum opt=%s mode=%s n=%" PRIu64 " repeats=%u\n",
           STAGE0_OPT_LABEL, GUARD_OPTIMISER ? "guarded" : "plain", n, repeats);
    printf("sum=%" PRIu64 " median_ns=%" PRIu64 " best_ns=%" PRIu64 " worst_ns=%" PRIu64 " ns_per_add=%.6f\n",
           result, median, best, worst, (double)median / (double)n);
    if (n >= UINT64_C(10000000) && median < UINT64_C(5000000)) fprintf(stderr, "WARNING: impossibly fast result; verify optimiser guard and workload.\n");
    if (!stage0_write_csv(n, repeats, best, median, worst, result)) { fprintf(stderr, "Unable to append Stage 0 CSV.\n"); return EXIT_IO_ERROR; }
    return EXIT_OK;
}
