/* Timestamp-paced replay: load trades, wait on release deadlines, calculate
 * signals, and optionally write timing and decision evidence. */
#include "timing.h"
#include "market.h"
#include "replay_support.h"
#include "trade_io.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_WIN32)
#include <sched.h>
#endif

#ifndef REPLAY_VERSION_ID
#define REPLAY_VERSION_ID "replay-unknown"
#endif

#ifndef READER_KIND
#define READER_KIND 2
#endif
#ifndef SMA_KIND
#define SMA_KIND 1
#endif
#ifndef FIXED_PRICE
#define FIXED_PRICE 0
#endif
#ifndef STORAGE_KIND
#define STORAGE_KIND 0
#endif
#ifndef FIXED_TRADE_CAPACITY
#define FIXED_TRADE_CAPACITY 12000000U
#endif

#define REPLAY_LINE_LENGTH 512
#define REPLAY_MAX_WINDOW 100000
#define REPLAY_PRICE_SCALE INT64_C(100000000)

typedef struct {
    int64_t id;
    int64_t timestamp_us;
    double price;
    int64_t price_units;
    double qty;
    int is_buy;
} ReplayTrade;

typedef struct {
    FILE *file;
    int helper_open;
} ReplayReader;

typedef enum {
    SIGNAL_POLICY_PRIMARY = 0,
    SIGNAL_POLICY_LEGACY_HYBRID_SHADOW = 1
} SignalPolicy;

static volatile sig_atomic_t replay_stop_requested = 0;

#if STORAGE_KIND == 1
static Trade replay_fixed_array[FIXED_TRADE_CAPACITY];
#endif

static void replay_signal_handler(int signal_number) {
    (void)signal_number;
    replay_stop_requested = 1;
}

static uint64_t real_now(void *context) {
    (void)context;
    return now_ns();
}

static void real_wait(uint64_t deadline_ns, void *context) {
    (void)context;
    uint64_t now = now_ns();
    if (now >= deadline_ns) return;
    uint64_t remaining = deadline_ns - now;
    if (remaining > UINT64_C(3000000)) {
        uint64_t sleep_for_ms = remaining / UINT64_C(1000000) - 1U;
        if (sleep_for_ms > 25U) sleep_for_ms = 25U;
        sleep_ms((int)sleep_for_ms);
    } else {
#if defined(_WIN32)
        SwitchToThread();
#else
        sched_yield();
#endif
    }
}

#if READER_KIND != 0
static int parse_bool(const char *text, int *is_buy) {
    if (strcmp(text, "False") == 0 || strcmp(text, "false") == 0) {
        *is_buy = 1;
        return 1;
    }
    if (strcmp(text, "True") == 0 || strcmp(text, "true") == 0) {
        *is_buy = 0;
        return 1;
    }
    return 0;
}
#endif

#if READER_KIND != 0
static void trim_line(char *line) {
    size_t length = strlen(line);
    while (length != 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) {
        line[--length] = '\0';
    }
}

static int skip_header_or_blank(const char *line) {
    while (isspace((unsigned char)*line)) ++line;
    return *line == '\0' || strncmp(line, "trade_id", 8) == 0;
}

#if READER_KIND == 1 || READER_KIND == 2
static int split_csv_fields(char *line, char **fields) {
    size_t count = 0;
    char *cursor = line;
    for (;;) {
        if (count == 7) return 0;
        fields[count++] = cursor;
        char *comma = strchr(cursor, ',');
        if (comma == NULL) break;
        *comma = '\0';
        cursor = comma + 1;
    }
    return count == 7;
}
#endif
#endif

#if READER_KIND == 1 || READER_KIND == 3
static int parse_integer_strict(const char *text, int64_t *result) {
    char *end = NULL;
    errno = 0;
    int64_t value = (int64_t)strtoll(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0') return 0;
    *result = value;
    return 1;
}

static int parse_double_strict(const char *text, double *result) {
    char *end = NULL;
    errno = 0;
    double value = strtod(text, &end);
    if (errno != 0 || end == text || *end != '\0' || !isfinite(value)) return 0;
    *result = value;
    return 1;
}
#endif

#if READER_KIND == 2
static int parse_unsigned_digits(const char **cursor, uint64_t *value_out) {
    const char *p = *cursor;
    if (!isdigit((unsigned char)*p)) return 0;
    uint64_t value = 0;
    do {
        unsigned digit = (unsigned)(*p - '0');
        if (value > (UINT64_MAX - digit) / 10U) return 0;
        value = value * 10U + digit;
        ++p;
    } while (isdigit((unsigned char)*p));
    *cursor = p;
    *value_out = value;
    return 1;
}

static int parse_fast_integer(const char *text, int64_t *value_out) {
    const char *cursor = text;
    uint64_t value = 0;
    if (!parse_unsigned_digits(&cursor, &value) || *cursor != '\0' || value > (uint64_t)LLONG_MAX) return 0;
    *value_out = (int64_t)value;
    return 1;
}

/* The manual replay parser uses plain CSV fields and checked scale-1e8
 * conversion for fixed-point versions. */
static int parse_fast_decimal(const char *text, double *double_out,
                              int64_t *units_out, int fixed_precision) {
    const char *cursor = text;
    uint64_t whole = 0;
    if (!parse_unsigned_digits(&cursor, &whole)) return 0;
    long double fraction = 0.0L;
    long double place = 0.1L;
    uint64_t fraction_units = 0;
    unsigned stored_digits = 0;
    unsigned digits = 0;
    if (*cursor == '.') {
        ++cursor;
        if (!isdigit((unsigned char)*cursor)) return 0;
        while (isdigit((unsigned char)*cursor)) {
            unsigned digit = (unsigned)(*cursor - '0');
            fraction += (long double)digit * place;
            place *= 0.1L;
            ++digits;
            if (stored_digits < 8) {
                fraction_units = fraction_units * 10U + digit;
                ++stored_digits;
            } else if (fixed_precision && digit != 0) {
                return 0;
            }
            if (digits > 18) return 0;
            ++cursor;
        }
    }
    if (*cursor != '\0' || whole > (uint64_t)INT64_MAX / (uint64_t)REPLAY_PRICE_SCALE) return 0;
    while (stored_digits < 8) {
        fraction_units *= 10U;
        ++stored_digits;
    }
    uint64_t units = whole * (uint64_t)REPLAY_PRICE_SCALE + fraction_units;
    if (units > (uint64_t)INT64_MAX) return 0;
    long double parsed = (long double)whole + fraction;
    if (!isfinite((double)parsed)) return 0;
    *double_out = (double)parsed;
    if (units_out != NULL) *units_out = (int64_t)units;
    return 1;
}
#endif

#if READER_KIND == 1 || READER_KIND == 2
static int parse_custom_line(char *line, ReplayTrade *trade) {
    char *fields[7];
    if (!split_csv_fields(line, fields)) return 0;
    double quote_qty = 0.0;
    int best_match = 0;
#if READER_KIND == 1
    int ok = parse_integer_strict(fields[0], &trade->id) &&
             parse_double_strict(fields[1], &trade->price) &&
             parse_double_strict(fields[2], &trade->qty) &&
             parse_double_strict(fields[3], &quote_qty) &&
             parse_integer_strict(fields[4], &trade->timestamp_us) &&
             parse_bool(fields[5], &trade->is_buy) && parse_bool(fields[6], &best_match);
#else
    int ok = parse_fast_integer(fields[0], &trade->id) &&
             parse_fast_decimal(fields[1], &trade->price,
                                FIXED_PRICE ? &trade->price_units : NULL, FIXED_PRICE) &&
             parse_fast_decimal(fields[2], &trade->qty, NULL, 0) &&
             parse_fast_decimal(fields[3], &quote_qty, NULL, 0) &&
             parse_fast_integer(fields[4], &trade->timestamp_us) &&
             parse_bool(fields[5], &trade->is_buy) && parse_bool(fields[6], &best_match);
#endif
    return ok && trade->id >= 0 && trade->timestamp_us >= 0 && trade->price > 0.0 &&
           trade->qty > 0.0 && quote_qty >= 0.0;
}
#endif

#if READER_KIND == 3
static int parse_baseline_line(char *line, ReplayTrade *trade) {
    char *fields[7] = {0};
    size_t field_count = 0;
    char *token = strtok(line, ",");
    while (token != NULL && field_count < 7) {
        fields[field_count++] = token;
        token = strtok(NULL, ",");
    }
    double quote = 0.0;
    int best_match = 0;
    return field_count == 7 && parse_integer_strict(fields[0], &trade->id) &&
           parse_double_strict(fields[1], &trade->price) &&
           parse_double_strict(fields[2], &trade->qty) &&
           parse_double_strict(fields[3], &quote) &&
           parse_integer_strict(fields[4], &trade->timestamp_us) &&
           parse_bool(fields[5], &trade->is_buy) && parse_bool(fields[6], &best_match) &&
           trade->id >= 0 && trade->timestamp_us >= 0 && trade->price > 0.0 &&
           trade->qty > 0.0 && quote >= 0.0;
}
#endif

static int reader_open(ReplayReader *reader, const char *path) {
#if READER_KIND == 0
    reader->helper_open = open_trades(path);
    return reader->helper_open;
#else
    reader->file = fopen(path, "r");
    if (reader->file == NULL) perror(path);
    return reader->file != NULL;
#endif
}

static int reader_next(ReplayReader *reader, ReplayTrade *trade) {
#if READER_KIND == 0
    (void)reader;
    if (!next_trade()) return 0;
    trade->id = trade_id();
    trade->timestamp_us = trade_time();
    trade->price = trade_price();
    trade->qty = trade_qty();
    trade->is_buy = trade_is_buy();
#if FIXED_PRICE
    long double units = (long double)trade->price * (long double)REPLAY_PRICE_SCALE;
    if (units > (long double)INT64_MAX || units < 0.0L || fabsl(units - roundl(units)) > 0.0001L) return -1;
    trade->price_units = (int64_t)roundl(units);
#endif
    return 1;
#else
    char line[REPLAY_LINE_LENGTH];
    while (fgets(line, sizeof line, reader->file) != NULL) {
        size_t length = strlen(line);
        if (length == sizeof line - 1 && line[length - 1] != '\n' && !feof(reader->file)) return -1;
        trim_line(line);
        if (skip_header_or_blank(line)) continue;
#if READER_KIND == 1
    if (!parse_custom_line(line, trade)) return -1;
#elif READER_KIND == 2
    if (!parse_custom_line(line, trade)) return -1;
#elif READER_KIND == 3
        if (!parse_baseline_line(line, trade)) return -1;
#else
#error Unsupported READER_KIND for replay engine
#endif
        return 1;
    }
    return ferror(reader->file) ? -1 : 0;
#endif
}

static int reader_close(ReplayReader *reader) {
#if READER_KIND == 0
    if (reader->helper_open) close_trades();
    reader->helper_open = 0;
    return 1;
#else
    if (reader->file == NULL) return 1;
    int ok = fclose(reader->file) == 0;
    reader->file = NULL;
    return ok;
#endif
}

#if STORAGE_KIND != 0
static Trade to_trade(const ReplayTrade *source) {
    Trade result = {0};
    result.trade_id = source->id;
    result.price = source->price;
    result.qty = source->qty;
    result.quote_qty = source->price * source->qty;
    result.timestamp_us = source->timestamp_us;
    result.is_buy = source->is_buy;
    return result;
}
#endif

#if STORAGE_KIND == 3
static int grow_trade_array(Trade **trades, size_t *count, size_t *capacity,
                            size_t max_capacity, const ReplayTrade *row) {
    if (*count >= max_capacity) return 0;
    if (*count == *capacity) {
        size_t next_capacity = *capacity == 0 ? 4096 : *capacity * 2;
        if (next_capacity < *capacity || next_capacity > max_capacity) next_capacity = max_capacity;
        if (next_capacity <= *capacity || next_capacity > SIZE_MAX / sizeof **trades) return 0;
        Trade *grown = realloc(*trades, next_capacity * sizeof **trades);
        if (grown == NULL) return 0;
        *trades = grown;
        *capacity = next_capacity;
    }
    (*trades)[(*count)++] = to_trade(row);
    return 1;
}
#endif

static int load_replay_storage(const char *path, ReplayTrade *first_stream_row,
                               int *has_first_stream_row, Trade **heap_trades,
                               size_t *trade_count, size_t max_capacity,
                               ReplayReader *stream_reader,
                               uint64_t *preload_ns, uint64_t *counting_ns,
                               uint64_t *loading_ns, char *error, size_t error_size) {
    *heap_trades = NULL;
    *trade_count = 0;
    *has_first_stream_row = 0;
    *counting_ns = 0;
    *loading_ns = 0;
#if STORAGE_KIND == 0
    (void)max_capacity;
    if (!reader_open(stream_reader, path)) {
        snprintf(error, error_size, "cannot open replay input");
        return 0;
    }
    uint64_t first_read = now_ns();
    int status = reader_next(stream_reader, first_stream_row);
    *preload_ns = now_ns() - first_read;
    if (status < 0) {
        reader_close(stream_reader);
        snprintf(error, error_size, "invalid or malformed first trade");
        return 0;
    }
    *has_first_stream_row = status > 0;
    if (*has_first_stream_row) *trade_count = 1;
    *loading_ns = *preload_ns;
    return 1;
#elif STORAGE_KIND == 1 || STORAGE_KIND == 2 || STORAGE_KIND == 3
    (void)first_stream_row;
    (void)stream_reader;
    uint64_t total_started = now_ns();
    size_t count = 0;
    size_t capacity = 0;
    Trade *trades = NULL;
#if STORAGE_KIND == 1
    (void)capacity;
    (void)trades;
    if (max_capacity > (size_t)FIXED_TRADE_CAPACITY || max_capacity > SIZE_MAX / sizeof replay_fixed_array[0]) {
        snprintf(error, error_size, "runtime capacity exceeds compiled global array");
        return 0;
    }
#endif
#if STORAGE_KIND == 2
    uint64_t count_started = now_ns();
    ReplayReader counter = {0};
    if (!reader_open(&counter, path)) {
        snprintf(error, error_size, "cannot open counting pass");
        return 0;
    }
    ReplayTrade counter_row;
    int counter_status;
    while ((counter_status = reader_next(&counter, &counter_row)) > 0) {
        if (count == SIZE_MAX) {
            reader_close(&counter);
            snprintf(error, error_size, "trade count overflow");
            return 0;
        }
        ++count;
    }
    int counter_closed = reader_close(&counter);
    *counting_ns = now_ns() - count_started;
    if (counter_status < 0 || !counter_closed) {
        snprintf(error, error_size, "malformed input during counting pass");
        return 0;
    }
    if (count > max_capacity || count > SIZE_MAX / sizeof *trades) {
        snprintf(error, error_size, "exact allocation capacity/size exceeded");
        return 0;
    }
    trades = count == 0 ? NULL : malloc(count * sizeof *trades);
    if (count != 0 && trades == NULL) {
        snprintf(error, error_size, "exact trade allocation failed");
        return 0;
    }
    capacity = count;
    count = 0;
#endif

    uint64_t load_started = now_ns();
    ReplayReader reader = {0};
    if (!reader_open(&reader, path)) {
        free(trades);
        snprintf(error, error_size, "cannot open replay loading pass");
        return 0;
    }
    ReplayTrade row;
    int status;
    while ((status = reader_next(&reader, &row)) > 0) {
        if (count >= max_capacity) {
            reader_close(&reader);
            free(trades);
            snprintf(error, error_size, "trade array capacity exceeded");
            return 0;
        }
#if STORAGE_KIND == 1
        replay_fixed_array[count++] = to_trade(&row);
#elif STORAGE_KIND == 2
        if (count >= capacity) {
            reader_close(&reader);
            free(trades);
            snprintf(error, error_size, "dataset changed after exact counting pass");
            return 0;
        }
        trades[count++] = to_trade(&row);
#else
        if (!grow_trade_array(&trades, &count, &capacity, max_capacity, &row)) {
            reader_close(&reader);
            free(trades);
            snprintf(error, error_size, "whole-file dynamic allocation/capacity failed");
            return 0;
        }
#endif
    }
    int closed = reader_close(&reader);
    *loading_ns = now_ns() - load_started;
    if (status < 0 || !closed) {
        free(trades);
        snprintf(error, error_size, "malformed input during loading pass");
        return 0;
    }
#if STORAGE_KIND == 2
    if (count != capacity) {
        free(trades);
        snprintf(error, error_size, "dataset changed after counting pass");
        return 0;
    }
#endif
    *trade_count = count;
#if STORAGE_KIND == 2 || STORAGE_KIND == 3
    *heap_trades = trades;
#endif
    *preload_ns = now_ns() - total_started;
    return 1;
#else
#error Unsupported STORAGE_KIND for replay engine
#endif
}

#if STORAGE_KIND != 0
static Trade replay_trade_at(size_t index, const Trade *heap_trades) {
#if STORAGE_KIND == 1
    (void)heap_trades;
    return replay_fixed_array[index];
#elif STORAGE_KIND == 2 || STORAGE_KIND == 3
    return heap_trades[index];
#else
    (void)index;
    (void)heap_trades;
    return (Trade){0};
#endif
}
#endif

#if FIXED_PRICE
static int compare_fixed_average(int64_t short_sum, int short_window,
                                 int64_t long_sum, int long_window) {
    int64_t short_whole = short_sum / short_window;
    int64_t long_whole = long_sum / long_window;
    if (short_whole != long_whole) return (short_whole > long_whole) - (short_whole < long_whole);
    int64_t short_remainder = short_sum % short_window;
    int64_t long_remainder = long_sum % long_window;
    int64_t short_scaled = short_remainder * long_window;
    int64_t long_scaled = long_remainder * short_window;
    return (short_scaled > long_scaled) - (short_scaled < long_scaled);
}
#endif

static int crossed(int previous, int current, int ready) {
    if (!ready) return 0;
    if (previous <= 0 && current > 0) return 1;
    if (previous >= 0 && current < 0) return -1;
    return 0;
}

static int parse_unsigned_option(const char *name, const char *text,
                                 uint64_t maximum, uint64_t *out) {
    char *end = NULL;
    errno = 0;
    unsigned long long value = strtoull(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || value > maximum) {
        fprintf(stderr, "Invalid %s: %s\n", name, text);
        return 0;
    }
    *out = (uint64_t)value;
    return 1;
}

int main(int argc, char **argv) {
    if (argc == 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        printf("Usage: %s <csv> [options]\n", argv[0]);
        puts("Options:");
        puts("  --version-id ID              Label the selected replay version");
        puts("  --speed 1|100|1000           Replay speed (default: 1)");
        puts("  --short N                    Short SMA window (default: 3)");
        puts("  --long N                     Long SMA window (default: 5)");
        puts("  --duration-seconds N         Limit market-time duration from first trade");
        puts("  --limit N                    Limit processed records");
        puts("  --capacity N                 Maximum stored trades for array variants");
        puts("  --output off|console|hardware Signal output mode (default: off)");
        puts("  --signal-policy primary|legacy-hybrid-shadow");
        puts("  --latency-csv PATH           Write per-trade timing samples");
        puts("  --decision-csv PATH          Write per-trade strategy decisions");
        puts("  --start-at-ns N              Shared monotonic replay start epoch (race use)");
        puts("  --race-progress              Emit post-decision telemetry for the race launcher");
        return 0;
    }
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <csv> [--help]\n", argv[0]);
        return 2;
    }
    const char *path = argv[1];
    const char *version_id = REPLAY_VERSION_ID;
    const char *latency_path = NULL;
    const char *decision_path = NULL;
    unsigned speed = 1;
    int short_window = 3;
    int long_window = 5;
    uint64_t duration_seconds = 0;
    int duration_enabled = 0;
    uint64_t record_limit = 0;
    uint64_t shared_start_ns = 0;
    int race_progress = 0;
    size_t runtime_capacity = (size_t)FIXED_TRADE_CAPACITY;
    ReplayOutputMode output_mode = REPLAY_OUTPUT_OFF;
    SignalPolicy policy = SIGNAL_POLICY_PRIMARY;

    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "--speed") == 0 && i + 1 < argc) {
            uint64_t parsed = 0;
            if (!parse_unsigned_option("speed", argv[++i], 1000, &parsed) ||
                !(parsed == 1 || parsed == 100 || parsed == 1000)) return 2;
            speed = (unsigned)parsed;
        } else if (strcmp(argv[i], "--version-id") == 0 && i + 1 < argc) {
            version_id = argv[++i];
        } else if (strcmp(argv[i], "--short") == 0 && i + 1 < argc) {
            uint64_t parsed = 0;
            if (!parse_unsigned_option("short window", argv[++i], REPLAY_MAX_WINDOW, &parsed) || parsed == 0) return 2;
            short_window = (int)parsed;
        } else if (strcmp(argv[i], "--long") == 0 && i + 1 < argc) {
            uint64_t parsed = 0;
            if (!parse_unsigned_option("long window", argv[++i], REPLAY_MAX_WINDOW, &parsed) || parsed == 0) return 2;
            long_window = (int)parsed;
        } else if (strcmp(argv[i], "--duration-seconds") == 0 && i + 1 < argc) {
            if (!parse_unsigned_option("duration", argv[++i], UINT64_MAX / 1000000U, &duration_seconds)) return 2;
            duration_enabled = 1;
        } else if (strcmp(argv[i], "--limit") == 0 && i + 1 < argc) {
            if (!parse_unsigned_option("record limit", argv[++i], SIZE_MAX, &record_limit)) return 2;
        } else if (strcmp(argv[i], "--capacity") == 0 && i + 1 < argc) {
            uint64_t parsed = 0;
            if (!parse_unsigned_option("capacity", argv[++i], SIZE_MAX, &parsed)) return 2;
            runtime_capacity = (size_t)parsed;
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            const char *mode = argv[++i];
            if (strcmp(mode, "off") == 0) output_mode = REPLAY_OUTPUT_OFF;
            else if (strcmp(mode, "console") == 0) output_mode = REPLAY_OUTPUT_CONSOLE;
            else if (strcmp(mode, "hardware") == 0) output_mode = REPLAY_OUTPUT_HARDWARE;
            else { fprintf(stderr, "Invalid output mode: %s\n", mode); return 2; }
        } else if (strcmp(argv[i], "--signal-policy") == 0 && i + 1 < argc) {
            const char *value = argv[++i];
            if (strcmp(value, "primary") == 0) policy = SIGNAL_POLICY_PRIMARY;
            else if (strcmp(value, "legacy-hybrid-shadow") == 0) policy = SIGNAL_POLICY_LEGACY_HYBRID_SHADOW;
            else { fprintf(stderr, "Invalid signal policy: %s\n", value); return 2; }
        } else if (strcmp(argv[i], "--latency-csv") == 0 && i + 1 < argc) {
            latency_path = argv[++i];
        } else if (strcmp(argv[i], "--decision-csv") == 0 && i + 1 < argc) {
            decision_path = argv[++i];
        } else if (strcmp(argv[i], "--start-at-ns") == 0 && i + 1 < argc) {
            if (!parse_unsigned_option("start epoch", argv[++i], UINT64_MAX, &shared_start_ns)) return 2;
        } else if (strcmp(argv[i], "--race-progress") == 0) {
            race_progress = 1;
        } else {
            fprintf(stderr, "Unknown replay option: %s\n", argv[i]);
            return 2;
        }
    }
    if (short_window > long_window) {
        fprintf(stderr, "short window must not exceed long window.\n");
        return 2;
    }

#if defined(_WIN32)
    if (output_mode == REPLAY_OUTPUT_HARDWARE) {
        fprintf(stderr, "Hardware LED mode unavailable on Windows.\n");
        return 4;
    }
#endif
    ReplayOutput output = {0};
    ReplayLatencySet latency = {0};
    Trade *heap_trades = NULL;
    ReplayReader stream_reader = {0};
    ReplayTrade first_stream_row = {0};
    int has_first_stream_row = 0;
    size_t trade_count = 0;
    uint64_t preload_ns = 0, counting_ns = 0, loading_ns = 0;
    char error[256] = {0};
    double *double_ring = calloc((size_t)long_window, sizeof *double_ring);
    int64_t *fixed_ring = NULL;
#if FIXED_PRICE
    fixed_ring = calloc((size_t)long_window, sizeof *fixed_ring);
#endif
    if (double_ring == NULL
#if FIXED_PRICE
        || fixed_ring == NULL
#endif
    ) {
        fprintf(stderr, "Replay ring allocation failed.\n");
        free(double_ring);
        free(fixed_ring);
        return 3;
    }
    if (!load_replay_storage(path, &first_stream_row, &has_first_stream_row,
                             &heap_trades, &trade_count, runtime_capacity,
                             &stream_reader,
                             &preload_ns, &counting_ns, &loading_ns, error, sizeof error)) {
        fprintf(stderr, "Replay preload failed: %s\n", error);
        free(double_ring);
        free(fixed_ring);
        free(heap_trades);
        return 4;
    }

    if (output_mode == REPLAY_OUTPUT_HARDWARE && !replay_output_init(&output, output_mode)) {
        free(double_ring);
        free(fixed_ring);
        free(heap_trades);
        return 4;
    }
    if (output_mode != REPLAY_OUTPUT_HARDWARE && !replay_output_init(&output, output_mode)) {
        free(double_ring);
        free(fixed_ring);
        free(heap_trades);
        return 4;
    }
    FILE *decision_file = NULL;
    if (decision_path != NULL) {
        decision_file = fopen(decision_path, "w");
        if (decision_file == NULL) {
            perror(decision_path);
            replay_output_cleanup(&output);
            free(double_ring);
            free(fixed_ring);
            free(heap_trades);
            return 4;
        }
        fputs("trade,timestamp_us,primary_short_sma,primary_long_sma,primary_decision,shadow_short_sma,shadow_long_sma,shadow_decision,effective_decision,policy\n", decision_file);
    }

    replay_stop_requested = 0;
    void (*old_int_handler)(int) = signal(SIGINT, replay_signal_handler);
    void (*old_term_handler)(int) = signal(SIGTERM, replay_signal_handler);
    uint64_t start_ns = now_ns();
    int64_t first_timestamp_us = 0;
    int first_timestamp_ready = 0;
    int have_previous_timestamp = 0;
    int64_t previous_timestamp = 0;
    uint64_t duration_us = duration_seconds * UINT64_C(1000000);
    uint64_t count = 0;
    uint64_t primary_buy = 0, primary_sell = 0;
    uint64_t shadow_buy = 0, shadow_sell = 0;
    uint64_t effective_buy = 0, effective_sell = 0;
    uint64_t disagreement_count = 0;
    int previous_primary_comparison = 0, primary_ready = 0;
    int previous_shadow_comparison = 0, shadow_ready = 0;
#if SMA_KIND == 1 && !FIXED_PRICE
    double primary_short_sum = 0.0, primary_long_sum = 0.0;
#endif
    double shadow_short_sum = 0.0, shadow_long_sum = 0.0;
#if FIXED_PRICE
    int64_t fixed_short_sum = 0, fixed_long_sum = 0;
#endif
    int read_status = 1;

    for (size_t index = 0; ; ++index) {
        ReplayTrade row = {0};
#if STORAGE_KIND == 0
        if (index == 0) {
            if (!has_first_stream_row) { read_status = 0; break; }
            row = first_stream_row;
        } else {
            int status = reader_next(&stream_reader, &row);
            if (status <= 0) {
                read_status = status;
                if (status < 0) snprintf(error, sizeof error, "malformed replay input");
                break;
            }
        }
#else
    Trade stored = {0};
        if (index >= trade_count) { read_status = 0; break; }
        stored = replay_trade_at(index, heap_trades);
        row.id = stored.trade_id;
        row.timestamp_us = stored.timestamp_us;
        row.price = stored.price;
        row.qty = stored.qty;
        row.is_buy = stored.is_buy;
#if FIXED_PRICE
        long double stored_units = (long double)stored.price * (long double)REPLAY_PRICE_SCALE;
        if (stored_units < 0.0L || stored_units > (long double)INT64_MAX ||
            fabsl(stored_units - roundl(stored_units)) > 0.0001L) {
            read_status = -1; snprintf(error, sizeof error, "fixed-price conversion failed at trade %zu", index + 1); break;
        }
        row.price_units = (int64_t)roundl(stored_units);
#endif
#endif
        if (replay_stop_requested) { read_status = -2; break; }
        if (have_previous_timestamp && row.timestamp_us < previous_timestamp) {
            read_status = -1;
            snprintf(error, sizeof error, "decreasing timestamp at trade %zu", index + 1);
            break;
        }
        if (!first_timestamp_ready) {
            first_timestamp_us = row.timestamp_us;
            first_timestamp_ready = 1;
            if (shared_start_ns != 0) {
                while (now_ns() < shared_start_ns && !replay_stop_requested) sleep_ms(1);
                if (replay_stop_requested) { read_status = -2; break; }
                start_ns = shared_start_ns;
            } else {
                start_ns = now_ns();
            }
        }
        if (record_limit != 0 && count >= record_limit) { read_status = 2; break; }
        uint64_t delta_us = (uint64_t)(row.timestamp_us - first_timestamp_us);
        if (duration_enabled && delta_us > duration_us) { read_status = 3; break; }
        /* Convert timestamps relative to the first trade into absolute
         * monotonic deadlines so waits do not accumulate relative drift. */
        uint64_t release_ns = 0;
        if (!replay_schedule_ns(start_ns, first_timestamp_us, row.timestamp_us, speed, &release_ns)) {
            read_status = -1; snprintf(error, sizeof error, "release time overflow"); break;
        }
        if (!replay_wait_until(release_ns, real_now, real_wait, NULL, &replay_stop_requested)) {
            read_status = -2; break;
        }
        uint64_t processing_start_ns = now_ns();

#if FIXED_PRICE
        row.price = (double)row.price_units / (double)REPLAY_PRICE_SCALE;
#endif
        if (!isfinite(row.price) || !isfinite(row.qty) || row.price <= 0.0 || row.qty <= 0.0) {
            read_status = -1; snprintf(error, sizeof error, "invalid trade values at trade %zu", index + 1); break;
        }

        /* The long-sized ring serves both windows; the slot leaving the long
         * window is removed before this trade replaces it. */
        size_t ring_index = (size_t)(count % (uint64_t)long_window);
        if (count >= (uint64_t)short_window) {
#if FIXED_PRICE
            int64_t expired = fixed_ring[(count - (uint64_t)short_window) % (uint64_t)long_window];
            if (expired > fixed_short_sum) { read_status = -1; snprintf(error, sizeof error, "fixed short SMA underflow"); break; }
            fixed_short_sum -= expired;
            shadow_short_sum -= (double)expired / (double)REPLAY_PRICE_SCALE;
#else
            double expired = double_ring[(count - (uint64_t)short_window) % (uint64_t)long_window];
#if SMA_KIND == 1
            primary_short_sum -= expired;
#endif
            shadow_short_sum -= expired;
#endif
        }
        if (count >= (uint64_t)long_window) {
#if FIXED_PRICE
            int64_t expired = fixed_ring[ring_index];
            if (expired > fixed_long_sum) { read_status = -1; snprintf(error, sizeof error, "fixed long SMA underflow"); break; }
            fixed_long_sum -= expired;
            shadow_long_sum -= (double)expired / (double)REPLAY_PRICE_SCALE;
#else
            double expired = double_ring[ring_index];
#if SMA_KIND == 1
            primary_long_sum -= expired;
#endif
            shadow_long_sum -= expired;
#endif
        }
        double_ring[ring_index] = row.price;
        shadow_short_sum += row.price;
        shadow_long_sum += row.price;
#if FIXED_PRICE
        fixed_ring[ring_index] = row.price_units;
        if ((row.price_units > 0 && fixed_short_sum > INT64_MAX - row.price_units) ||
            (row.price_units > 0 && fixed_long_sum > INT64_MAX - row.price_units)) {
            read_status = -1; snprintf(error, sizeof error, "fixed SMA sum overflow at trade %zu", index + 1); break;
        }
        fixed_short_sum += row.price_units;
        fixed_long_sum += row.price_units;
#else
#if SMA_KIND == 1
        primary_short_sum += row.price;
        primary_long_sum += row.price;
#endif
#endif

        int short_ready = count + 1 >= (uint64_t)short_window;
        int long_ready = count + 1 >= (uint64_t)long_window;
        double primary_short_sma = 0.0, primary_long_sma = 0.0;
        double shadow_short_sma = 0.0, shadow_long_sma = 0.0;
#if SMA_KIND == 0
        if (short_ready) {
            double sum = 0.0;
            for (int i = 0; i < short_window; ++i) {
#if FIXED_PRICE
                size_t slot = (ring_index + (size_t)long_window - (size_t)i) % (size_t)long_window;
                sum += (double)fixed_ring[slot] / (double)REPLAY_PRICE_SCALE;
#else
                sum += double_ring[(ring_index + (size_t)long_window - (size_t)i) % (size_t)long_window];
#endif
            }
            primary_short_sma = sum / short_window;
        }
        if (long_ready) {
            double sum = 0.0;
            for (int i = 0; i < long_window; ++i) {
#if FIXED_PRICE
                size_t slot = (ring_index + (size_t)long_window - (size_t)i) % (size_t)long_window;
                sum += (double)fixed_ring[slot] / (double)REPLAY_PRICE_SCALE;
#else
                sum += double_ring[(ring_index + (size_t)long_window - (size_t)i) % (size_t)long_window];
#endif
            }
            primary_long_sma = sum / long_window;
        }
#else
#if FIXED_PRICE
        if (short_ready) primary_short_sma = (double)fixed_short_sum / short_window / (double)REPLAY_PRICE_SCALE;
        if (long_ready) primary_long_sma = (double)fixed_long_sum / long_window / (double)REPLAY_PRICE_SCALE;
#else
        if (short_ready) primary_short_sma = primary_short_sum / short_window;
        if (long_ready) primary_long_sma = primary_long_sum / long_window;
#endif
#endif
        if (short_ready) shadow_short_sma = shadow_short_sum / short_window;
        if (long_ready) shadow_long_sma = shadow_long_sum / long_window;

        int primary_comparison = 0;
        int shadow_comparison = 0;
        if (short_ready && long_ready) {
#if FIXED_PRICE
            primary_comparison = compare_fixed_average(fixed_short_sum, short_window,
                                                       fixed_long_sum, long_window);
#else
            double primary_difference = primary_short_sma - primary_long_sma;
            primary_comparison = (primary_difference > 0.0) - (primary_difference < 0.0);
#endif
            double shadow_difference = shadow_short_sma - shadow_long_sma;
            shadow_comparison = (shadow_difference > 0.0) - (shadow_difference < 0.0);
        }
        int primary_signal = crossed(previous_primary_comparison, primary_comparison, primary_ready);
        int shadow_signal = crossed(previous_shadow_comparison, shadow_comparison, shadow_ready);
        if (short_ready && long_ready) {
            previous_primary_comparison = primary_comparison;
            previous_shadow_comparison = shadow_comparison;
            primary_ready = 1;
            shadow_ready = 1;
        }
        /* Primary is the chosen strategy output; legacy shadow remains a
         * diagnostic unless explicitly selected by the caller. */
        int effective_signal = policy == SIGNAL_POLICY_PRIMARY ? primary_signal : shadow_signal;
        if (primary_signal != shadow_signal) ++disagreement_count;
        if (primary_signal > 0) ++primary_buy; else if (primary_signal < 0) ++primary_sell;
        if (shadow_signal > 0) ++shadow_buy; else if (shadow_signal < 0) ++shadow_sell;
        if (effective_signal > 0) ++effective_buy; else if (effective_signal < 0) ++effective_sell;

        uint64_t decision_ns = now_ns();
        if (!replay_latency_add(&latency, release_ns, processing_start_ns, decision_ns)) {
            read_status = -1; snprintf(error, sizeof error, "latency sample allocation/overflow failed"); break;
        }
        if (race_progress && (count == 0 || (count + 1) % 1000U == 0 || effective_signal != 0)) {
            uint64_t duration_ns = decision_ns - processing_start_ns;
            /* Processing work and deadline lateness are separate durations. */
            uint64_t release_latency_ns = decision_ns - release_ns;
            if (printf("RACE|%" PRIu64 "|%" PRId64 "|%.12f|%d|%" PRIu64 "|%" PRIu64 "\n",
                       count + 1, row.timestamp_us, row.price, effective_signal,
                       duration_ns, release_latency_ns) < 0 || fflush(stdout) != 0) {
                read_status = -1; snprintf(error, sizeof error, "race progress output failed"); break;
            }
        }
        if (decision_file != NULL) {
            if (fprintf(decision_file, "%" PRIu64 ",%" PRId64 ",%.12f,%.12f,%d,%.12f,%.12f,%d,%d,%s\n",
                        count + 1, row.timestamp_us, primary_short_sma, primary_long_sma,
                        primary_signal, shadow_short_sma, shadow_long_sma, shadow_signal,
                        effective_signal, policy == SIGNAL_POLICY_PRIMARY ? "primary" : "legacy-hybrid-shadow") < 0) {
                read_status = -1; snprintf(error, sizeof error, "decision log write failed"); break;
            }
        }
        if (effective_signal != 0) replay_output_signal(&output, effective_signal, count + 1);
        ++count;
        previous_timestamp = row.timestamp_us;
        have_previous_timestamp = 1;
    }

#if STORAGE_KIND == 0
    if (!reader_close(&stream_reader) && read_status >= 0) {
        read_status = -1;
        snprintf(error, sizeof error, "replay input close failed");
    }
#endif
    if (decision_file != NULL && fclose(decision_file) != 0 && read_status >= 0) {
        read_status = -1; snprintf(error, sizeof error, "decision log close failed");
    }
    replay_output_cleanup(&output);
    signal(SIGINT, old_int_handler);
    signal(SIGTERM, old_term_handler);
    uint64_t elapsed_ns = first_timestamp_ready ? now_ns() - start_ns : 0;

    if (latency_path != NULL) {
        FILE *latency_file = fopen(latency_path, "w");
        if (latency_file == NULL) {
            perror(latency_path);
            read_status = -1;
            snprintf(error, sizeof error, "latency sample file open failed");
        } else {
            fputs("trade,scheduled_release_ns,processing_start_ns,decision_ns,processing_duration_ns,release_to_decision_latency_ns\n", latency_file);
            for (size_t i = 0; i < latency.count; ++i) {
                const ReplayLatencySample *sample = &latency.samples[i];
                uint64_t scheduled = sample->decision_ns - sample->release_latency_ns;
                uint64_t processing_start = sample->decision_ns - sample->processing_duration_ns;
                if (fprintf(latency_file, "%zu,%llu,%llu,%llu,%llu,%llu\n",
                            i + 1, (unsigned long long)scheduled,
                            (unsigned long long)processing_start,
                            (unsigned long long)sample->decision_ns,
                            (unsigned long long)sample->processing_duration_ns,
                            (unsigned long long)sample->release_latency_ns) < 0) {
                    read_status = -1;
                    snprintf(error, sizeof error, "latency sample write failed");
                    break;
                }
            }
            if (fclose(latency_file) != 0 && read_status >= 0) {
                read_status = -1;
                snprintf(error, sizeof error, "latency sample close failed");
            }
        }
    }

    if (read_status < 0) fprintf(stderr, "Replay failed after %" PRIu64 " trades: %s\n", count, error);
    printf("replay_version=%s\n", version_id);
    printf("replay_policy=%s\n", policy == SIGNAL_POLICY_PRIMARY ? "primary" : "legacy-hybrid-shadow");
    printf("output_mode=%s\n", output_mode == REPLAY_OUTPUT_OFF ? "off" : output_mode == REPLAY_OUTPUT_CONSOLE ? "console" : "hardware");
    printf("replay_speed=%ux\n", speed);
    printf("preload_ns=%" PRIu64 "\n", preload_ns);
    printf("counting_ns=%" PRIu64 "\n", counting_ns);
    printf("loading_ns=%" PRIu64 "\n", loading_ns);
    printf("processed_count=%" PRIu64 "\n", count);
    printf("elapsed_ns=%" PRIu64 "\n", elapsed_ns);
    printf("lag_definition=max(decision_ns-scheduled_release_ns,0)\n");
    printf("max_lag_ns=%" PRIu64 "\n", latency.worst_release_latency_ns);
    printf("primary_buy=%" PRIu64 "\nprimary_sell=%" PRIu64 "\n", primary_buy, primary_sell);
    printf("shadow_buy=%" PRIu64 "\nshadow_sell=%" PRIu64 "\n", shadow_buy, shadow_sell);
    printf("effective_buy=%" PRIu64 "\neffective_sell=%" PRIu64 "\n", effective_buy, effective_sell);
    printf("primary_shadow_disagreements=%" PRIu64 "\n", disagreement_count);
    printf("latency_sample_count=%zu\n", latency.count);
    printf("latency_sample_bytes=%zu\n", latency.count * sizeof *latency.samples);
    printf("percentile_convention=nearest-rank ceil(p*N), 1-based rank\n");
    printf("processing_average_ns=%" PRIu64 "\nprocessing_worst_ns=%" PRIu64 "\n",
           replay_average_processing_ns(&latency), latency.worst_processing_ns);
    printf("release_latency_average_ns=%" PRIu64 "\nrelease_latency_worst_ns=%" PRIu64 "\n",
           replay_average_release_latency_ns(&latency), latency.worst_release_latency_ns);
    uint64_t duration_p50 = 0, duration_p99 = 0, latency_p50 = 0, latency_p99 = 0;
    if (latency.count != 0 && replay_percentile_ns(&latency, 0, 50, &duration_p50) &&
        replay_percentile_ns(&latency, 0, 99, &duration_p99) &&
        replay_percentile_ns(&latency, 1, 50, &latency_p50) &&
        replay_percentile_ns(&latency, 1, 99, &latency_p99)) {
        printf("processing_p50_ns=%" PRIu64 "\nprocessing_p99_ns=%" PRIu64 "\n", duration_p50, duration_p99);
        printf("release_latency_p50_ns=%" PRIu64 "\nrelease_latency_p99_ns=%" PRIu64 "\n", latency_p50, latency_p99);
    }
    if (read_status == -2) printf("stop_reason=interrupted\n");
    else if (read_status == 2) printf("stop_reason=record_limit\n");
    else if (read_status == 3) printf("stop_reason=duration_limit\n");
    else if (read_status == 0) printf("stop_reason=end_of_file\n");

    replay_latency_destroy(&latency);
    free(double_ring);
    free(fixed_ring);
    free(heap_trades);
    return read_status < 0 && read_status != -2 ? 5 : read_status == -2 ? 130 : 0;
}
