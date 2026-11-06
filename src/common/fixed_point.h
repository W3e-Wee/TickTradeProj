/* Exact decimal scaling and wide integer helpers for fixed-point engines. */
#ifndef TICKTRADE_FIXED_POINT_H
#define TICKTRADE_FIXED_POINT_H
#include <stdint.h>

/* C profiler found eight decimal places in both supplied numeric fields. */
#define PRICE_SCALE INT64_C(100000000)
#define QTY_SCALE INT64_C(100000000)

typedef struct { uint64_t hi; uint64_t lo; } FpAccumulator;
/* Convert a decimal string to its nearest scaled signed integer. */
int fp_parse_scaled(const char *text, int64_t scale, int64_t *value_out);
/* Checked signed arithmetic returns zero without modifying output on overflow. */
int fp_checked_add(int64_t left, int64_t right, int64_t *value_out);
int fp_checked_mul(int64_t left, int64_t right, int64_t *value_out);
/* Add a nonnegative product to a portable unsigned 128-bit accumulator. */
int fp_acc_add_product(FpAccumulator *accumulator, uint64_t left, uint64_t right);
/* Compare nonnegative sum*factor products exactly, returning -1, 0, or 1. */
int fp_compare_products(uint64_t left_sum, uint64_t left_factor,
                        uint64_t right_sum, uint64_t right_factor);
/* Convert the portable accumulator to double for reporting only. */
double fp_acc_to_double(const FpAccumulator *accumulator);

#endif
