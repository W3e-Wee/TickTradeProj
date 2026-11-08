/* Verify replay event order, bounded drain, and overflow accounting. */
#include "check.h"
#include "replay_events.h"

/* Exercise replay event ordering, draining, and queue overflow accounting. */
int main(void)
{
    ReplayEvent storage[3], drained[2], event;
    ReplayEventQueue queue;
    CHECK(replay_events_init(&queue, storage, 3));
    CHECK(replay_events_push(&queue, (ReplayEvent){1, 1, 10.0, 100}));
    CHECK(replay_events_push(&queue, (ReplayEvent){2, -1, 11.0, 200}));
    CHECK(replay_events_push(&queue, (ReplayEvent){3, 1, 12.0, 300}));
    CHECK(!replay_events_push(&queue, (ReplayEvent){4, -1, 13.0, 400}));
    CHECK(queue.dropped == 1);
    CHECK(replay_events_drain(&queue, drained, 2) == 2);
    CHECK(drained[0].trade_index == 1 && drained[1].trade_index == 2);
    CHECK(replay_events_pop(&queue, &event) && event.trade_index == 3);
    CHECK(!replay_events_pop(&queue, &event));
    return CHECK_SUMMARY();
}
