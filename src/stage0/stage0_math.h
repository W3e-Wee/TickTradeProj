/* Overflow-safe closed-form helper shared by Stage 0 and its tests. */
#ifndef TICKTRADE_STAGE0_MATH_H
#define TICKTRADE_STAGE0_MATH_H
#include <stdint.h>
/* Compute n*(n+1)/2 without overflowing an intermediate uint64_t value. */
int stage0_closed_form(uint64_t n, uint64_t *sum_out);
#endif
