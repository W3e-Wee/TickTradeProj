/* Parameterized comparison ladder for CSV readers, SMA, price representation,
 * and storage; compile-time switches share one computation contract. */
#include "timing.h"
#include "market.h"
#include "trade_io.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#endif

#ifndef READER_KIND
#define READER_KIND 1
#endif
#ifndef SMA_KIND
#define SMA_KIND 0
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

#define PRICE_SCALE INT64_C(100000000)
#define MAX_LINE_LENGTH 512
#define MAX_WINDOW 100000

typedef struct {
    long long id;
    long long timestamp_us;
    double price;
    int64_t price_units;
    double qty;
    int is_buy;
} TradeRow;

#if STORAGE_KIND == 1
/* Bounded global storage reserves the configured capacity for process life;
 * this differs from the exact-malloc rung sized to the current dataset. */
static Trade fixed_trade_array[FIXED_TRADE_CAPACITY];
#endif

#if READER_KIND != 0
static int is_header_or_blank(const char *line) {
    while (isspace((unsigned char)*line)) {
        ++line;
    }
    return *line == '\0' || strncmp(line, "trade_id", 8) == 0;
}
static void trim_line(char *line) {
    size_t length = strlen(line);
    while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) {
        line[--length] = '\0';
    }
}
#endif

#if READER_KIND == 1
static int parse_integer(const char *text, long long *out) {
    char *end = NULL;
    errno = 0;
    long long value = strtoll(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0') {
        return 0;
    }
    *out = value;
    return 1;
}

static int parse_double_field(const char *text, double *out) {
    char *end = NULL;
    errno = 0;
    double value = strtod(text, &end);
    if (errno != 0 || end == text || *end != '\0' || !isfinite(value)) {
        return 0;
    }
    *out = value;
    return 1;
}
#endif

#if READER_KIND != 0
static int parse_bool_field(const char *text, int *is_buy) {
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
static int split_fields(char *line, char **fields) {
    size_t count = 0;
    char *cursor = line;
    for (;;) {
        if (count == 7) {
            return 0;
        }
        fields[count++] = cursor;
        char *comma = strchr(cursor, ',');
        if (comma == NULL) {
            break;
        }
        *comma = '\0';
        cursor = comma + 1;
    }
    return count == 7;
}
#endif

#if READER_KIND == 1
static int parse_own_line(char *line, TradeRow *trade) {
    char *fields[7];
    if (!split_fields(line, fields)) {
        return 0;
    }
    double quote_qty = 0.0;
    int best_match = 0;
    if (!parse_integer(fields[0], &trade->id) ||
        !parse_double_field(fields[1], &trade->price) ||
        !parse_double_field(fields[2], &trade->qty) ||
        !parse_double_field(fields[3], &quote_qty) ||
        !parse_integer(fields[4], &trade->timestamp_us) ||
        !parse_bool_field(fields[5], &trade->is_buy) ||
        !parse_bool_field(fields[6], &best_match)) {
        return 0;
    }
    return trade->id >= 0 && trade->timestamp_us >= 0 && trade->price > 0.0 &&
           trade->qty > 0.0 && quote_qty >= 0.0;
}

static int read_trade(FILE *fp, long long *trade_id_out, long long *timestamp_us_out,
                      double *price_out, double *qty_out, int *is_buy_out) {
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof line, fp) != NULL) {
        size_t length = strlen(line);
        if (length == sizeof line - 1 && line[length - 1] != '\n' && !feof(fp)) {
            return -1;
        }
        trim_line(line);
        if (is_header_or_blank(line)) {
            continue;
        }
        TradeRow trade = {0};
        if (!parse_own_line(line, &trade)) {
            return -1;
        }
        *trade_id_out = trade.id;
        *timestamp_us_out = trade.timestamp_us;
        *price_out = trade.price;
        *qty_out = trade.qty;
        *is_buy_out = trade.is_buy;
        return 1;
    }
    return ferror(fp) ? -1 : 0;
}
#endif

#if READER_KIND == 2
static int parse_digits(const char **cursor, uint64_t *value) {
    const char *p = *cursor;
    if (!isdigit((unsigned char)*p)) return 0;
    uint64_t result = 0;
    do {
        unsigned digit = (unsigned)(*p - '0');
        if (result > (UINT64_MAX - digit) / 10U) return 0;
        result = result * 10U + digit;
        ++p;
    } while (isdigit((unsigned char)*p));
    *cursor = p;
    *value = result;
    return 1;
}

static int parse_fast_integer(const char *text, long long *value_out) {
    if (*text == '\0') return 0;
    uint64_t value = 0;
    for (const char *cursor = text; *cursor != '\0'; ++cursor) {
        if (!isdigit((unsigned char)*cursor)) return 0;
        unsigned digit = (unsigned)(*cursor - '0');
        if (value > ((uint64_t)LLONG_MAX - digit) / 10U) return 0;
        value = value * 10U + digit;
    }
    *value_out = (long long)value;
    return 1;
}
#endif

#if READER_KIND == 2
/* The manual reader accepts plain decimal fields (no quoted CSV commas), at
 * most 18 fractional digits, and enforces scale-1e8 precision for fixed mode. */
static int parse_fast_decimal(const char *text, double *value_out,
                              int64_t *units_out, int require_fixed_precision) {
    const char *cursor = text;
    uint64_t whole = 0;
    if (!parse_digits(&cursor, &whole)) {
        return 0;
    }
    long double fraction = 0.0L;
    long double place = 0.1L;
    uint64_t fixed_fraction = 0;
    unsigned fixed_digits = 0;
    unsigned fraction_digits = 0;
    if (*cursor == '.') {
        ++cursor;
        if (!isdigit((unsigned char)*cursor)) {
            return 0;
        }
        while (isdigit((unsigned char)*cursor)) {
            unsigned digit = (unsigned)(*cursor - '0');
            fraction += (long double)digit * place;
            place *= 0.1L;
            ++fraction_digits;
            if (fixed_digits < 8) {
                fixed_fraction = fixed_fraction * 10U + digit;
                ++fixed_digits;
            } else if (require_fixed_precision && digit != 0) {
                return 0;
            }
            if (fraction_digits > 18) {
                return 0;
            }
            ++cursor;
        }
    }
    if (*cursor != '\0' || whole > (uint64_t)INT64_MAX / (uint64_t)PRICE_SCALE) {
        return 0;
    }
    while (fixed_digits < 8) {
        fixed_fraction *= 10U;
        ++fixed_digits;
    }
    uint64_t units = whole * (uint64_t)PRICE_SCALE + fixed_fraction;
    if (units > (uint64_t)INT64_MAX) {
        return 0;
    }
    if (units_out != NULL) {
        *units_out = (int64_t)units;
    }
    long double parsed = (long double)whole + fraction;
    if (!isfinite((double)parsed)) {
        return 0;
    }
    *value_out = (double)parsed;
    return 1;
}

static int parse_fast_line(char *line, TradeRow *trade) {
    char *fields[7];
    if (!split_fields(line, fields)) {
        return 0;
    }
    double quote_qty = 0.0;
    int best_match = 0;
    if (!parse_fast_integer(fields[0], &trade->id) ||
        !parse_fast_decimal(fields[1], &trade->price,
                            FIXED_PRICE ? &trade->price_units : NULL, FIXED_PRICE) ||
        !parse_fast_decimal(fields[2], &trade->qty, NULL, 0) ||
        !parse_fast_decimal(fields[3], &quote_qty, NULL, 0) ||
        !parse_fast_integer(fields[4], &trade->timestamp_us) ||
        !parse_bool_field(fields[5], &trade->is_buy) ||
        !parse_bool_field(fields[6], &best_match)) {
        return 0;
    }
    return trade->id >= 0 && trade->timestamp_us >= 0 && trade->price > 0.0 &&
           trade->qty > 0.0 && quote_qty >= 0.0;
}

static int read_fast_trade(FILE *fp, TradeRow *trade) {
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof line, fp) != NULL) {
        size_t length = strlen(line);
        if (length == sizeof line - 1 && line[length - 1] != '\n' && !feof(fp)) {
            return -1;
        }
        trim_line(line);
        if (is_header_or_blank(line)) {
            continue;
        }
        return parse_fast_line(line, trade) ? 1 : -1;
    }
    return ferror(fp) ? -1 : 0;
}
#endif

#if STORAGE_KIND == 1 || STORAGE_KIND == 2
static Trade make_trade(const TradeRow *row) {
    Trade trade = {0};
    trade.trade_id = row->id;
    trade.price = row->price;
    trade.qty = row->qty;
    trade.quote_qty = row->price * row->qty;
    trade.timestamp_us = row->timestamp_us;
    trade.is_buy = row->is_buy;
    return trade;
}
#endif

#if STORAGE_KIND == 1
static int load_fixed_array(const char *path, size_t capacity, size_t *count_out) {
    if (capacity > (size_t)FIXED_TRADE_CAPACITY ||
        capacity > SIZE_MAX / sizeof fixed_trade_array[0]) {
        fprintf(stderr, "Configured capacity exceeds the compiled static array.\n");
        return 0;
    }
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        perror(path);
        return 0;
    }
    size_t count = 0;
    TradeRow row;
    int status;
    while ((status = read_fast_trade(fp, &row)) > 0) {
        if (count == capacity) {
            fprintf(stderr, "Fixed Trade array capacity exceeded: capacity=%zu\n", capacity);
            fclose(fp);
            return 0;
        }
        fixed_trade_array[count++] = make_trade(&row);
    }
    int close_status = fclose(fp);
    if (status < 0 || close_status != 0) {
        fprintf(stderr, "Failed while reading into fixed Trade array.\n");
        return 0;
    }
    *count_out = count;
    return 1;
}
#endif

#if STORAGE_KIND == 2
/* The exact-allocation rung counts valid rows, then rereads and allocates the
 * precise AoS payload; a changed input between passes is rejected. */
static int count_valid_trades(const char *path, size_t *count_out, uint64_t *elapsed_ns) {
    uint64_t started = now_ns();
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        perror(path);
        return 0;
    }
    size_t count = 0;
    TradeRow row;
    int status;
    while ((status = read_fast_trade(fp, &row)) > 0) {
        if (count == SIZE_MAX) {
            fprintf(stderr, "Valid trade count overflow.\n");
            fclose(fp);
            return 0;
        }
        ++count;
    }
    int close_status = fclose(fp);
    *elapsed_ns = now_ns() - started;
    if (status < 0 || close_status != 0) {
        fprintf(stderr, "Failed while counting valid trades.\n");
        return 0;
    }
    *count_out = count;
    return 1;
}

static int load_exact_array(const char *path, size_t expected_count, size_t capacity,
                            Trade **trades_out, uint64_t *elapsed_ns) {
    if (expected_count > capacity) {
        fprintf(stderr, "Exact AoS capacity exceeded: trades=%zu capacity=%zu\n", expected_count, capacity);
        return 0;
    }
    if (expected_count > SIZE_MAX / sizeof **trades_out) {
        fprintf(stderr, "Exact AoS allocation size overflow.\n");
        return 0;
    }
    const char *fail_allocation = getenv("MARKET_TEST_FAIL_ALLOC");
    if (expected_count != 0 && fail_allocation != NULL && strcmp(fail_allocation, "1") == 0) {
        fprintf(stderr, "Unable to allocate exact AoS buffer (test-injected failure).\n");
        return 0;
    }
    Trade *trades = expected_count == 0 ? NULL : malloc(expected_count * sizeof *trades);
    if (expected_count != 0 && trades == NULL) {
        fprintf(stderr, "Unable to allocate exact AoS buffer (%zu bytes).\n",
                expected_count * sizeof *trades);
        return 0;
    }
    uint64_t started = now_ns();
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        perror(path);
        free(trades);
        return 0;
    }
    size_t count = 0;
    TradeRow row;
    int status;
    while ((status = read_fast_trade(fp, &row)) > 0) {
        if (count >= expected_count) {
            fprintf(stderr, "Dataset changed after counting pass.\n");
            fclose(fp);
            free(trades);
            return 0;
        }
        trades[count++] = make_trade(&row);
    }
    int close_status = fclose(fp);
    *elapsed_ns = now_ns() - started;
    if (status < 0 || close_status != 0 || count != expected_count) {
        fprintf(stderr, "Exact AoS load pass failed or dataset changed (expected=%zu loaded=%zu).\n",
                expected_count, count);
        free(trades);
        return 0;
    }
    *trades_out = trades;
    return 1;
}
#endif

#if FIXED_PRICE && SMA_KIND == 1
/* Checked arithmetic prevents overflow in scale-1e8 price and SMA sums. */
static int checked_add_int64(int64_t left, int64_t right, int64_t *result) {
    if ((right > 0 && left > INT64_MAX - right) ||
        (right < 0 && left < INT64_MIN - right)) {
        return 0;
    }
    *result = left + right;
    return 1;
}

static int checked_sub_int64(int64_t left, int64_t right, int64_t *result) {
    if ((right > 0 && left < INT64_MIN + right) ||
        (right < 0 && left > INT64_MAX + right)) {
        return 0;
    }
    *result = left - right;
    return 1;
}

#endif

static void print_bar(int64_t second, double open, double high, double low,
                      double close, int count) {
    printf("TRACE_BAR|%" PRId64 "|%.8f|%.8f|%.8f|%.8f|%d\n",
           second, open, high, low, close, count);
}

static uint64_t peak_process_rss_bytes(void) {
#if defined(_WIN32)
    typedef BOOL(WINAPI *GetProcessMemoryInfoFn)(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);
    HMODULE psapi = LoadLibraryA("psapi.dll");
    if (psapi == NULL) return 0;
    FARPROC address = GetProcAddress(psapi, "GetProcessMemoryInfo");
    GetProcessMemoryInfoFn get_info = NULL;
    if (sizeof get_info == sizeof address) {
        memcpy(&get_info, &address, sizeof get_info);
    }
    PROCESS_MEMORY_COUNTERS_EX counters = {0};
    counters.cb = sizeof counters;
    uint64_t result = get_info != NULL &&
                      get_info(GetCurrentProcess(), (PPROCESS_MEMORY_COUNTERS)&counters,
                               sizeof counters)
                          ? (uint64_t)counters.PeakWorkingSetSize
                          : 0;
    FreeLibrary(psapi);
    return result;
#else
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0) return 0;
#if defined(__APPLE__)
    return (uint64_t)usage.ru_maxrss;
#else
    return (uint64_t)usage.ru_maxrss * 1024U;
#endif
#endif
}

static void print_phase_metrics(int enabled, uint64_t count_ns, uint64_t load_ns,
                                uint64_t compute_ns, size_t trade_count, int long_window) {
    if (!enabled) return;
    (void)trade_count;
    (void)long_window;
    uint64_t allocated_bytes = 0;
    uint64_t used_bytes = 0;
#if STORAGE_KIND == 1
    allocated_bytes = sizeof fixed_trade_array;
    used_bytes = (uint64_t)trade_count * sizeof fixed_trade_array[0];
#elif STORAGE_KIND == 2
    allocated_bytes = (uint64_t)trade_count * sizeof(Trade);
    used_bytes = allocated_bytes;
#elif FIXED_PRICE
    allocated_bytes = (uint64_t)long_window * sizeof(int64_t);
    used_bytes = allocated_bytes;
#else
    allocated_bytes = (uint64_t)long_window * sizeof(double);
    used_bytes = allocated_bytes;
#endif
    printf("PHASES|counting_ns=%" PRIu64 "|loading_ns=%" PRIu64
           "|compute_ns=%" PRIu64 "|logical_allocated_bytes=%" PRIu64
           "|logical_used_bytes=%" PRIu64 "|process_peak_memory_bytes=%" PRIu64
           "|process_memory_metric=%s|compute_scope=%s\n",
           count_ns, load_ns, compute_ns, allocated_bytes, used_bytes,
           peak_process_rss_bytes(),
#if defined(_WIN32)
           "peak_working_set",
#else
           "ru_maxrss",
#endif
           STORAGE_KIND == 0 ? "stream_read_parse_and_compute" : "compute_only");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <csv-file> [short_window] [long_window] [--trace] [--signals-only] [--bars-only] [--capacity N] [--bench-phases]\n", argv[0]);
        return 2;
    }
    const char *path = argv[1];
    int short_window = 3;
    int long_window = 5;
    int trace = 0;
    int signals_only = 0;
    int bars_only = 0;
    int bench_phases = 0;
    size_t capacity = (size_t)FIXED_TRADE_CAPACITY;
    (void)capacity;
    int positional = 0;
    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "--trace") == 0) {
            trace = 1;
            continue;
        }
        if (strcmp(argv[i], "--signals-only") == 0) {
            signals_only = 1;
            continue;
        }
        if (strcmp(argv[i], "--bars-only") == 0) {
            bars_only = 1;
            continue;
        }
        if (strcmp(argv[i], "--bench-phases") == 0) {
            bench_phases = 1;
            continue;
        }
        if (strcmp(argv[i], "--capacity") == 0 && i + 1 < argc) {
            char *end = NULL;
            unsigned long long value = strtoull(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || value > SIZE_MAX) {
                fprintf(stderr, "Invalid capacity: %s\n", argv[i]);
                return 2;
            }
            capacity = (size_t)value;
            continue;
        }
        char *end = NULL;
        long value = strtol(argv[i], &end, 10);
        if (end == argv[i] || *end != '\0' || value <= 0 || value > MAX_WINDOW || positional >= 2) {
            fprintf(stderr, "Invalid window or argument: %s\n", argv[i]);
            return 2;
        }
        if (positional++ == 0) {
            short_window = (int)value;
        } else {
            long_window = (int)value;
        }
    }
    if (short_window > long_window) {
        fprintf(stderr, "short_window must not exceed long_window.\n");
        return 2;
    }

    double *double_ring = NULL;
    int64_t *fixed_ring = NULL;
#if FIXED_PRICE
    fixed_ring = calloc((size_t)long_window, sizeof *fixed_ring);
    if (fixed_ring == NULL) {
        fprintf(stderr, "Unable to allocate fixed-point ring.\n");
        return 3;
    }
#else
    double_ring = calloc((size_t)long_window, sizeof *double_ring);
    if (double_ring == NULL) {
        fprintf(stderr, "Unable to allocate price ring.\n");
        return 3;
    }
#endif

    FILE *fp = NULL;
    (void)fp;
    Trade *loaded_trades = NULL;
    size_t loaded_count = 0;
    (void)loaded_count;
    uint64_t phase_count_ns = 0;
    uint64_t phase_load_ns = 0;
    uint64_t phase_compute_ns = 0;
#if STORAGE_KIND == 0 && READER_KIND == 0
    if (!open_trades(path)) {
        free(double_ring);
        free(fixed_ring);
        return 4;
    }
#elif STORAGE_KIND == 0
    fp = fopen(path, "r");
    if (fp == NULL) {
        perror(path);
        free(double_ring);
        free(fixed_ring);
        return 4;
    }
#elif STORAGE_KIND == 1
    uint64_t load_started = now_ns();
    if (!load_fixed_array(path, capacity, &loaded_count)) {
        free(double_ring);
        free(fixed_ring);
        return 4;
    }
    phase_load_ns = now_ns() - load_started;
#elif STORAGE_KIND == 2
    if (!count_valid_trades(path, &loaded_count, &phase_count_ns)) {
        free(double_ring);
        free(fixed_ring);
        return 4;
    }
    if (!load_exact_array(path, loaded_count, capacity, &loaded_trades, &phase_load_ns)) {
        free(double_ring);
        free(fixed_ring);
        return 4;
    }
#else
#error STORAGE_KIND must be 0 (stream), 1 (static array), or 2 (exact malloc AoS)
#endif

    double vwap_numerator = 0.0;
    double vwap_denominator = 0.0;
    double short_double_sum = 0.0;
    double long_double_sum = 0.0;
#if SMA_KIND == 1 && FIXED_PRICE
    int64_t short_fixed_sum = 0;
    int64_t long_fixed_sum = 0;
#endif
    size_t count = 0;
    int previous_comparison = 0;
    int comparison_ready = 0;
    int64_t buy_count = 0;
    int64_t sell_count = 0;
    int bar_active = 0;
    int64_t bar_second = 0;
    double bar_open = 0.0, bar_high = 0.0, bar_low = 0.0, bar_close = 0.0;
    int bar_count = 0;
    uint64_t bar_total = 0;
    int read_status = 0;

    uint64_t compute_started = now_ns();
    for (;;) {
        TradeRow trade = {0};
#if STORAGE_KIND == 0 && READER_KIND == 0
        if (!next_trade()) {
            read_status = 0;
            break;
        }
        trade.id = trade_id();
        trade.timestamp_us = trade_time();
        trade.price = trade_price();
        trade.qty = trade_qty();
        trade.is_buy = trade_is_buy();
#elif STORAGE_KIND == 0 && READER_KIND == 1
        read_status = read_trade(fp, &trade.id, &trade.timestamp_us,
                                 &trade.price, &trade.qty, &trade.is_buy);
        if (read_status <= 0) {
            break;
        }
#elif STORAGE_KIND == 0
        read_status = read_fast_trade(fp, &trade);
        if (read_status <= 0) {
            break;
        }
#else
        if (count >= loaded_count) {
            read_status = 0;
            break;
        }
    #if STORAGE_KIND == 1
        const Trade *stored = &fixed_trade_array[count];
    #else
        const Trade *stored = &loaded_trades[count];
    #endif
        trade.id = stored->trade_id;
        trade.timestamp_us = stored->timestamp_us;
        trade.price = stored->price;
        trade.qty = stored->qty;
        trade.is_buy = stored->is_buy;
#endif
        if (count == SIZE_MAX) {
            fprintf(stderr, "Trade count overflow.\n");
            read_status = -1;
            break;
        }

#if FIXED_PRICE
#if READER_KIND == 0
        long double units = (long double)trade.price * (long double)PRICE_SCALE;
        if (units > (long double)INT64_MAX || units < 0.0L ||
            fabsl(units - roundl(units)) > 0.0001L) {
            fprintf(stderr, "Fixed-point price is out of range or precision at row %zu.\n", count + 1);
            read_status = -1;
            break;
        }
        trade.price_units = (int64_t)roundl(units);
#endif
        trade.price = (double)trade.price_units / (double)PRICE_SCALE;
#endif
        if (!isfinite(trade.price) || !isfinite(trade.qty) || trade.price <= 0.0 || trade.qty <= 0.0) {
            fprintf(stderr, "Invalid numeric value at row %zu.\n", count + 1);
            read_status = -1;
            break;
        }

        /* The long-window ring also holds the short window; modulo slots let
         * each arrival replace its expired long-window sample in constant time. */
        size_t ring_index = count % (size_t)long_window;
#if !FIXED_PRICE || SMA_KIND == 1
        if (count >= (size_t)short_window) {
#if FIXED_PRICE
            int64_t updated = 0;
            int64_t expired = fixed_ring[(count - (size_t)short_window) % (size_t)long_window];
            short_double_sum -= (double)expired / (double)PRICE_SCALE;
            if (!checked_sub_int64(short_fixed_sum, expired, &updated)) {
                read_status = -1;
                break;
            }
            short_fixed_sum = updated;
#else
            short_double_sum -= double_ring[(count - (size_t)short_window) % (size_t)long_window];
#endif
        }
        if (count >= (size_t)long_window) {
#if FIXED_PRICE
            int64_t updated = 0;
            long_double_sum -= (double)fixed_ring[ring_index] / (double)PRICE_SCALE;
            if (!checked_sub_int64(long_fixed_sum, fixed_ring[ring_index], &updated)) {
                read_status = -1;
                break;
            }
            long_fixed_sum = updated;
#else
            long_double_sum -= double_ring[ring_index];
#endif
        }
#endif
#if FIXED_PRICE
        fixed_ring[ring_index] = trade.price_units;
#if SMA_KIND == 1
        if (!checked_add_int64(short_fixed_sum, trade.price_units, &short_fixed_sum) ||
            !checked_add_int64(long_fixed_sum, trade.price_units, &long_fixed_sum)) {
            fprintf(stderr, "Fixed-point SMA sum overflow at row %zu.\n", count + 1);
            read_status = -1;
            break;
        }
        short_double_sum += trade.price;
        long_double_sum += trade.price;
        #endif
#else
        double_ring[ring_index] = trade.price;
        short_double_sum += trade.price;
        long_double_sum += trade.price;
#endif

        int short_ready = count + 1 >= (size_t)short_window;
        int long_ready = count + 1 >= (size_t)long_window;
        double short_sma = 0.0;
        double long_sma = 0.0;
#if SMA_KIND == 0
        /* The naive rung re-sums each active window; its running comparator
         * shadow keeps crossover equality behavior aligned with the baseline. */
        if (short_ready) {
#if FIXED_PRICE
            int64_t sum = 0;
            for (int i = 0; i < short_window; ++i) {
                size_t slot = (ring_index + (size_t)long_window - (size_t)i) % (size_t)long_window;
                if (!checked_add_int64(sum, fixed_ring[slot], &sum)) read_status = -1;
            }
            short_sma = (double)sum / (double)short_window / (double)PRICE_SCALE;
#else
            double sum = 0.0;
            for (int i = 0; i < short_window; ++i) {
                sum += double_ring[(ring_index + (size_t)long_window - (size_t)i) % (size_t)long_window];
            }
            short_sma = sum / (double)short_window;
#endif
        }
        if (long_ready) {
#if FIXED_PRICE
            int64_t sum = 0;
            for (int i = 0; i < long_window; ++i) {
                size_t slot = (ring_index + (size_t)long_window - (size_t)i) % (size_t)long_window;
                if (!checked_add_int64(sum, fixed_ring[slot], &sum)) read_status = -1;
            }
            long_sma = (double)sum / (double)long_window / (double)PRICE_SCALE;
#else
            double sum = 0.0;
            for (int i = 0; i < long_window; ++i) {
                sum += double_ring[(ring_index + (size_t)long_window - (size_t)i) % (size_t)long_window];
            }
            long_sma = sum / (double)long_window;
#endif
        }
#else
        if (short_ready) {
#if FIXED_PRICE
            short_sma = (double)short_fixed_sum / (double)short_window / (double)PRICE_SCALE;
#else
            short_sma = short_double_sum / (double)short_window;
#endif
        }
        if (long_ready) {
#if FIXED_PRICE
            long_sma = (double)long_fixed_sum / (double)long_window / (double)PRICE_SCALE;
#else
            long_sma = long_double_sum / (double)long_window;
#endif
        }
#endif
        if (read_status < 0) {
            fprintf(stderr, "SMA arithmetic overflow at row %zu.\n", count + 1);
            break;
        }

        vwap_numerator += trade.price * trade.qty;
        vwap_denominator += trade.qty;
        if (!isfinite(vwap_numerator) || !isfinite(vwap_denominator) || vwap_denominator <= 0.0) {
            fprintf(stderr, "VWAP accumulator overflow at row %zu.\n", count + 1);
            read_status = -1;
            break;
        }

        int signal = 0;
        if (short_ready && long_ready) {
            int comparison = 0;
#if SMA_KIND == 0 && !FIXED_PRICE
            double difference = short_double_sum / (double)short_window -
                                long_double_sum / (double)long_window;
#else
            double difference = short_double_sum / (double)short_window -
                                long_double_sum / (double)long_window;
#endif
            comparison = (difference > 0.0) - (difference < 0.0);
            if (comparison_ready) {
                if (previous_comparison <= 0 && comparison > 0) {
                    signal = 1;
                    ++buy_count;
                } else if (previous_comparison >= 0 && comparison < 0) {
                    signal = -1;
                    ++sell_count;
                }
            }
            previous_comparison = comparison;
            comparison_ready = 1;
        }

        int64_t second = trade.timestamp_us / INT64_C(1000000);
        if (!bar_active || second != bar_second) {
            if (bar_active) {
                ++bar_total;
                if (trace || bars_only) print_bar(bar_second, bar_open, bar_high, bar_low, bar_close, bar_count);
            }
            bar_second = second;
            bar_open = bar_high = bar_low = bar_close = trade.price;
            bar_count = 1;
            bar_active = 1;
        } else {
            if (trade.price > bar_high) bar_high = trade.price;
            if (trade.price < bar_low) bar_low = trade.price;
            bar_close = trade.price;
            ++bar_count;
        }

        ++count;
        if (signals_only && signal != 0) {
            printf("SIGNAL|%zu|%s|%.8f|%.8f\n", count,
                   signal > 0 ? "BUY" : "SELL", short_sma, long_sma);
        }
        if (trace) {
            printf("TRACE_ROW|%zu|%.8f|%.8f|%d|%.8f|", count, trade.price,
                   trade.qty, trade.is_buy, vwap_numerator / vwap_denominator);
            if (short_ready) printf("%.8f", short_sma); else printf("-");
            printf("|");
            if (long_ready) printf("%.8f", long_sma); else printf("-");
            printf("|%s\n", signal > 0 ? "BUY" : signal < 0 ? "SELL" : "-");
        }
    }

#if STORAGE_KIND == 0 && READER_KIND == 0
    close_trades();
#elif STORAGE_KIND == 0
    if (fclose(fp) != 0) {
        perror("fclose");
        read_status = -1;
    }
#endif
    phase_compute_ns = now_ns() - compute_started;
    if (read_status < 0) {
        fprintf(stderr, "Input or calculation failed after %zu trades.\n", count);
        free(double_ring);
        free(fixed_ring);
        free(loaded_trades);
        return 5;
    }
    if (count == 0) {
    #if STORAGE_KIND == 0
        fprintf(stderr, "No valid trades found.\n");
        free(double_ring);
        free(fixed_ring);
        free(loaded_trades);
        return 5;
    #else
        printf("trades=0\nfinal_vwap=NA\nbuy_signals=0\nsell_signals=0\nohlc_bars=0\n");
        print_phase_metrics(bench_phases, phase_count_ns, phase_load_ns,
                    phase_compute_ns, count, long_window);
        free(double_ring);
        free(fixed_ring);
        free(loaded_trades);
        return 0;
    #endif
    }
    if (bar_active) {
        ++bar_total;
        if (trace || bars_only) print_bar(bar_second, bar_open, bar_high, bar_low, bar_close, bar_count);
    }

    printf("trades=%zu\n", count);
    printf("final_vwap=%.8f\n", vwap_numerator / vwap_denominator);
    printf("buy_signals=%" PRId64 "\n", buy_count);
    printf("sell_signals=%" PRId64 "\n", sell_count);
    printf("ohlc_bars=%" PRIu64 "\n", bar_total);
    print_phase_metrics(bench_phases, phase_count_ns, phase_load_ns,
                        phase_compute_ns, count, long_window);
    free(double_ring);
    free(fixed_ring);
    free(loaded_trades);
    return 0;
}
