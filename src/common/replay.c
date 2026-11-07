/* Portable replay pacing built on the course-provided monotonic clock. */
#include "timing.h"
#include "replay.h"
#include "platform.h"
#include <limits.h>

/* Calculate start + (trade timestamp - first timestamp) / speed in ns. */
int replay_release_ns(uint64_t start_ns, int64_t first_ts_us, int64_t trade_ts_us,
                      uint64_t speed, uint64_t *release_ns)
{
    uint64_t delta_us, offset_ns;
    if (release_ns == NULL || speed == 0 || first_ts_us < 0 || trade_ts_us < first_ts_us) return 0;
    delta_us = (uint64_t)(trade_ts_us - first_ts_us);
    if (delta_us > UINT64_MAX / 1000U) return 0;
    offset_ns = delta_us * 1000U / speed;
    if (start_ns > UINT64_MAX - offset_ns) return 0;
    *release_ns = start_ns + offset_ns;
    return 1;
}

/* Clamp early execution to zero so the displayed lag never becomes negative. */
uint64_t replay_lag_ns(uint64_t now_ns_value, uint64_t release_ns_value)
{
    return now_ns_value > release_ns_value ? now_ns_value - release_ns_value : 0;
}

/* Reserve sleep granularity plus half a millisecond for the final spin. */
uint64_t replay_sleep_budget_ns(uint64_t gap_ns, uint64_t granularity_us)
{
    if (granularity_us > (UINT64_MAX / 1000U) - 500U) return 0;
    uint64_t guard_ns = (granularity_us + 500U) * 1000U;
    return gap_ns > guard_ns ? gap_ns - guard_ns : 0;
}

/* Sleep coarse gaps and use the monotonic helper clock to avoid early release. */
int replay_wait_until(uint64_t release_ns_value, uint64_t granularity_us,
                      const volatile sig_atomic_t *stop_requested,
                      uint64_t stop_deadline_ns)
{
    for (;;) {
        uint64_t now = now_ns();
        if ((stop_requested != NULL && *stop_requested) ||
            (stop_deadline_ns != 0 && now >= stop_deadline_ns)) return 0;
        if (now >= release_ns_value) return 1;
        uint64_t gap = release_ns_value - now;
        uint64_t budget = replay_sleep_budget_ns(gap, granularity_us);
        if (budget >= UINT64_C(1000000)) {
            uint64_t sleep_ms_value = budget / UINT64_C(1000000);
            unsigned bounded_ms = sleep_ms_value > (uint64_t)INT_MAX ? (unsigned)INT_MAX : (unsigned)sleep_ms_value;
            (void)plat_sleep_ms(bounded_ms);
        }
    }
}
