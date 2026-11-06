/* Streaming VWAP, compensated circular SMA, and one-second OHLC updates. */
#include "indicators.h"
#include <string.h>

/* Prepare a caller-backed ring buffer so the timed update never allocates. */
int ind_sma_init(IndSma *state, double *storage, size_t window)
{
    if (state == NULL || storage == NULL || window == 0) return 0;
    memset(state, 0, sizeof *state);
    state->prices = storage;
    state->window = window;
    return 1;
}

/* Use Neumaier compensation while replacing the oldest ring-buffer price. */
int ind_sma_push(IndSma *state, double price, double *average_out)
{
    double delta = price;
    double total;
    if (state == NULL || average_out == NULL || state->window == 0) return 0;
    if (state->count == state->window) delta -= state->prices[state->next];
    else ++state->count;
    state->prices[state->next] = price;
    state->next = (state->next + 1) % state->window;
    total = state->sum + delta;
    if (state->sum >= delta) state->correction += (state->sum - total) + delta;
    else state->correction += (delta - total) + state->sum;
    state->sum = total;
    *average_out = (state->sum + state->correction) / (double)state->count;
    return state->count == state->window;
}

/* Add one trade contribution to the two running VWAP totals. */
void ind_vwap_push(IndVwap *state, double price, double qty)
{
    if (state != NULL) { state->price_qty += price * qty; state->qty += qty; }
}

/* Divide accumulated quote value by quantity only when a value exists. */
double ind_vwap_value(const IndVwap *state)
{
    return state != NULL && state->qty != 0.0 ? state->price_qty / state->qty : 0.0;
}

/* Update the active second or return the completed prior bar at a boundary. */
int ind_ohlc_push(IndOhlc *state, long long timestamp_us, double price, IndOhlc *completed)
{
    long long second;
    if (state == NULL || completed == NULL || timestamp_us < 0) return 0;
    second = timestamp_us / 1000000LL;
    if (!state->active) {
        *state = (IndOhlc){second, price, price, price, price, 1};
        return 0;
    }
    if (second != state->second) {
        *completed = *state;
        *state = (IndOhlc){second, price, price, price, price, 1};
        return 1;
    }
    if (price > state->high) state->high = price;
    if (price < state->low) state->low = price;
    state->close = price;
    return 0;
}

/* Flush the last bar at EOF, which has no following trade to trigger it. */
int ind_ohlc_flush(IndOhlc *state, IndOhlc *completed)
{
    if (state == NULL || completed == NULL || !state->active) return 0;
    *completed = *state;
    state->active = 0;
    return 1;
}
