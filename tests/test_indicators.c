/* Check worked VWAP, SMA, crossover, and OHLC examples from the specification. */
#include "check.h"
#include "indicators.h"
#include "signal_rule.h"

/* Compare streaming calculations to hand-computed reference values. */
int main(void)
{
    const double prices[] = {100.0, 100.4, 100.2, 100.9, 101.3, 101.0, 101.6, 102.1, 101.8};
    double short_ring[3], long_ring[5], sma3 = 0.0, sma5 = 0.0;
    IndSma short_sma, long_sma;
    IndVwap vwap = {0.0, 0.0};
    IndOhlc bar = {0}, complete = {0};
    SignalDoubleState signal = {0.0, 0};
    CHECK(ind_sma_init(&short_sma, short_ring, 3));
    CHECK(ind_sma_init(&long_sma, long_ring, 5));
    ind_vwap_push(&vwap, 100.0, 2.0);
    ind_vwap_push(&vwap, 101.0, 1.0);
    ind_vwap_push(&vwap, 102.0, 1.0);
    CHECK_NEAR(ind_vwap_value(&vwap), 100.75, 1e-12);
    for (size_t i = 0; i < sizeof prices / sizeof prices[0]; ++i) {
        (void)ind_sma_push(&short_sma, prices[i], &sma3);
        (void)ind_sma_push(&long_sma, prices[i], &sma5);
        if (i == 7) { CHECK_NEAR(sma3, 101.5666666667, 1e-9); CHECK_NEAR(sma5, 101.38, 1e-9); }
    }
    CHECK_NEAR(sma3, 101.8333333333, 1e-9);
    CHECK_NEAR(sma5, 101.56, 1e-9);
    CHECK(signal_double_step(&signal, -0.70, 0.0) == SIGNAL_NONE);
    CHECK(signal_double_step(&signal, 0.35, 0.0) == SIGNAL_BUY);
    CHECK(signal_double_step(&signal, 0.29, 0.0) == SIGNAL_NONE);
    CHECK(signal_double_step(&signal, -0.15, 0.0) == SIGNAL_SELL);
    CHECK(!ind_ohlc_push(&bar, 100000, 100.2, &complete));
    CHECK(!ind_ohlc_push(&bar, 200000, 100.9, &complete));
    CHECK(!ind_ohlc_push(&bar, 300000, 100.1, &complete));
    CHECK(ind_ohlc_push(&bar, 1400000, 100.5, &complete));
    CHECK_NEAR(complete.open, 100.2, 1e-12);
    CHECK_NEAR(complete.high, 100.9, 1e-12);
    CHECK_NEAR(complete.low, 100.1, 1e-12);
    CHECK_NEAR(complete.close, 100.1, 1e-12);
    CHECK(ind_ohlc_flush(&bar, &complete));
    CHECK_NEAR(complete.close, 100.5, 1e-12);
    return CHECK_SUMMARY();
}
