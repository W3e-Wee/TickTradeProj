/* LED, ANSI signal colors, and replay status line output. */
#ifndef TICKTRADE_SIGNAL_DISPLAY_H
#define TICKTRADE_SIGNAL_DISPLAY_H
#include <stdint.h>
/* Initialize platform ANSI handling before any colored output. */
void signal_display_init(void);
/* Render a queued signal and drive its LED; call only outside engine processing. */
void signal_display_signal(int side, uint64_t trade_index, double price);
/* Refresh market time, replay progress, lag, counts, and running P&L. */
void signal_display_status(uint64_t trades, double market_seconds,
                           uint64_t lag_us, uint64_t buys, uint64_t sells,
                           double pnl);
/* Turn off both LEDs, restore terminal color, and flush the final line. */
void signal_display_cleanup(void);
#endif
