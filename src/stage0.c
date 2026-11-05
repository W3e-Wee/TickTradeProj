/*
    Stage 0 - Iteration and Recursive Sum Benchmarking:
    The program reads an upper limit of sum (N) and repeat count from command-line arguments.

    It computes the sum of integers from 1 to N using both iterative and recusive methods.
    Benchmarks each execution method using the following:
    - Best time
    - Worst time
    - Average time

    Validates the iterative sum against the closed-form formula for correctness.
    Guard for excessive recursion depth to prevent stack overflow.
*/
#include "timing.h"
#include "led.h"

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SAFE_RECURSION_N 20000ULL
#define DEFAULT_EXPERIMENT_N 100000000ULL
#define DEFAULT_REPEAT_COUNT 20U

/**
 * Loops from 1 to n, summing the values iteratively.
 *
 * @param n Upper limit of sum (inclusive).
 *
 * @return The sum of integers from 1 to n.
 */
static uint64_t sum_iterative(uint64_t n)
{
    uint64_t total = 0;
    for (uint64_t i = 1; i <= n; ++i)
    {
        total += i;
    }
    return total;
}

/**
 * Recursively sums integers from 1 to n.
 *
 * @param n Upper limit of sum (inclusive).
 *
 * @return The sum of integers from 1 to n.
 */
static uint64_t sum_recursive(uint64_t n)
{
    if (n == 0)
    {
        return 0;
    }
    return n + sum_recursive(n - 1);
}

/**
 * Runs a benchmark of the iterative sum function,
 * repeating it multiple times and reporting the best, worst and average execution times.
 *
 * @param n Upper limit of sum (inclusive).
 * @param repeats Number of times to repeat the benchmark.
 */
static void run_repeat_benchmark(uint64_t n, unsigned repeats)
{
    // Sums variables
    uint64_t iterative_sum = 0;
    uint64_t recursive_sum = 0;

    // Timing variables
    uint64_t avg_ns = 0;
    uint64_t start_ns = 0;
    uint64_t end_ns = 0;
    uint64_t best_ns = UINT64_MAX;
    uint64_t worst_ns = 0;

    printf("Benchmark Results:\n");

    // ===============================
    // Iterative sum benchmarking
    // ===============================
    for (unsigned i = 0; i < repeats; ++i)
    {
        start_ns = now_ns(); // start timing
        iterative_sum = sum_iterative(n);
        end_ns = now_ns(); // end timing

        // Accumulate total time for averaging
        avg_ns += (end_ns - start_ns);

        // Calculate elapsed time and update best/worst times
        uint64_t elapsed_ns = end_ns - start_ns;
        if (elapsed_ns < best_ns)
        {
            best_ns = elapsed_ns;
        }
        if (elapsed_ns > worst_ns)
        {
            worst_ns = elapsed_ns;
        }
    }
    printf("Iterative: \nN =%lu \tRepeats=%u \tBest-Time(ns)=%lu \tWorst-Time(ns)=%lu  Average-Time(ns)=%lu Result=%lu \n",
           n, repeats, best_ns, worst_ns, avg_ns / repeats, iterative_sum);

    // Reset timing variables for recursive benchmarking
    avg_ns = 0;

    // Guard against excessive recursion depth
    if (n > MAX_SAFE_RECURSION_N)
    {
        printf("Skipping recursive benchmark for N=%lu as it exceeds safe recursion limit of %llu.\n\n", n, MAX_SAFE_RECURSION_N);
        return;
    }

    // ===============================
    // Recursive sum benchmarking
    // ===============================
    for (unsigned i = 0; i < repeats; ++i)
    {
        start_ns = now_ns(); // start timing
        recursive_sum = sum_recursive(n);
        end_ns = now_ns(); // end timing

        // Accumulate total time for averaging
        avg_ns += (end_ns - start_ns);

        // Calculate elapsed time and update best/worst times
        uint64_t elapsed_ns = end_ns - start_ns;
        if (elapsed_ns < best_ns)
        {
            best_ns = elapsed_ns;
        }
        if (elapsed_ns > worst_ns)
        {
            worst_ns = elapsed_ns;
        }
    }

    printf("Recursive: \nN =%lu \tRepeats=%u \tBest-Time(ns)=%lu \tWorst-Time(ns)=%lu  Average-Time(ns)=%lu Result=%lu \n\n",
           n, repeats, best_ns, worst_ns, avg_ns / repeats, recursive_sum);
}

static int parse_n(const char *text, uint64_t *value_out)
{
    char *end = NULL;
    errno = 0;
    unsigned long long parsed = strtoull(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0')
    {
        return -1;
    }
    *value_out = (uint64_t)parsed;
    return 0;
}

static int validate_sum(uint64_t n, uint64_t iterative_sum)
{
    uint64_t closed_form = (n * (n + 1ULL)) / 2ULL;
    if (closed_form != iterative_sum)
    {
        fprintf(stderr, "Closed-form check failed for N=%" PRIu64 "\n", n);
        return 0;
    }
    return 1;
}

/***
 * Entry point for the Stage 0 arithmetic demo program.
 * 
 * @param argc Repeat count or upper limit of sum (N)
 * @param argv Command-line arguments (--n, --repeat, --help
 * 
 * @return Exit code
 */
int main(int argc, char **argv)
{
    uint64_t n = DEFAULT_EXPERIMENT_N;
    unsigned repeats = DEFAULT_REPEAT_COUNT;

    // Checks if there are command-line arguments
    // Uses default values if none are provided
    if (argc > 1)
    {
        // Parse command-line arguments to get N and repeat count
        for (int i = 1; i < argc; ++i)
        {
            // Check for the flag "--n" and parse the next argument as N
            if (strcmp(argv[i], "--n") == 0 && i + 1 < argc)
            {
                if (parse_n(argv[++i], &n) != 0)
                {
                    fprintf(stderr, "Invalid N input: %s\n", argv[i]);
                    return 2;
                }
            }
            // Check for the flag "--repeat" and parse the next argument as repeat count
            else if (strcmp(argv[i], "--repeat") == 0 && i + 1 < argc) 
            {
                char *end = NULL;
                unsigned long value = strtoul(argv[++i], &end, 10);
                if (end == argv[i] || *end != '\0' || value == 0)
                {
                    fprintf(stderr, "Invalid repeat count: %s\n", argv[i]);
                    return 2;
                }
                repeats = (unsigned)value;
            }
            // Check for the flag "--help" and display usage information
            else if (strcmp(argv[i], "--help") == 0)
            {
                printf("Usage: %s [--n N] [--repeat R]\n", argv[0]);
                return 0;
            }
            else
            {
                if (parse_n(argv[i], &n) != 0)
                {
                    fprintf(stderr, "Invalid N input: %s\n", argv[i]);
                    return 2;
                }
            }
        }
    }

    if (n > UINT64_MAX / 2ULL)
    {
        fprintf(stderr, "N is too large for the 1..N sum in uint64_t.\n");
        return 3;
    }

    const uint64_t start_ns = now_ns();
    const uint64_t iterative_sum = sum_iterative(n);
    const uint64_t end_ns = now_ns();
    const uint64_t elapsed_ns = end_ns - start_ns;
    printf("Iterative sum of %lu:\n", n);
    printf("N=%lu \tIterative_sum=%lu \tElapsed-Time(ns)=%lu\n\n", n, iterative_sum, elapsed_ns);

    // Sanity check: validate the iterative sum against the closed-form formula
    if (!validate_sum(n, iterative_sum))
    {
        return 5;
    }

    led_init(); // Initialize LED resources, checks for available hardware
    led_on(LED_GREEN); // Turn on green LED to indicate success

    // Run the benchmark for both iterative and recursive sums
    run_repeat_benchmark(10000ULL, repeats);

    led_off(LED_GREEN);
    led_cleanup(); // Clean up LED resources before exiting

    printf("Stage0 Note: Iterative sum is the optimiser relevant value; recursive path is guarded for stack safety\n");
    return 0;
}
