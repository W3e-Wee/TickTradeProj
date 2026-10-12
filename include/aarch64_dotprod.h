#ifndef TICK_TO_TRADE_AARCH64_DOTPROD_H
#define TICK_TO_TRADE_AARCH64_DOTPROD_H

#include <stddef.h>
#include <stdint.h>

/* Isolated unsigned price/quantity-style accumulation kernel.
 * Arrays contain count naturally aligned uint64_t values. The result wraps
 * modulo 2^64, count zero returns zero, and no element is read at zero.
 */
uint64_t dot_product_u64(const uint64_t *a, const uint64_t *b, size_t count);
uint64_t dot_product_ref(const uint64_t *a, const uint64_t *b, size_t count);

#endif
