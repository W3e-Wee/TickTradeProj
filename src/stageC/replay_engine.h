/* Per-trade indicator engine shared by all replay front ends. */
#ifndef TICKTRADE_REPLAY_ENGINE_H
#define TICKTRADE_REPLAY_ENGINE_H
#include "fixed_point.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
    int64_t *short_ring;
    int64_t *long_ring;
    int short_window;
    int long_window;
    int fast_core;
    uint64_t trades, bars, buys, sells, signal_hash;
    int64_t short_sum, long_sum, qty_sum;
    FpAccumulator numerator;
    double double_numerator, double_qty;
    int previous_sign, signal_ready;
    int64_t active_second;
} ReplayEngine;

int replay_engine_init(ReplayEngine *engine, int short_window, int long_window,
                       int fast_core, int64_t *short_ring, int64_t *long_ring);
int replay_engine_step(ReplayEngine *engine, int64_t timestamp_us,
                       double price, double qty, int *signal_out);
double replay_engine_vwap(const ReplayEngine *engine);
#endif
