/* Pure indicator state and update functions; this module performs no I/O. */
#ifndef TICKTRADE_INDICATORS_H
#define TICKTRADE_INDICATORS_H
#include <stddef.h>

typedef struct {
    double sum;
    double correction;
    double *prices;
    size_t window;
    size_t count;
    size_t next;
} IndSma;
typedef struct {
    long long second;
    double open;
    double high;
    double low;
    double close;
    int active;
} IndOhlc;
typedef struct { double price_qty; double qty; } IndVwap;

/* Initialize an SMA using caller-owned circular storage of window doubles. */
int ind_sma_init(IndSma *state, double *storage, size_t window);
/* Add one price; report ready and the average after window prices exist. */
int ind_sma_push(IndSma *state, double price, double *average_out);
/* Accumulate a price-quantity product and quantity for streaming VWAP. */
void ind_vwap_push(IndVwap *state, double price, double qty);
/* Return current VWAP, or zero if no quantity was accumulated. */
double ind_vwap_value(const IndVwap *state);
/* Update a one-second bar; return one when a prior second was completed. */
int ind_ohlc_push(IndOhlc *state, long long timestamp_us, double price, IndOhlc *completed);
/* Copy the final active bar into completed and clear the state. */
int ind_ohlc_flush(IndOhlc *state, IndOhlc *completed);

#endif
