/* Checked decimal conversion, signed arithmetic, and two-limb wide products. */
#include "fixed_point.h"
#include "compat.h"
#include <limits.h>
#include <stddef.h>

/* Multiply two unsigned words into high and low limbs without extensions. */
static void fp_mul_limb(uint64_t left, uint64_t right, uint64_t *high, uint64_t *low)
{
    uint64_t a0 = (uint32_t)left, a1 = left >> 32;
    uint64_t b0 = (uint32_t)right, b1 = right >> 32;
    uint64_t p0 = a0 * b0, p1 = a0 * b1, p2 = a1 * b0, p3 = a1 * b1;
    uint64_t middle = (p0 >> 32) + (uint32_t)p1 + (uint32_t)p2;
    *low = (middle << 32) | (uint32_t)p0;
    *high = p3 + (p1 >> 32) + (p2 >> 32) + (middle >> 32);
}

/* Parse a decimal with at most the scale precision and round excess digits. */
int fp_parse_scaled(const char *text, int64_t scale, int64_t *value_out)
{
    const char *cursor = text;
    uint64_t whole = 0, fraction = 0;
    unsigned scale_digits = 0, used = 0, discarded = 0;
    int negative = 0, round_up = 0;
    if (text == NULL || value_out == NULL || scale <= 0) return 0;
    while (scale % 10 == 0) { scale_digits++; scale /= 10; }
    if (scale != 1) return 0;
    scale = 1;
    for (unsigned i = 0; i < scale_digits; ++i) scale *= 10;
    if (*cursor == '-') { negative = 1; ++cursor; }
    else if (*cursor == '+') { ++cursor; }
    if (*cursor < '0' || *cursor > '9') return 0;
    while (*cursor >= '0' && *cursor <= '9') {
        unsigned digit = (unsigned)(*cursor++ - '0');
        if (whole > (UINT64_MAX - digit) / 10U) return 0;
        whole = whole * 10U + digit;
    }
    if (*cursor == '.') {
        ++cursor;
        if (*cursor < '0' || *cursor > '9') return 0;
        while (*cursor >= '0' && *cursor <= '9') {
            unsigned digit = (unsigned)(*cursor++ - '0');
            if (used < scale_digits) { fraction = fraction * 10U + digit; ++used; }
            else if (discarded == 0) { round_up = digit >= 5U; ++discarded; }
        }
    }
    if (*cursor != '\0') return 0;
    while (used++ < scale_digits) fraction *= 10U;
    if (whole > UINT64_MAX / (uint64_t)scale) return 0;
    uint64_t magnitude = whole * (uint64_t)scale;
    if (magnitude > UINT64_MAX - fraction) return 0;
    magnitude += fraction;
    if (round_up) { if (magnitude == UINT64_MAX) return 0; ++magnitude; }
    if ((!negative && magnitude > INT64_MAX) ||
        (negative && magnitude > (uint64_t)INT64_MAX + 1U)) return 0;
    if (negative && magnitude == (uint64_t)INT64_MAX + 1U) *value_out = INT64_MIN;
    else *value_out = negative ? -(int64_t)magnitude : (int64_t)magnitude;
    return 1;
}

/* Add signed values only after proving the result stays in int64 range. */
int fp_checked_add(int64_t left, int64_t right, int64_t *value_out)
{
    if (value_out == NULL || (right > 0 && left > INT64_MAX - right) ||
        (right < 0 && left < INT64_MIN - right)) return 0;
    *value_out = left + right;
    return 1;
}

/* Multiply signed values with a 128-bit check where available. */
int fp_checked_mul(int64_t left, int64_t right, int64_t *value_out)
{
    if (value_out == NULL) return 0;
#if COMPAT_HAS_INT128
    __extension__ typedef __int128 fp_i128;
    fp_i128 product = (fp_i128)left * (fp_i128)right;
    if (product > INT64_MAX || product < INT64_MIN) return 0;
    *value_out = (int64_t)product;
    return 1;
#else
    if (left == 0 || right == 0) { *value_out = 0; return 1; }
    if (left == -1 && right == INT64_MIN) return 0;
    if (right == -1 && left == INT64_MIN) return 0;
    if (left > 0 ? (right > 0 ? left > INT64_MAX / right : right < INT64_MIN / left)
                 : (right > 0 ? left < INT64_MIN / right : left < INT64_MAX / right)) return 0;
    *value_out = left * right;
    return 1;
#endif
}

/* Accumulate one nonnegative product and detect overflow beyond two limbs. */
int fp_acc_add_product(FpAccumulator *accumulator, uint64_t left, uint64_t right)
{
    uint64_t product_hi, product_lo, old_lo, new_lo, carry;
    if (accumulator == NULL) return 0;
    fp_mul_limb(left, right, &product_hi, &product_lo);
    old_lo = accumulator->lo;
    new_lo = old_lo + product_lo;
    carry = new_lo < old_lo;
    if (product_hi == UINT64_MAX && carry != 0) return 0;
    product_hi += carry;
    if (accumulator->hi > UINT64_MAX - product_hi) return 0;
    accumulator->lo = new_lo;
    accumulator->hi += product_hi;
    return 1;
}

/* Compare exact wide products using portable limbs on every compiler. */
int fp_compare_products(uint64_t left_sum, uint64_t left_factor,
                        uint64_t right_sum, uint64_t right_factor)
{
#if COMPAT_HAS_INT128
    __extension__ typedef unsigned __int128 fp_u128;
    fp_u128 left = (fp_u128)left_sum * left_factor;
    fp_u128 right = (fp_u128)right_sum * right_factor;
    return left < right ? -1 : left > right ? 1 : 0;
#else
    uint64_t left_hi, left_lo, right_hi, right_lo;
    fp_mul_limb(left_sum, left_factor, &left_hi, &left_lo);
    fp_mul_limb(right_sum, right_factor, &right_hi, &right_lo);
    if (left_hi != right_hi) return left_hi < right_hi ? -1 : 1;
    if (left_lo != right_lo) return left_lo < right_lo ? -1 : 1;
    return 0;
#endif
}

/* Convert the limbs only at the output boundary, never for decisions. */
double fp_acc_to_double(const FpAccumulator *accumulator)
{
    return accumulator == NULL ? 0.0 : (double)accumulator->hi * 18446744073709551616.0 + (double)accumulator->lo;
}
