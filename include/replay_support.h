/* Shared replay timing, signal, and summary interfaces. */
#ifndef REPLAY_SUPPORT_H
#define REPLAY_SUPPORT_H

#include <signal.h>
#include <stddef.h>
#include <stdint.h>

typedef uint64_t (*ReplayNowFn)(void *context);
typedef void (*ReplayWaitFn)(uint64_t deadline_ns, void *context);

typedef struct {
    uint64_t decision_ns;
    uint64_t processing_duration_ns;
    uint64_t release_latency_ns;
} ReplayLatencySample;

typedef struct {
    ReplayLatencySample *samples;
    size_t count;
    size_t capacity;
    uint64_t total_processing_ns;
    uint64_t total_release_latency_ns;
    uint64_t worst_processing_ns;
    uint64_t worst_release_latency_ns;
} ReplayLatencySet;

typedef enum {
    REPLAY_OUTPUT_OFF = 0,
    REPLAY_OUTPUT_CONSOLE = 1,
    REPLAY_OUTPUT_HARDWARE = 2
} ReplayOutputMode;

typedef enum {
    REPLAY_COLOR_NONE = -1,
    REPLAY_COLOR_GREEN = 0,
    REPLAY_COLOR_RED = 1
} ReplayColor;

typedef struct {
    ReplayOutputMode mode;
    ReplayColor active_color;
    int initialized;
} ReplayOutput;

int replay_schedule_ns(uint64_t start_ns, int64_t first_timestamp_us,
                       int64_t timestamp_us, unsigned speed,
                       uint64_t *release_ns_out);
int replay_within_duration_us(int64_t first_timestamp_us,
                              int64_t timestamp_us,
                              uint64_t duration_us);
int replay_wait_until(uint64_t release_ns, ReplayNowFn now_fn,
                      ReplayWaitFn wait_fn, void *context,
                      const volatile sig_atomic_t *stop_flag);
int replay_latency_add(ReplayLatencySet *set, uint64_t release_ns,
                       uint64_t processing_start_ns, uint64_t decision_ns);
void replay_latency_destroy(ReplayLatencySet *set);
uint64_t replay_average_processing_ns(const ReplayLatencySet *set);
uint64_t replay_average_release_latency_ns(const ReplayLatencySet *set);
int replay_percentile_ns(const ReplayLatencySet *set, int release_latency,
                         unsigned percentile, uint64_t *value_out);
void replay_led_transition(ReplayColor current, int signal,
                           ReplayColor *next, ReplayColor *turn_off,
                           ReplayColor *turn_on);
void replay_output_cleanup_transition(ReplayColor current,
                                     ReplayColor *next,
                                     ReplayColor *turn_off);
int replay_output_init(ReplayOutput *output, ReplayOutputMode mode);
void replay_output_signal(ReplayOutput *output, int signal, uint64_t trade_number);
void replay_output_cleanup(ReplayOutput *output);
const char *replay_color_name(ReplayColor color);

#endif
