/* Stage 0 arithmetic reference used to validate iterative sums. */
#include "stage0_math.h"
#include <limits.h>
/* Divide the even factor first, then check the remaining multiplication. */
int stage0_closed_form(uint64_t n, uint64_t *sum_out)
{
    uint64_t left = n, right;
    if (sum_out == 0 || n == UINT64_MAX) return 0;
    right = n + 1U;
    if ((left & 1U) == 0) left /= 2U; else right /= 2U;
    if (left != 0 && right > UINT64_MAX / left) return 0;
    *sum_out = left * right;
    return 1;
}
