/* Compact trade representation shared by ladder engines and replay tools. */
#ifndef TICKTRADE_TRADE_H
#define TICKTRADE_TRADE_H
typedef struct Trade {
    long long ts_us;
    long long price;
    long long qty;
    int is_buy;
} Trade;
#endif
