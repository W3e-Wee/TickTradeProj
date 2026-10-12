/* Shared market types and interfaces for the frozen engine and variants. */
#ifndef MARKET_H
#define MARKET_H

#include <stddef.h>
#include <stdint.h>

#define DEFAULT_SHORT_WINDOW 3
#define DEFAULT_LONG_WINDOW 5
#define DEFAULT_MAX_OHLC_BARS 4096

typedef struct {
    long long trade_id;
    double price;
    double qty;
    double quote_qty;
    long long timestamp_us;
    int is_buy;
    int is_best_match;
} Trade;

typedef struct {
    double open;
    double high;
    double low;
    double close;
    long long second;
    int trade_count;
} OhlcBar;

typedef struct {
    int short_window;
    int long_window;
    double cum_vwap;
    double vwap_numerator;
    double vwap_denominator;
    int64_t processed_count;
    int64_t buy_signals;
    int64_t sell_signals;
    double last_signal_price;
    double previous_diff;
    int diff_ready;
    int has_signal;
    int previous_signal;
    double short_sum;
    double long_sum;
    int short_count;
    int long_count;
    double *short_ring;
    double *long_ring;
    int short_head;
    int long_head;
    double short_sma;
    double long_sma;
    OhlcBar current_bar;
    int bar_active;
    int64_t last_second;
    int64_t total_second_bars;
} MarketState;

int parse_trade_line(const char *line, Trade *trade, int line_number);
int parse_trade_file(const char *path, Trade **trades_out, size_t *count_out);
int compute_market_summary(const char *path, int short_window, int long_window, double *final_vwap_out,
                          int64_t *buy_count_out, int64_t *sell_count_out, int64_t *trades_out,
                          int *first_signal_out, int *last_signal_out, int *ohlc_count_out);

#endif
