/* Verify rival fill timing, position rules, and sweep endpoints. */
#include "check.h"
#include "rival.h"
/* Check the specified +60 P&L and exact reaction-time boundary behavior. */
int main(void)
{
    const double prices[] = {65000, 65040, 64990, 65010};
    const int signals[] = {1, -1, 1, -1};
    const uint64_t fast[] = {1, 1, 1, 1};
    const uint64_t equal[] = {100000, 100000, 100000, 100000};
    double points[25];
    RivalResult result = rival_simulate(prices, signals, fast, 4, 100.0);
    CHECK_NEAR(result.pnl, 60.0, 1e-12);
    result = rival_simulate(prices, signals, equal, 4, 100.0);
    CHECK_NEAR(result.pnl, -50.0, 1e-12);
    const int position_signals[] = {1, 1, -1, -1};
    result = rival_simulate(prices, position_signals, fast, 4, 100.0);
    CHECK(result.trades == 2 && result.holding == 0);
    const int final_buy[] = {0, 0, 0, 1};
    result = rival_simulate(prices, final_buy, equal, 4, 100.0);
    CHECK(result.trades == 0 && result.holding == 0);
    rival_sweep_points(points, 25);
    CHECK_NEAR(points[0], 0.1, 1e-12);
    CHECK_NEAR(points[12], 1.0, 1e-12);
    CHECK_NEAR(points[24], 100.0, 1e-10);
    return CHECK_SUMMARY();
}
