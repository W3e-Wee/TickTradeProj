/* Simplified position and P&L model for the coursework rival comparison. */
#ifndef TICKTRADE_RIVAL_H
#define TICKTRADE_RIVAL_H
#include <stddef.h>
#include <stdint.h>
typedef struct { double pnl; size_t trades; int holding; } RivalResult;
/* Execute signal fills at trigger or next price based on strict reaction time. */
RivalResult rival_simulate(const double *prices, const int *signals,
                           const uint64_t *processing_ns, size_t count,
                           double reaction_us);
/* Fill 25 log-spaced reaction thresholds including 0.1, 1, and 100 microseconds. */
void rival_sweep_points(double *points, size_t count);
#endif
