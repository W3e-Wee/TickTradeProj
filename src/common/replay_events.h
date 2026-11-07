/* Fixed-capacity event queue for replay output outside the decision path. */
#ifndef TICKTRADE_REPLAY_EVENTS_H
#define TICKTRADE_REPLAY_EVENTS_H
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint64_t trade_index;
    int side;
    double price;
    uint64_t decision_ns;
} ReplayEvent;

typedef struct {
    ReplayEvent *items;
    size_t capacity;
    size_t head;
    size_t count;
    uint64_t dropped;
} ReplayEventQueue;

int replay_events_init(ReplayEventQueue *queue, ReplayEvent *storage, size_t capacity);
int replay_events_push(ReplayEventQueue *queue, ReplayEvent event);
int replay_events_pop(ReplayEventQueue *queue, ReplayEvent *event);
size_t replay_events_drain(ReplayEventQueue *queue, ReplayEvent *events,
                           size_t maximum);
#endif
