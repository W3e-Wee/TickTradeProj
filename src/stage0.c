/* Stage 0 arithmetic demo: reads an iteration count, reports iterative and
 * guarded-recursive sums, and keeps recursion depth bounded for safety. */
#if defined(__linux__) && !defined(_POSIX_C_SOURCE) && !defined(_GNU_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <time.h>
#endif

#define MAX_SAFE_RECURSION_N 20000ULL
#define DEFAULT_EXPERIMENT_N 100000000ULL
#define DEFAULT_REPEAT_COUNT 20U

static uint64_t now_ns(void) {
#if defined(_WIN32)
    LARGE_INTEGER freq;
    LARGE_INTEGER count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    return (uint64_t)((1000000000ULL * (uint64_t)count.QuadPart) / (uint64_t)freq.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec * 1000000000ULL) + (uint64_t)ts.tv_nsec;
#endif
}

static uint64_t sum_iterative(uint64_t n) {
    uint64_t total = 0;
    for (uint64_t i = 1; i <= n; ++i) {
        total += i;
    }
    return total;
}

static uint64_t sum_recursive(uint64_t n) {
    if (n == 0) {
        return 0;
    }
    return n + sum_recursive(n - 1);
}

static int parse_n(const char *text, uint64_t *value_out) {
    char *end = NULL;
    errno = 0;
    unsigned long long parsed = strtoull(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0') {
        return -1;
    }
    *value_out = (uint64_t)parsed;
    return 0;
}

static int validate_sum(uint64_t n, uint64_t iterative_sum) {
    uint64_t closed_form = (n * (n + 1ULL)) / 2ULL;
    if (closed_form != iterative_sum) {
        fprintf(stderr, "Closed-form check failed for N=%" PRIu64 "\n", n);
        return 0;
    }
    return 1;
}

static void run_repeat_benchmark(uint64_t n, unsigned repeats) {
    uint64_t sum = 0;
    uint64_t start_ns = 0;
    uint64_t end_ns = 0;
    uint64_t best_ns = UINT64_MAX;
    uint64_t worst_ns = 0;

    for (unsigned i = 0; i < repeats; ++i) {
        start_ns = now_ns();
        sum = sum_iterative(n);
        end_ns = now_ns();
        uint64_t elapsed_ns = end_ns - start_ns;
        if (elapsed_ns < best_ns) {
            best_ns = elapsed_ns;
        }
        if (elapsed_ns > worst_ns) {
            worst_ns = elapsed_ns;
        }
    }

    printf("benchmark_n=%" PRIu64 " repeats=%u best_ns=%" PRIu64 " worst_ns=%" PRIu64 " result=%" PRIu64 "\n",
           n, repeats, best_ns, worst_ns, sum);
}

int main(int argc, char **argv) {
    uint64_t n = DEFAULT_EXPERIMENT_N;
    unsigned repeats = DEFAULT_REPEAT_COUNT;
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            if (strcmp(argv[i], "--n") == 0 && i + 1 < argc) {
                if (parse_n(argv[++i], &n) != 0) {
                    fprintf(stderr, "Invalid N input: %s\n", argv[i]);
                    return 2;
                }
            } else if (strcmp(argv[i], "--repeat") == 0 && i + 1 < argc) {
                char *end = NULL;
                unsigned long value = strtoul(argv[++i], &end, 10);
                if (end == argv[i] || *end != '\0' || value == 0) {
                    fprintf(stderr, "Invalid repeat count: %s\n", argv[i]);
                    return 2;
                }
                repeats = (unsigned)value;
            } else if (strcmp(argv[i], "--help") == 0) {
                printf("Usage: %s [--n N] [--repeat R]\n", argv[0]);
                return 0;
            } else {
                if (parse_n(argv[i], &n) != 0) {
                    fprintf(stderr, "Invalid N input: %s\n", argv[i]);
                    return 2;
                }
            }
        }
    }

    if (n > UINT64_MAX / 2ULL) {
        fprintf(stderr, "N is too large for the 1..N sum in uint64_t.\n");
        return 3;
    }

    const uint64_t iterative_sum = sum_iterative(n);
    uint64_t recursive_sum = 0;
    int recursion_status = 0;
    if (n <= MAX_SAFE_RECURSION_N) {
        recursive_sum = sum_recursive(n);
    } else {
        recursion_status = 1;
        printf("N=%" PRIu64 " recursion_skipped: recursive depth exceeds the safe stack budget.\n", n);
    }

    printf("N=%" PRIu64 " iterative_sum=%" PRIu64 " recursive_sum=%" PRIu64 " recursion_status=%s\n",
           n, iterative_sum, recursive_sum, recursion_status ? "SKIPPED" : "OK");

    if (!recursion_status && iterative_sum != recursive_sum) {
        fprintf(stderr, "Mismatch between iterative and recursive sums.\n");
        return 4;
    }

    if (!validate_sum(n, iterative_sum)) {
        return 5;
    }

    printf("stage0_note=iterative_sum_is_the_optimiser_relevant_value; recursive_path_is_guarded_for_stack_safety\n");
    run_repeat_benchmark(10000ULL, repeats);
    return 0;
}
