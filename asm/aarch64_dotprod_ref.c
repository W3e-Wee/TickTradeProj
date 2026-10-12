#include <stddef.h>
#include <stdint.h>

/*
 * Shared contract: a and b each point to count naturally aligned uint64_t
 * elements. Return sum(a[i] * b[i]) modulo 2^64; zero count returns zero.
 * No pointer is dereferenced at zero count. Arrays may overlap. Unsigned
 * arithmetic has defined modulo behavior and does not use floating point.
 */

uint64_t dot_product_ref(const uint64_t *a, const uint64_t *b, size_t count) {
    uint64_t sum = 0;
    for (size_t i = 0; i < count; ++i) {
        sum += a[i] * b[i];
    }
    return sum;
}

/* Non-AArch64 build: provide the same public entry point as a C fallback. */
#if !defined(TT_USE_AARCH64_ASM)
uint64_t dot_product_u64(const uint64_t *a, const uint64_t *b, size_t count) {
    return dot_product_ref(a, b, count);
}
#endif
