/* Portable replay pacing built on the course-provided monotonic clock. */
#include "replay.h"
#include "timing.h"
#include <limits.h>

/* Calculate start + (trade timestamp - first timestamp) / speed in ns. */
int replay_release_ns(uint64_t start_ns, int64_t first_ts_us, int64_t trade_ts_us,
                      unsigned speed, uint64_t *release_ns)
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

/* Use coarse sleeps for large gaps and monotonic spinning for short ones. */
void replay_wait_until(uint64_t release_ns_value)
{
    for (;;) {
        uint64_t now = now_ns();
        if (now >= release_ns_value) return;
        uint64_t gap = release_ns_value - now;
        if (gap > UINT64_C(2000000)) {
            uint64_t sleep_for = gap / UINT64_C(1000000);
            sleep_ms(sleep_for > (uint64_t)INT_MAX ? INT_MAX : (int)sleep_for);
        }
    }
}
