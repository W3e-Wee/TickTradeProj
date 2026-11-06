/* Timestamp release schedule, nonnegative lag, and hybrid wait strategy. */
#ifndef TICKTRADE_REPLAY_H
#define TICKTRADE_REPLAY_H
#include <stdint.h>
/* Convert an exchange timestamp to scheduled monotonic nanoseconds. */
int replay_release_ns(uint64_t start_ns, int64_t first_ts_us, int64_t trade_ts_us,
                      unsigned speed, uint64_t *release_ns);
/* Return positive lateness, or zero when execution is ahead of schedule. */
uint64_t replay_lag_ns(uint64_t now_ns, uint64_t release_ns);
/* Sleep for gaps above two milliseconds and spin for the final interval. */
void replay_wait_until(uint64_t release_ns);
#endif
