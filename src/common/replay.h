/* Timestamp release schedule, nonnegative lag, and hybrid wait strategy. */
#ifndef TICKTRADE_REPLAY_H
#define TICKTRADE_REPLAY_H
#include <stdint.h>
#include <signal.h>
/* Convert an exchange timestamp to scheduled monotonic nanoseconds. */
int replay_release_ns(uint64_t start_ns, int64_t first_ts_us, int64_t trade_ts_us,
                      uint64_t speed, uint64_t *release_ns);
/* Return positive lateness, or zero when execution is ahead of schedule. */
uint64_t replay_lag_ns(uint64_t now_ns, uint64_t release_ns);
/* Compute the sleep portion while retaining granularity + 500us for spin. */
uint64_t replay_sleep_budget_ns(uint64_t gap_ns, uint64_t granularity_us);
/* Sleep for coarse portions, spin to release, and stop promptly on interruption. */
int replay_wait_until(uint64_t release_ns, uint64_t granularity_us,
                      const volatile sig_atomic_t *stop_requested,
                      uint64_t stop_deadline_ns);
#endif
