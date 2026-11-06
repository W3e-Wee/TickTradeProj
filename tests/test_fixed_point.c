/* Verify decimal scaling, checked overflow, and wide exact comparisons. */
#include "check.h"
#include "fixed_point.h"
/* Check the brief's price/quantity worked check and overflow behavior. */
int main(void)
{
    int64_t price = 0, qty = 0, product = 0;
    FpAccumulator sum = {0, 0};
    CHECK(fp_parse_scaled("65012.34", 100, &price) && price == 6501234);
    CHECK(fp_parse_scaled("0.0015", 1000000, &qty) && qty == 1500);
    CHECK(fp_checked_mul(price, qty, &product) && product == 9751851000LL);
    CHECK_NEAR((double)product / 100000000.0, 97.51851, 1e-12);
    CHECK(!fp_checked_mul(INT64_MAX, 2, &product));
    CHECK(!fp_checked_add(INT64_MAX, 1, &product));
    CHECK(fp_parse_scaled("1.235", 100, &product) && product == 124);
    CHECK(fp_parse_scaled("-1.235", 100, &product) && product == -124);
    CHECK(fp_acc_add_product(&sum, (uint64_t)price, (uint64_t)qty));
    CHECK(sum.hi == 0 && sum.lo == 9751851000ULL);
    sum.hi = UINT64_MAX; sum.lo = UINT64_MAX - 1U;
    CHECK(!fp_acc_add_product(&sum, 1, 2));
    CHECK(sum.hi == UINT64_MAX && sum.lo == UINT64_MAX - 1U);
    CHECK(fp_compare_products(101, 5, 100, 5) > 0);
    CHECK(fp_compare_products(100, 5, 100, 5) == 0);
    return CHECK_SUMMARY();
}
