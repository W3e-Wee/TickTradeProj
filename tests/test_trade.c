/* Check the common trade record fields and their declared types are usable. */
#include "check.h"
#include "trade.h"
/* Initialize a representative course trade and verify field mapping. */
int main(void)
{
    Trade trade = {123, 6501234, 1500, 1};
    CHECK(trade.ts_us == 123 && trade.price == 6501234);
    CHECK(trade.qty == 1500 && trade.is_buy == 1);
    return CHECK_SUMMARY();
}
