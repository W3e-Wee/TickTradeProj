#include "../provided/timing.h"
#include "market.h"

/* Frozen coursework baseline: parses CSV into a heap Trade array, then
 * computes VWAP, running SMA crossover signals, and timestamp OHLC bars. */
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <psapi.h>
#else
#include <sys/resource.h>
#endif

static int trace_enabled = 0;
static int signals_enabled = 0;
static int bars_enabled = 0;
static int phases_enabled = 0;
static uint64_t phase_loading_ns = 0;
static uint64_t phase_compute_ns = 0;
static uint64_t phase_logical_allocated_bytes = 0;

static uint64_t peak_process_rss_bytes(void) {
#if defined(_WIN32)
    typedef BOOL(WINAPI *GetProcessMemoryInfoFn)(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);
    HMODULE psapi = LoadLibraryA("psapi.dll");
    if (psapi == NULL) return 0;
    FARPROC address = GetProcAddress(psapi, "GetProcessMemoryInfo");
    GetProcessMemoryInfoFn get_info = NULL;
    if (sizeof get_info == sizeof address) memcpy(&get_info, &address, sizeof get_info);
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

static int is_header_or_blank(const char *line) {
    while (*line == ' ' || *line == '\t' || *line == '\r' || *line == '\n') {
        ++line;
    }
    return *line == '\0' || strncmp(line, "trade_id", 8) == 0;
}

static int parse_bool_value(const char *text, int *out) {
    if (strcmp(text, "True") == 0 || strcmp(text, "true") == 0) {
        *out = 1;
        return 1;
    }
    if (strcmp(text, "False") == 0 || strcmp(text, "false") == 0) {
        *out = 0;
        return 1;
    }
    return 0;
}

int parse_trade_line(const char *line, Trade *trade, int line_number) {
    (void)line_number;
    char buffer[512];
    char *parts[7] = {0};
    size_t len = strlen(line);
    if (len >= sizeof(buffer)) {
        return -2;
    }

    memcpy(buffer, line, len + 1);
    size_t idx = 0;
    for (char *p = buffer; *p != '\0'; ++p) {
        if (*p == '\r' || *p == '\n') {
            *p = '\0';
            break;
        }
    }

    if (is_header_or_blank(buffer)) {
        return 1;
    }

    char *token = strtok(buffer, ",");
    while (token != NULL && idx < 7) {
        parts[idx++] = token;
        token = strtok(NULL, ",");
    }
    if (idx != 7) {
        return -3;
    }

    char *end = NULL;
    errno = 0;
    long long trade_id = strtoll(parts[0], &end, 10);
    if (errno != 0 || end == parts[0] || *end != '\0') {
        return -4;
    }

    double price = strtod(parts[1], &end);
    if (end == parts[1] || errno != 0) {
        return -5;
    }

    double qty = strtod(parts[2], &end);
    if (end == parts[2] || errno != 0) {
        return -6;
    }

    double quote_qty = strtod(parts[3], &end);
    if (end == parts[3] || errno != 0) {
        return -7;
    }

    long long ts = strtoll(parts[4], &end, 10);
    if (errno != 0 || end == parts[4] || *end != '\0') {
        return -8;
    }

    int is_buy_flag = 0;
    if (!parse_bool_value(parts[5], &is_buy_flag)) {
        return -9;
    }

    int is_best_match = 0;
    if (!parse_bool_value(parts[6], &is_best_match)) {
        return -10;
    }

    if (qty <= 0.0) {
        return -11;
    }

    trade->trade_id = trade_id;
    trade->price = price;
    trade->qty = qty;
    trade->quote_qty = quote_qty;
    trade->timestamp_us = ts;
    trade->is_buy = is_buy_flag ? 0 : 1;
    trade->is_best_match = is_best_match;
    return 0;
}

int parse_trade_file(const char *path, Trade **trades_out, size_t *count_out) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        return -1;
    }

    Trade *trades = NULL;
    size_t count = 0;
    size_t capacity = 0;
    char line[512];
    int line_number = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        ++line_number;
        Trade next = {0};
        int parse_result = parse_trade_line(line, &next, line_number);
        if (parse_result == 1) {
            continue;
        }
        if (parse_result != 0) {
            fprintf(stderr, "parse error on %s line %d: code=%d\n", path, line_number, parse_result);
            free(trades);
            fclose(fp);
            return -2;
        }

        if (count == capacity) {
            size_t new_capacity = capacity == 0 ? 4096 : capacity * 2;
            Trade *tmp = realloc(trades, new_capacity * sizeof(Trade));
            if (!tmp) {
                free(trades);
                fclose(fp);
                return -3;
            }
            trades = tmp;
            capacity = new_capacity;
            phase_logical_allocated_bytes = (uint64_t)capacity * sizeof *trades;
        }
        trades[count++] = next;
    }

    fclose(fp);
    *trades_out = trades;
    *count_out = count;
    return 0;
}

static void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s <csv-file> [short_window] [long_window]\n", prog);
}

int compute_market_summary(const char *path, int short_window, int long_window, double *final_vwap_out,
                          int64_t *buy_count_out, int64_t *sell_count_out, int64_t *trades_out,
                          int *first_signal_out, int *last_signal_out, int *ohlc_count_out) {
    Trade *trades = NULL;
    size_t trade_count = 0;
    uint64_t load_started = now_ns();
    if (parse_trade_file(path, &trades, &trade_count) != 0) {
        phase_loading_ns = now_ns() - load_started;
        return -1;
    }
    phase_loading_ns = now_ns() - load_started;
    if (trade_count == 0) {
        free(trades);
        return -2;
    }

    double vwap_num = 0.0;
    double vwap_den = 0.0;
    double short_sum = 0.0;
    double long_sum = 0.0;
    int short_count = 0;
    int long_count = 0;
    int64_t buy_count = 0;
    int64_t sell_count = 0;
    int first_signal = -1;
    int last_signal = -1;
    double previous_diff = 0.0;
    int diff_ready = 0;
    OhlcBar current = {0};
    int bar_active = 0;
    int ohlc_count = 0;

    if (short_window <= 0 || long_window <= 0 || short_window > long_window) {
        free(trades);
        return -3;
    }

    uint64_t compute_started = now_ns();
    for (size_t i = 0; i < trade_count; ++i) {
        const Trade *t = &trades[i];
        double row_short_sma = 0.0;
        double row_long_sma = 0.0;
        int short_ready = 0;
        int long_ready = 0;
        int row_signal = 0;
        vwap_num += t->price * t->qty;
        vwap_den += t->qty;

        short_sum += t->price;
        short_count++;
        if (short_count > short_window) {
            short_sum -= trades[i - short_window].price;
            short_count = short_window;
        }

        long_sum += t->price;
        long_count++;
        if (long_count > long_window) {
            long_sum -= trades[i - long_window].price;
            long_count = long_window;
        }

        short_ready = (int)(i + 1) >= short_window;
        long_ready = (int)(i + 1) >= long_window;
        if (short_ready) row_short_sma = short_sum / (double)short_window;
        if (long_ready) row_long_sma = long_sum / (double)long_window;
        if (short_ready && long_ready) {
            double current_diff = row_short_sma - row_long_sma;
            if (diff_ready) {
                if (previous_diff <= 0.0 && current_diff > 0.0) {
                    if (first_signal == -1) first_signal = (int)i + 1;
                    last_signal = (int)i + 1;
                    buy_count++;
                    row_signal = 1;
                } else if (previous_diff >= 0.0 && current_diff < 0.0) {
                    if (first_signal == -1) first_signal = (int)i + 1;
                    last_signal = (int)i + 1;
                    sell_count++;
                    row_signal = -1;
                }
            }
            previous_diff = current_diff;
            diff_ready = 1;
        }

        const long long current_second = t->timestamp_us / 1000000LL;
        if (!bar_active) {
            current.second = current_second;
            current.open = t->price;
            current.high = t->price;
            current.low = t->price;
            current.close = t->price;
            current.trade_count = 1;
            bar_active = 1;
        } else if (current_second == current.second) {
            if (t->price > current.high) current.high = t->price;
            if (t->price < current.low) current.low = t->price;
            current.close = t->price;
            current.trade_count++;
        } else {
            if (trace_enabled || bars_enabled) {
                printf("TRACE_BAR|%lld|%.8f|%.8f|%.8f|%.8f|%d\n",
                       current.second, current.open, current.high, current.low,
                       current.close, current.trade_count);
            }
            ohlc_count++;
            current = (OhlcBar){0};
            current.second = current_second;
            current.open = t->price;
            current.high = t->price;
            current.low = t->price;
            current.close = t->price;
            current.trade_count = 1;
        }

        if (signals_enabled && row_signal != 0) {
            printf("SIGNAL|%zu|%s|%.8f|%.8f\n", i + 1,
                   row_signal > 0 ? "BUY" : "SELL", row_short_sma, row_long_sma);
        }
        if (trace_enabled) {
            printf("TRACE_ROW|%zu|%.8f|%.8f|%d|%.8f|", i + 1, t->price, t->qty,
                   t->is_buy, vwap_num / vwap_den);
            if (short_ready) printf("%.8f", row_short_sma); else printf("-");
            printf("|");
            if (long_ready) printf("%.8f", row_long_sma); else printf("-");
            printf("|%s\n", row_signal > 0 ? "BUY" : row_signal < 0 ? "SELL" : "-");
        }
    }

    if (bar_active) {
        if (trace_enabled || bars_enabled) {
            printf("TRACE_BAR|%lld|%.8f|%.8f|%.8f|%.8f|%d\n",
                   current.second, current.open, current.high, current.low,
                   current.close, current.trade_count);
        }
        ohlc_count++;
    }
    phase_compute_ns = now_ns() - compute_started;

    *final_vwap_out = vwap_num / vwap_den;
    *buy_count_out = buy_count;
    *sell_count_out = sell_count;
    *trades_out = (int64_t)trade_count;
    *first_signal_out = first_signal;
    *last_signal_out = last_signal;
    *ohlc_count_out = ohlc_count;

    free(trades);
    return 0;
}

int main(int argc, char **argv) {
    const char *path = NULL;
    int short_window = DEFAULT_SHORT_WINDOW;
    int long_window = DEFAULT_LONG_WINDOW;

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    path = argv[1];
    int window_index = 0;
    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "--trace") == 0) {
            trace_enabled = 1;
            continue;
        }
        if (strcmp(argv[i], "--signals-only") == 0) {
            signals_enabled = 1;
            continue;
        }
        if (strcmp(argv[i], "--bars-only") == 0) {
            bars_enabled = 1;
            continue;
        }
        if (strcmp(argv[i], "--bench-phases") == 0) {
            phases_enabled = 1;
            continue;
        }
        char *end = NULL;
        long value = strtol(argv[i], &end, 10);
        if (end == argv[i] || *end != '\0' || value <= 0 || value > 100000 || window_index >= 2) {
            fprintf(stderr, "Invalid window or argument: %s\n", argv[i]);
            return 2;
        }
        if (window_index++ == 0) short_window = (int)value;
        else long_window = (int)value;
    }

    double final_vwap = 0.0;
    int64_t buy_count = 0;
    int64_t sell_count = 0;
    int64_t trades = 0;
    int signal_first = -1;
    int signal_last = -1;
    int ohlc_count = 0;
    if (compute_market_summary(path, short_window, long_window, &final_vwap, &buy_count, &sell_count,
                               &trades, &signal_first, &signal_last, &ohlc_count) != 0) {
        fprintf(stderr, "Failed to compute summary for %s\n", path);
        return 4;
    }

    printf("file=%s\n", path);
    printf("trades=%" PRId64 "\n", trades);
    printf("final_vwap=%.8f\n", final_vwap);
    printf("buy_signals=%" PRId64 "\n", buy_count);
    printf("sell_signals=%" PRId64 "\n", sell_count);
    printf("first_signal=%d\n", signal_first);
    printf("last_signal=%d\n", signal_last);
    printf("ohlc_bars=%d\n", ohlc_count);
    if (phases_enabled) {
        uint64_t logical_bytes = (uint64_t)trades * sizeof(Trade);
        printf("PHASES|counting_ns=0|loading_ns=%" PRIu64 "|compute_ns=%" PRIu64
               "|logical_allocated_bytes=%" PRIu64 "|logical_used_bytes=%" PRIu64
               "|process_peak_memory_bytes=%" PRIu64 "|process_memory_metric=%s|compute_scope=compute_only\n",
               phase_loading_ns, phase_compute_ns, phase_logical_allocated_bytes, logical_bytes,
               peak_process_rss_bytes(),
    #if defined(_WIN32)
               "peak_working_set"
    #else
               "ru_maxrss"
    #endif
        );
    }
    return 0;
}
