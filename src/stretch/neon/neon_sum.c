/* Optional AArch64 NEON sum kernel with a portable scalar fallback. */
#include <stddef.h>
#include <stdio.h>
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#endif

/* Sum doubles with NEON where the compiler exposes a 128-bit vector ISA. */
static double neon_sum(const double *values, size_t count)
{
    size_t i = 0; double sum = 0.0;
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    float64x2_t vector_sum = vdupq_n_f64(0.0);
    for (; i + 1 < count; i += 2) vector_sum = vaddq_f64(vector_sum, vld1q_f64(values + i));
    sum = vaddvq_f64(vector_sum);
#endif
    for (; i < count; ++i) sum += values[i];
    return sum;
}

/* Keep the kernel check deterministic on both AArch64 and non-NEON hosts. */
int main(void)
{
    const double values[] = {1.25, 2.5, 4.0, 8.25, 16.0};
    double result = neon_sum(values, sizeof values / sizeof values[0]);
    if (result != 32.0) { puts("FAIL neon sum"); return 1; }
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    puts("PASS neon=enabled sum=32");
#else
    puts("PASS neon=unavailable scalar_fallback=used sum=32");
#endif
    return 0;
}
