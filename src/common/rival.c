/* Position rules and reaction-time sweep used by rival_sweep. */
#include "rival.h"
#include <math.h>

/* Apply flat/holding rules; equal reaction times use the next available trade. */
RivalResult rival_simulate(const double *prices, const int *signals,
                           const uint64_t *processing_ns, size_t count,
                           double reaction_us)
{
    RivalResult result = {0.0, 0, 0};
    double entry = 0.0;
    if (prices == NULL || signals == NULL || processing_ns == NULL || reaction_us < 0.0) return result;
    for (size_t i = 0; i < count; ++i) {
        if (signals[i] == 0) continue;
        size_t fill = processing_ns[i] < reaction_us * 1000.0 ? i : i + 1;
        if (fill >= count) continue;
        if (signals[i] > 0 && !result.holding) { entry = prices[fill]; result.holding = 1; ++result.trades; }
        else if (signals[i] < 0 && result.holding) { result.pnl += prices[fill] - entry; result.holding = 0; ++result.trades; }
    }
    return result;
}

/* Create an inclusive two-decade log sweep with a shared exact 1us endpoint. */
void rival_sweep_points(double *points, size_t count)
{
    if (points == NULL || count == 0) return;
    if (count == 25) {
        for (size_t i = 0; i <= 12; ++i) points[i] = pow(10.0, -1.0 + (double)i / 12.0);
        for (size_t i = 13; i < count; ++i) points[i] = pow(10.0, (double)(i - 12) * 2.0 / 12.0);
    } else {
        for (size_t i = 0; i < count; ++i) points[i] = count == 1 ? 0.1 : pow(10.0, -1.0 + 3.0 * (double)i / (double)(count - 1));
    }
}
