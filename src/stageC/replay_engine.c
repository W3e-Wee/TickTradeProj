/* Exact crossover signals with per-core VWAP behavior and no I/O. */
#include "replay_engine.h"
#include "result.h"
#include <math.h>
#include <string.h>

/* Attach caller-owned rings before a replay starts. */
int replay_engine_init(ReplayEngine *engine, int short_window, int long_window,
                       int fast_core, int64_t *short_ring, int64_t *long_ring)
{
    if (engine == NULL || short_window <= 0 || long_window < short_window ||
        short_ring == NULL || long_ring == NULL) return 0;
    memset(engine, 0, sizeof *engine);
    engine->short_window = short_window; engine->long_window = long_window;
    engine->fast_core = fast_core; engine->short_ring = short_ring; engine->long_ring = long_ring;
    engine->signal_hash = UINT64_C(14695981039346656037);
    engine->active_second = -1;
    return 1;
}

/* Keep nonnegative rolling sums checked so malformed input cannot wrap. */
static int replay_push_sum(int64_t *ring, int window, uint64_t index,
                           int64_t price, int64_t *sum)
{
    size_t slot = (size_t)(index % (uint64_t)window);
    int64_t next = *sum;
    if (index >= (uint64_t)window && !fp_checked_add(next, -ring[slot], &next)) return -1;
    ring[slot] = price;
    if (!fp_checked_add(next, price, &next)) return -1;
    *sum = next;
    return index + 1U >= (uint64_t)window;
}

/* Update VWAP, exact SMA direction, bar count and signal hash for one trade. */
int replay_engine_step(ReplayEngine *engine, int64_t timestamp_us,
                       double price, double qty, int *signal_out)
{
    if (engine == NULL || signal_out == NULL || timestamp_us < 0 ||
        !isfinite(price) || !isfinite(qty) || price <= 0.0 || qty <= 0.0) return 0;
    double scaled_price = price * (double)PRICE_SCALE;
    double scaled_qty = qty * (double)QTY_SCALE;
    if (!isfinite(scaled_price) || !isfinite(scaled_qty) || scaled_price < 0.5 ||
        scaled_qty < 0.5 || scaled_price >= (double)INT64_MAX || scaled_qty >= (double)INT64_MAX) return 0;
    int64_t fixed_price = (int64_t)llround(scaled_price);
    int64_t fixed_qty = (int64_t)llround(scaled_qty);
    uint64_t index = engine->trades;
    int short_ready = replay_push_sum(engine->short_ring, engine->short_window,
                                      index, fixed_price, &engine->short_sum);
    int long_ready = replay_push_sum(engine->long_ring, engine->long_window,
                                     index, fixed_price, &engine->long_sum);
    if (short_ready < 0 || long_ready < 0) return 0;
    *signal_out = 0;
    if (short_ready && long_ready) {
        int sign = fp_compare_products((uint64_t)engine->short_sum,
            (uint64_t)engine->long_window, (uint64_t)engine->long_sum,
            (uint64_t)engine->short_window);
        if (engine->signal_ready && engine->previous_sign <= 0 && sign > 0) *signal_out = 1;
        else if (engine->signal_ready && engine->previous_sign >= 0 && sign < 0) *signal_out = -1;
        engine->previous_sign = sign; engine->signal_ready = 1;
    }
    if (engine->fast_core) {
        if (!fp_acc_add_product(&engine->numerator, (uint64_t)fixed_price,
                                (uint64_t)fixed_qty) ||
            !fp_checked_add(engine->qty_sum, fixed_qty, &engine->qty_sum)) return 0;
    } else {
        engine->double_numerator += price * qty;
        engine->double_qty += qty;
    }
    int64_t second = timestamp_us / 1000000LL;
    if (engine->trades == 0 || second != engine->active_second) ++engine->bars;
    engine->active_second = second;
    ++engine->trades;
    if (*signal_out > 0) {
        ++engine->buys;
        engine->signal_hash = result_hash_signal(engine->signal_hash, engine->trades, 1);
    } else if (*signal_out < 0) {
        ++engine->sells;
        engine->signal_hash = result_hash_signal(engine->signal_hash, engine->trades, -1);
    }
    return 1;
}

/* Convert the core's accumulated VWAP to a double for reporting only. */
double replay_engine_vwap(const ReplayEngine *engine)
{
    if (engine == NULL) return 0.0;
    if (engine->fast_core)
        return engine->qty_sum == 0 ? 0.0 : fp_acc_to_double(&engine->numerator) /
            (double)engine->qty_sum / (double)PRICE_SCALE;
    return engine->double_qty == 0.0 ? 0.0 : engine->double_numerator / engine->double_qty;
}
