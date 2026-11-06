/* Live signal display keeps terminal colors and course LED calls in one module. */
#include "signal_display.h"
#include "platform.h"
#include "led.h"
#include <stdio.h>

/* Enable Windows VT support before color escapes are written. */
void signal_display_init(void)
{
    (void)plat_enable_ansi();
    led_init();
}

/* Print and indicate a BUY/SELL with green/red output and the matching LED. */
void signal_display_signal(int side, uint64_t trade_index, double price)
{
    if (side > 0) { printf("\033[32mBUY trade=%llu price=%.8f\033[0m\n", (unsigned long long)trade_index, price); led_on(LED_GREEN); led_off(LED_GREEN); }
    else if (side < 0) { printf("\033[31mSELL trade=%llu price=%.8f\033[0m\n", (unsigned long long)trade_index, price); led_on(LED_RED); led_off(LED_RED); }
}

/* Refresh one carriage-return status line without changing lag sign. */
void signal_display_status(uint64_t trades, uint64_t lag_us, uint64_t buys,
                           uint64_t sells, double pnl)
{
    printf("\rtrades=%llu lag_us=%llu buys=%llu sells=%llu pnl=%.2f",
           (unsigned long long)trades, (unsigned long long)lag_us,
           (unsigned long long)buys, (unsigned long long)sells, pnl);
    fflush(stdout);
}
