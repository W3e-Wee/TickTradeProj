/* Optimised portable C reference for the optional NEON VWAP kernel. */
#include <stddef.h>

/* Sum the price-quantity products as the -O3 comparison implementation. */
double neon_scalar_vwap_numerator(const double *prices, const double *quantities,
                                  size_t count)
{
    double sum = 0.0;
    for (size_t i = 0; i < count; ++i) sum += prices[i] * quantities[i];
    return sum;
}
