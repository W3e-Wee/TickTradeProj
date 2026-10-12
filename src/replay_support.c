/* Shared replay parsing, monotonic scheduling, latency aggregation, and
 * optional LED support used by replay executables. */
#include "timing.h"
#include "replay_support.h"
#include "led.h"

#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int replay_schedule_ns(uint64_t start_ns, int64_t first_timestamp_us,
                       int64_t timestamp_us, unsigned speed,
                       uint64_t *release_ns_out) {
    if (release_ns_out == NULL || speed == 0 || speed > 1000 ||
        first_timestamp_us < 0 || timestamp_us < first_timestamp_us) {
        return 0;
    }
    uint64_t delta_us = (uint64_t)(timestamp_us - first_timestamp_us);
    uint64_t divisor = (uint64_t)speed;
    uint64_t whole = delta_us / divisor;
    uint64_t remainder = delta_us % divisor;
    if (whole > UINT64_MAX / 1000U) {
        return 0;
    }
    uint64_t offset_ns = whole * 1000U + (remainder * 1000U) / divisor;
    if (offset_ns > UINT64_MAX - start_ns) {
        return 0;
    }
    *release_ns_out = start_ns + offset_ns;
    return 1;
}

int replay_within_duration_us(int64_t first_timestamp_us,
                              int64_t timestamp_us,
                              uint64_t duration_us) {
    return first_timestamp_us >= 0 && timestamp_us >= first_timestamp_us &&
           (uint64_t)(timestamp_us - first_timestamp_us) <= duration_us;
}

int replay_wait_until(uint64_t release_ns, ReplayNowFn now_fn,
                      ReplayWaitFn wait_fn, void *context,
                      const volatile sig_atomic_t *stop_flag) {
    if (now_fn == NULL || wait_fn == NULL) return 0;
    while (now_fn(context) < release_ns) {
        if (stop_flag != NULL && *stop_flag) return 0;
        wait_fn(release_ns, context);
    }
    return stop_flag == NULL || !*stop_flag;
}

int replay_latency_add(ReplayLatencySet *set, uint64_t release_ns,
                       uint64_t processing_start_ns, uint64_t decision_ns) {
    if (set == NULL || processing_start_ns > decision_ns || release_ns > decision_ns) {
        return 0;
    }
    uint64_t duration = decision_ns - processing_start_ns;
    /* Preserve deadline lateness separately from decision processing time. */
    uint64_t release_latency = decision_ns - release_ns;
    if (duration > UINT64_MAX - set->total_processing_ns ||
        release_latency > UINT64_MAX - set->total_release_latency_ns ||
        set->count == SIZE_MAX) {
        return 0;
    }
    if (set->count == set->capacity) {
        size_t next_capacity = set->capacity == 0 ? 4096 : set->capacity * 2;
        if (next_capacity < set->capacity || next_capacity > SIZE_MAX / sizeof *set->samples) {
            return 0;
        }
        ReplayLatencySample *grown = realloc(set->samples, next_capacity * sizeof *set->samples);
        if (grown == NULL) return 0;
        set->samples = grown;
        set->capacity = next_capacity;
    }
    set->samples[set->count++] = (ReplayLatencySample){
        .decision_ns = decision_ns,
        .processing_duration_ns = duration,
        .release_latency_ns = release_latency,
    };
    set->total_processing_ns += duration;
    set->total_release_latency_ns += release_latency;
    if (duration > set->worst_processing_ns) set->worst_processing_ns = duration;
    if (release_latency > set->worst_release_latency_ns) set->worst_release_latency_ns = release_latency;
    return 1;
}

void replay_latency_destroy(ReplayLatencySet *set) {
    if (set == NULL) return;
    free(set->samples);
    memset(set, 0, sizeof *set);
}

uint64_t replay_average_processing_ns(const ReplayLatencySet *set) {
    return set != NULL && set->count != 0 ? set->total_processing_ns / (uint64_t)set->count : 0;
}

uint64_t replay_average_release_latency_ns(const ReplayLatencySet *set) {
    return set != NULL && set->count != 0 ? set->total_release_latency_ns / (uint64_t)set->count : 0;
}

static int compare_u64(const void *left, const void *right) {
    uint64_t a = *(const uint64_t *)left;
    uint64_t b = *(const uint64_t *)right;
    return (a > b) - (a < b);
}

int replay_percentile_ns(const ReplayLatencySet *set, int release_latency,
                         unsigned percentile, uint64_t *value_out) {
    if (set == NULL || value_out == NULL || set->count == 0 || percentile == 0 || percentile > 100 ||
        set->count > SIZE_MAX / sizeof(uint64_t)) {
        return 0;
    }
    uint64_t *values = malloc(set->count * sizeof *values);
    if (values == NULL) return 0;
    for (size_t i = 0; i < set->count; ++i) {
        values[i] = release_latency ? set->samples[i].release_latency_ns
                                    : set->samples[i].processing_duration_ns;
    }
    qsort(values, set->count, sizeof *values, compare_u64);
    size_t quotient = set->count / 100U;
    size_t remainder = set->count % 100U;
    size_t rank = quotient * percentile + (remainder * percentile + 99U) / 100U;
    if (rank == 0) rank = 1;
    *value_out = values[rank - 1];
    free(values);
    return 1;
}

const char *replay_color_name(ReplayColor color) {
    if (color == REPLAY_COLOR_GREEN) return "GREEN";
    if (color == REPLAY_COLOR_RED) return "RED";
    return "OFF";
}

void replay_led_transition(ReplayColor current, int signal,
                           ReplayColor *next, ReplayColor *turn_off,
                           ReplayColor *turn_on) {
    *next = current;
    *turn_off = REPLAY_COLOR_NONE;
    *turn_on = REPLAY_COLOR_NONE;
    if (signal > 0) {
        *next = REPLAY_COLOR_GREEN;
        *turn_off = REPLAY_COLOR_RED;
        *turn_on = REPLAY_COLOR_GREEN;
    } else if (signal < 0) {
        *next = REPLAY_COLOR_RED;
        *turn_off = REPLAY_COLOR_GREEN;
        *turn_on = REPLAY_COLOR_RED;
    }
}

void replay_output_cleanup_transition(ReplayColor current,
                                     ReplayColor *next,
                                     ReplayColor *turn_off) {
    *next = REPLAY_COLOR_NONE;
    *turn_off = current;
}

int replay_output_init(ReplayOutput *output, ReplayOutputMode mode) {
    if (output == NULL) return 0;
    output->mode = mode;
    output->active_color = REPLAY_COLOR_NONE;
    output->initialized = 0;
    if (mode == REPLAY_OUTPUT_OFF || mode == REPLAY_OUTPUT_CONSOLE) {
        output->initialized = 1;
        return 1;
    }
#if defined(__linux__)
    led_init();
    output->initialized = 1;
    if (!led_hw) {
        led_cleanup();
        output->initialized = 0;
        fprintf(stderr, "Hardware LED mode unavailable: helper could not acquire Pi LEDs.\n");
        return 0;
    }
    return 1;
#else
    fprintf(stderr, "Hardware LED mode unavailable on this operating system.\n");
    return 0;
#endif
}

void replay_output_signal(ReplayOutput *output, int signal, uint64_t trade_number) {
    if (output == NULL || !output->initialized || signal == 0 || output->mode == REPLAY_OUTPUT_OFF) return;
    ReplayColor next, turn_off, turn_on;
    replay_led_transition(output->active_color, signal, &next, &turn_off, &turn_on);
    if (output->mode == REPLAY_OUTPUT_CONSOLE) {
        printf("LED|trade=%" PRIu64 "|%s=OFF|%s=ON\n", trade_number,
               replay_color_name(turn_off), replay_color_name(turn_on));
        fflush(stdout);
    } else {
#if defined(__linux__)
        if (turn_off == REPLAY_COLOR_GREEN) led_off(LED_GREEN);
        if (turn_off == REPLAY_COLOR_RED) led_off(LED_RED);
        if (turn_on == REPLAY_COLOR_GREEN) led_on(LED_GREEN);
        if (turn_on == REPLAY_COLOR_RED) led_on(LED_RED);
#endif
    }
    output->active_color = next;
}

void replay_output_cleanup(ReplayOutput *output) {
    if (output == NULL || !output->initialized) return;
    ReplayColor next, turn_off;
    replay_output_cleanup_transition(output->active_color, &next, &turn_off);
    if (output->mode == REPLAY_OUTPUT_CONSOLE && turn_off != REPLAY_COLOR_NONE) {
        printf("LED|cleanup|%s=OFF\n", replay_color_name(turn_off));
        fflush(stdout);
    } else if (output->mode == REPLAY_OUTPUT_HARDWARE) {
#if defined(__linux__)
        if (turn_off == REPLAY_COLOR_GREEN) led_off(LED_GREEN);
        if (turn_off == REPLAY_COLOR_RED) led_off(LED_RED);
        led_cleanup();
#endif
    }
    output->active_color = next;
    output->initialized = 0;
}
