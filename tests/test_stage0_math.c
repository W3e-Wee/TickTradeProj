/* Verify Stage 0's closed form on normal values and intermediate overflow bounds. */
#include "check.h"
#include "stage0_math.h"
#include <stdint.h>
/* Check the even-factor reduction and UINT64 boundary rejection. */
int main(void)
{
    uint64_t sum = 0;
    CHECK(stage0_closed_form(10000, &sum) && sum == 50005000);
    CHECK(stage0_closed_form(UINT64_C(6000000000), &sum));
    CHECK(!stage0_closed_form(UINT64_MAX, &sum));
    CHECK(!stage0_closed_form(1, NULL));
    return CHECK_SUMMARY();
}
