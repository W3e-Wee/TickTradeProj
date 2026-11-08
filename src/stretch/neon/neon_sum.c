/* Optional AArch64 NEON VWAP-numerator kernel and portable comparison driver. */
#include "platform.h"
#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#endif

#define NEON_SAMPLE_COUNT 1000000U
#define NEON_REPEAT_COUNT 5U
#define NEON_RELATIVE_TOLERANCE 1e-12

double neon_scalar_vwap_numerator(const double *prices, const double *quantities,
                                  size_t count);

/* Sum price times quantity with NEON where AArch64 vector support is present. */
static double neon_vwap_numerator(const double *prices, const double *quantities,
                                 size_t count)
{
    size_t i = 0;
    double sum = 0.0;
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    float64x2_t vector_sum = vdupq_n_f64(0.0);
    for (; i + 1U < count; i += 2U) {
        float64x2_t price = vld1q_f64(prices + i);
        float64x2_t quantity = vld1q_f64(quantities + i);
        vector_sum = vaddq_f64(vector_sum, vmulq_f64(price, quantity));
    }
    sum = vaddvq_f64(vector_sum);
#endif
    for (; i < count; ++i) sum += prices[i] * quantities[i];
    return sum;
}

/* Measure warmed scalar and vector kernels using arrays prepared beforehand. */
static int neon_benchmark(void)
{
    double *prices = (double *)malloc(NEON_SAMPLE_COUNT * sizeof *prices);
    double *quantities = (double *)malloc(NEON_SAMPLE_COUNT * sizeof *quantities);
    uint64_t scalar_total_ns = 0, neon_total_ns = 0;
    volatile double sink = 0.0;
    if (prices == NULL || quantities == NULL) { free(prices); free(quantities); return 0; }
    for (size_t i = 0; i < NEON_SAMPLE_COUNT; ++i) {
        prices[i] = 100.0 + (double)(i % 10000U) / 100.0;
        quantities[i] = 0.001 + (double)(i % 100U) / 1000.0;
    }
    for (unsigned repeat = 0; repeat < NEON_REPEAT_COUNT; ++repeat) {
        uint64_t start = plat_now_ns();
        sink += neon_scalar_vwap_numerator(prices, quantities, NEON_SAMPLE_COUNT);
        scalar_total_ns += plat_now_ns() - start;
        start = plat_now_ns();
        sink += neon_vwap_numerator(prices, quantities, NEON_SAMPLE_COUNT);
        neon_total_ns += plat_now_ns() - start;
    }
    double scalar = neon_scalar_vwap_numerator(prices, quantities, NEON_SAMPLE_COUNT);
    double vector = neon_vwap_numerator(prices, quantities, NEON_SAMPLE_COUNT);
    int same = fabs(scalar - vector) <= fabs(scalar) * NEON_RELATIVE_TOLERANCE + 1e-9;
    printf("NEON samples=%u repeats=%u scalar_mean_ns=%llu kernel_mean_ns=%llu equal=%s sink=%.3f\n",
           NEON_SAMPLE_COUNT, NEON_REPEAT_COUNT,
           (unsigned long long)(scalar_total_ns / NEON_REPEAT_COUNT),
           (unsigned long long)(neon_total_ns / NEON_REPEAT_COUNT), same ? "yes" : "no", sink);
    free(prices);
    free(quantities);
    return same;
}

/* Check a small worked numerator and run the measured kernel comparison. */
int main(void)
{
    const double prices[] = {1.25, 2.5, 4.0, 8.25, 16.0};
    const double quantities[] = {2.0, 2.0, 1.0, 1.0, 0.5};
    double result = neon_vwap_numerator(prices, quantities,
                                        sizeof prices / sizeof prices[0]);
    double scalar = neon_scalar_vwap_numerator(prices, quantities,
                                                sizeof prices / sizeof prices[0]);
    if (result != 27.75 || scalar != result) { puts("FAIL NEON VWAP kernel"); return 1; }
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    puts("PASS neon=enabled worked_numerator=27.75");
#else
    puts("PASS neon=unavailable scalar_fallback=used worked_numerator=27.75");
#endif
    return neon_benchmark() ? 0 : 1;
}
