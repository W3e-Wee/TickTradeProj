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

/* Print and indicate one queued BUY/SELL event during deferred rendering. */
void signal_display_signal(int side, uint64_t trade_index, double price)
{
    if (side > 0) { printf("\033[32mBUY trade=%llu price=%.8f\033[0m\n", (unsigned long long)trade_index, price); led_on(LED_GREEN); led_off(LED_GREEN); }
    else if (side < 0) { printf("\033[31mSELL trade=%llu price=%.8f\033[0m\n", (unsigned long long)trade_index, price); led_on(LED_RED); led_off(LED_RED); }
}

/* Refresh one carriage-return status line from the main replay loop. */
void signal_display_status(uint64_t trades, double market_seconds,
                           uint64_t lag_us, uint64_t buys, uint64_t sells,
                           double pnl)
{
    printf("\033[2K\r\033[36mtrades=%llu market=%.3fs lag_us=%llu buys=%llu sells=%llu pnl_vs_rival_R1=%.6f\033[0m",
           (unsigned long long)trades, market_seconds, (unsigned long long)lag_us,
           (unsigned long long)buys, (unsigned long long)sells, pnl);
    fflush(stdout);
}

/* Restore laptop terminal state and release hardware LED control. */
void signal_display_cleanup(void)
{
    fputs("\033[0m\n", stdout);
    led_off(LED_GREEN);
    led_off(LED_RED);
    led_cleanup();
    fflush(stdout);
}
