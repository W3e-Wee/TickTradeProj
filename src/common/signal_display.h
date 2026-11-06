/* LED, ANSI signal colors, and replay status line output. */
#ifndef TICKTRADE_SIGNAL_DISPLAY_H
#define TICKTRADE_SIGNAL_DISPLAY_H
#include <stdint.h>
/* Initialize platform ANSI handling before any colored output. */
void signal_display_init(void);
/* Show one trade signal and drive the matching course LED. */
void signal_display_signal(int side, uint64_t trade_index, double price);
/* Refresh replay progress, nonnegative lag, counts, and running P&L. */
void signal_display_status(uint64_t trades, uint64_t lag_us, uint64_t buys,
                           uint64_t sells, double pnl);
#endif
