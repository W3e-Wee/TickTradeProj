/* Check timestamp scaling, explicit offsets, lag and sleep/spin decisions. */
#include "check.h"
#include "replay.h"
/* Confirm one-second gaps become one millisecond at 1000x speed. */
int main(void)
{
    uint64_t release = 0;
    CHECK(replay_release_ns(100, 5000000, 6000000, 1000, &release));
    CHECK(release == 1000100);
    CHECK(replay_release_ns(100, 5000000, 6000000, 1, &release));
    CHECK(release == 1000000100ULL);
    CHECK(replay_release_ns(100, 5000000, 6000000, 100, &release));
    CHECK(release == 10000100ULL);
    CHECK(replay_release_ns(9000, 7000000, 8000000, 1000, &release));
    CHECK(release == 1009000ULL);
    CHECK(replay_lag_ns(release + 20, release) == 20);
    CHECK(replay_lag_ns(release, release + 20) == 0);
    CHECK(!replay_release_ns(0, 10, 9, 1, &release));
    CHECK(replay_sleep_budget_ns(3000000, 1000) == 1500000);
    CHECK(replay_sleep_budget_ns(1500000, 1000) == 0);
    CHECK(replay_wait_until(0, 1000, NULL, 0));
    return CHECK_SUMMARY();
}
