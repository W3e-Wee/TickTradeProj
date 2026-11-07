/* Bounded FIFO event queue; it allocates nothing and performs no I/O. */
#include "replay_events.h"

/* Attach caller-owned storage before replay begins. */
int replay_events_init(ReplayEventQueue *queue, ReplayEvent *storage, size_t capacity)
{
    if (queue == NULL || storage == NULL || capacity == 0) return 0;
    *queue = (ReplayEventQueue){storage, capacity, 0, 0, 0};
    return 1;
}

/* Append an event or record one dropped event when the ring is full. */
int replay_events_push(ReplayEventQueue *queue, ReplayEvent event)
{
    if (queue == NULL || queue->items == NULL || queue->capacity == 0) return 0;
    if (queue->count == queue->capacity) { ++queue->dropped; return 0; }
    size_t tail = (queue->head + queue->count) % queue->capacity;
    queue->items[tail] = event;
    ++queue->count;
    return 1;
}

/* Remove the oldest queued event. */
int replay_events_pop(ReplayEventQueue *queue, ReplayEvent *event)
{
    if (queue == NULL || event == NULL || queue->items == NULL || queue->count == 0) return 0;
    *event = queue->items[queue->head];
    queue->head = (queue->head + 1U) % queue->capacity;
    --queue->count;
    return 1;
}

/* Copy at most maximum queued events into the caller's drain buffer. */
size_t replay_events_drain(ReplayEventQueue *queue, ReplayEvent *events, size_t maximum)
{
    if (queue == NULL || events == NULL) return 0;
    size_t drained = 0;
    while (drained < maximum && replay_events_pop(queue, &events[drained])) ++drained;
    return drained;
}
