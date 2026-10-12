/* C unit tests for replay scheduling, latency accounting, and summary helpers. */
#include "timing.h"
#include "replay_support.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    uint64_t now;
    uint64_t deadlines[8];
    size_t wait_count;
    uint64_t overshoot_ns;
} FakeClock;

static uint64_t fake_now(void *context) {
    return ((FakeClock *)context)->now;
}

static void fake_wait(uint64_t deadline, void *context) {
    FakeClock *clock = context;
    assert(clock->wait_count < sizeof clock->deadlines / sizeof clock->deadlines[0]);
    clock->deadlines[clock->wait_count++] = deadline;
    clock->now = deadline + clock->overshoot_ns;
}

static void test_scheduling_speeds_and_equal_timestamps(void) {
    uint64_t release = 0;
    const int64_t first = INT64_C(1787293073000000);
    assert(replay_schedule_ns(UINT64_C(5000000000), first, first, 1, &release));
    assert(release == UINT64_C(5000000000));
    assert(replay_schedule_ns(UINT64_C(5000000000), first, first + 2000000, 1, &release));
    assert(release == UINT64_C(7000000000));
    assert(replay_schedule_ns(UINT64_C(5000000000), first, first + 2000000, 100, &release));
    assert(release == UINT64_C(5020000000));
    assert(replay_schedule_ns(UINT64_C(5000000000), first, first + 2000000, 1000, &release));
    assert(release == UINT64_C(5002000000));
    assert(replay_schedule_ns(UINT64_C(5000000000), first, first, 1000, &release));
    assert(release == UINT64_C(5000000000));
    assert(!replay_schedule_ns(0, first, first - 1, 1, &release));
    assert(!replay_schedule_ns(UINT64_MAX - 5, 0, 1, 1, &release));
}

static void test_wait_uses_absolute_deadlines_and_preserves_late_trade(void) {
    FakeClock clock = {.now = UINT64_C(10000000000), .overshoot_ns = 100};
    uint64_t first_due = 0, second_due = 0, third_due = 0;
    assert(replay_schedule_ns(UINT64_C(10000000000), 1000000, 1000000, 1, &first_due));
    assert(replay_schedule_ns(UINT64_C(10000000000), 1000000, 1001000, 1, &second_due));
    assert(replay_schedule_ns(UINT64_C(10000000000), 1000000, 1002000, 1, &third_due));
    assert(replay_wait_until(first_due, fake_now, fake_wait, &clock, NULL));
    clock.now = second_due - UINT64_C(5000000);
    assert(replay_wait_until(second_due, fake_now, fake_wait, &clock, NULL));
    assert(replay_wait_until(third_due, fake_now, fake_wait, &clock, NULL));
    assert(clock.wait_count == 2);
    assert(clock.deadlines[0] == second_due);
    assert(clock.deadlines[1] == third_due);
    assert(third_due - second_due == UINT64_C(1000000));
    clock.now = third_due + UINT64_C(2500000);
    size_t waits_before_late = clock.wait_count;
    assert(replay_wait_until(third_due, fake_now, fake_wait, &clock, NULL));
    assert(clock.wait_count == waits_before_late);
}

static void test_duration_limit(void) {
    assert(replay_within_duration_us(100, 100, 0));
    assert(replay_within_duration_us(100, 200, 100));
    assert(!replay_within_duration_us(100, 201, 100));
    assert(!replay_within_duration_us(100, 99, 100));
}

static void test_latency_and_nearest_rank_percentiles(void) {
    ReplayLatencySet empty = {0};
    uint64_t percentile = 0;
    assert(!replay_percentile_ns(&empty, 1, 50, &percentile));
    assert(replay_latency_add(&empty, 10, 11, 15));
    assert(replay_percentile_ns(&empty, 1, 50, &percentile) && percentile == 5);
    assert(replay_percentile_ns(&empty, 1, 99, &percentile) && percentile == 5);
    replay_latency_destroy(&empty);

    ReplayLatencySet unsorted = {0};
    assert(replay_latency_add(&unsorted, 0, 0, 9));
    assert(replay_latency_add(&unsorted, 0, 0, 1));
    assert(replay_latency_add(&unsorted, 0, 0, 5));
    assert(replay_percentile_ns(&unsorted, 1, 50, &percentile) && percentile == 5);
    assert(replay_percentile_ns(&unsorted, 1, 99, &percentile) && percentile == 9);
    replay_latency_destroy(&unsorted);

    ReplayLatencySet set = {0};
    for (uint64_t i = 1; i <= 100; ++i) {
        uint64_t release = i * 1000;
        assert(replay_latency_add(&set, release, release + i, release + i + i * 2));
    }
    assert(set.count == 100);
    assert(replay_average_processing_ns(&set) == 101);
    assert(replay_average_release_latency_ns(&set) == 151);
    assert(set.worst_processing_ns == 200);
    assert(set.worst_release_latency_ns == 300);
    uint64_t p50 = 0, p99 = 0;
    assert(replay_percentile_ns(&set, 0, 50, &p50));
    assert(replay_percentile_ns(&set, 0, 99, &p99));
    assert(p50 == 100);
    assert(p99 == 198);
    assert(replay_percentile_ns(&set, 1, 50, &p50));
    assert(replay_percentile_ns(&set, 1, 99, &p99));
    assert(p50 == 150);
    assert(p99 == 297);
    replay_latency_destroy(&set);
    assert(set.count == 0 && set.samples == NULL);
}

static void test_led_transitions_and_cleanup(void) {
    ReplayColor next, off, on;
    replay_led_transition(REPLAY_COLOR_NONE, 1, &next, &off, &on);
    assert(next == REPLAY_COLOR_GREEN && off == REPLAY_COLOR_RED && on == REPLAY_COLOR_GREEN);
    replay_led_transition(next, -1, &next, &off, &on);
    assert(next == REPLAY_COLOR_RED && off == REPLAY_COLOR_GREEN && on == REPLAY_COLOR_RED);
    replay_led_transition(next, 0, &next, &off, &on);
    assert(next == REPLAY_COLOR_RED && off == REPLAY_COLOR_NONE && on == REPLAY_COLOR_NONE);
    replay_output_cleanup_transition(next, &next, &off);
    assert(next == REPLAY_COLOR_NONE && off == REPLAY_COLOR_RED);
}

int main(void) {
    test_scheduling_speeds_and_equal_timestamps();
    test_wait_uses_absolute_deadlines_and_preserves_late_trade();
    test_duration_limit();
    test_latency_and_nearest_rank_percentiles();
    test_led_transitions_and_cleanup();
    puts("replay support tests: PASS");
    return 0;
}
