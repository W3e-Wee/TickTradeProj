/* Check timestamp release scaling and nonnegative replay lag. */
#include "check.h"
#include "replay.h"
/* Confirm one-second gaps become one millisecond at 1000x speed. */
int main(void)
{
    uint64_t release = 0;
    CHECK(replay_release_ns(100, 5000000, 6000000, 1000, &release));
    CHECK(release == 1000100);
    CHECK(replay_lag_ns(release + 20, release) == 20);
    CHECK(replay_lag_ns(release, release + 20) == 0);
    CHECK(!replay_release_ns(0, 10, 9, 1, &release));
    return CHECK_SUMMARY();
}
