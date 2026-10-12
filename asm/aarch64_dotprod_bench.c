#include "aarch64_dotprod.h"
#include "timing.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ELEMENT_COUNT 262144U
#define TIMED_CALLS 128U
#define REPEATS 5U
#if defined(TT_USE_AARCH64_ASM)
#define KERNEL_NAME "aarch64_asm"
#else
#define KERNEL_NAME "portable_c_fallback"
#endif

static volatile uint64_t observed_checksum;

static uint64_t next_random(uint64_t *state) {
    /* Deterministic runtime population; values are not compile-time constants. */
    uint64_t x = *state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *state = x;
    return x;
}

static int compare(const uint64_t *a, const uint64_t *b, size_t count) {
    uint64_t expected = dot_product_ref(a, b, count);
    uint64_t actual = dot_product_u64(a, b, count);
    if (actual != expected) {
        fprintf(stderr, "mismatch count=%zu ref=%" PRIu64 " kernel=%" PRIu64 "\n",
                count, expected, actual);
        return 0;
    }
    return 1;
}

static int correctness_tests(void) {
    uint64_t a[513], b[513];
    uint64_t state = UINT64_C(0x4d595df4d0f33173);
    size_t i;
    const uint64_t fixed_a[] = {1, 2, 3, 4, UINT64_MAX};
    const uint64_t fixed_b[] = {9, 8, 7, 6, 2};
    uint64_t fixed_expected = UINT64_C(70) + UINT64_MAX * UINT64_C(2);

    if (dot_product_u64(NULL, NULL, 0) != 0) {
        fputs("zero-count contract failed\n", stderr);
        return 0;
    }
    if (!compare(fixed_a, fixed_b, 1) ||
        dot_product_ref(fixed_a, fixed_b, 5) != fixed_expected ||
        !compare(fixed_a, fixed_b, 5)) {
        fputs("fixed known-array test failed\n", stderr);
        return 0;
    }
    for (i = 0; i < 513; ++i) {
        a[i] = next_random(&state);
        b[i] = next_random(&state);
    }
    if (!compare(a, b, 2) || !compare(a, b, 513)) return 0;
    for (i = 0; i < 513; ++i) {
        a[i] = UINT64_C(0xfeedfacecafebeef);
        b[i] = (i & 1U) ? UINT64_MAX : 3;
    }
    if (!compare(a, b, 513)) return 0;

    /* Naturally aligned pointers and a boundary count not divisible by 4. */
    if (!compare(a + 1, b + 1, 511)) return 0;
    puts("assembly kernel correctness: PASS (zero, one, fixed, random, repeated, boundary, wraparound)");
    return 1;
}

static uint64_t timed_run(int use_kernel, const uint64_t *a, const uint64_t *b,
                          size_t count, uint64_t *elapsed_ns) {
    unsigned int call;
    uint64_t checksum = 0;
    uint64_t started = now_ns();
    for (call = 0; call < TIMED_CALLS; ++call) {
        checksum += use_kernel ? dot_product_u64(a, b, count)
                               : dot_product_ref(a, b, count);
    }
    *elapsed_ns = now_ns() - started;
    observed_checksum = checksum;
    return checksum;
}

static int run_benchmark(unsigned int repeats) {
    uint64_t *a = malloc(sizeof(*a) * ELEMENT_COUNT);
    uint64_t *b = malloc(sizeof(*b) * ELEMENT_COUNT);
    uint64_t state = UINT64_C(0x9e3779b97f4a7c15);
    unsigned int i, repeat;
    if (a == NULL || b == NULL) {
        fputs("unable to allocate runtime input arrays\n", stderr);
        free(a); free(b);
        return 2;
    }
    for (i = 0; i < ELEMENT_COUNT; ++i) {
        a[i] = next_random(&state);
        b[i] = next_random(&state);
    }
    /* Correctness gate and warm-up are outside all timed intervals. */
    if (!compare(a, b, ELEMENT_COUNT)) { free(a); free(b); return 1; }
    (void)dot_product_ref(a, b, ELEMENT_COUNT);
    (void)dot_product_u64(a, b, ELEMENT_COUNT);
    puts("kernel,repeat,elements,calls,elapsed_ns,checksum");
    for (repeat = 1; repeat <= repeats; ++repeat) {
        uint64_t elapsed, checksum;
        int asm_first = repeat & 1U;
        const char *first = asm_first ? KERNEL_NAME : "c_reference";
        const char *second = asm_first ? "c_reference" : KERNEL_NAME;
        checksum = timed_run(asm_first, a, b, ELEMENT_COUNT, &elapsed);
        printf("%s,%u,%u,%u,%" PRIu64 ",%" PRIu64 "\n",
               first, repeat, ELEMENT_COUNT, TIMED_CALLS, elapsed, checksum);
        checksum = timed_run(!asm_first, a, b, ELEMENT_COUNT, &elapsed);
        printf("%s,%u,%u,%u,%" PRIu64 ",%" PRIu64 "\n",
               second, repeat, ELEMENT_COUNT, TIMED_CALLS, elapsed, checksum);
    }
    printf("# observable_checksum=%" PRIu64 "\n", observed_checksum);
    free(a); free(b);
    return 0;
}

int main(int argc, char **argv) {
    unsigned long requested_repeats = REPEATS;
    if (argc < 2 || argc > 3 || (strcmp(argv[1], "--test") != 0 &&
                                  strcmp(argv[1], "--benchmark") != 0)) {
        fprintf(stderr, "usage: %s --test | --benchmark [repeats]\n", argv[0]);
        return 2;
    }
    if (argc == 3) {
        char *end = NULL;
        requested_repeats = strtoul(argv[2], &end, 10);
        if (end == argv[2] || *end != '\0' || requested_repeats == 0 ||
            requested_repeats > 10000) {
            fputs("repeats must be in 1..10000\n", stderr);
            return 2;
        }
    }
    if (!correctness_tests()) return 1;
    if (strcmp(argv[1], "--test") == 0) return 0;
    return run_benchmark((unsigned int)requested_repeats);
}
