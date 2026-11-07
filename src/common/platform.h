/* Stable platform interface; operating-system headers stay in platform.c. */
#ifndef TICKTRADE_PLATFORM_H
#define TICKTRADE_PLATFORM_H
#include <stdint.h>

/* Return monotonic nanoseconds, or zero if the platform clock fails. */
uint64_t plat_now_ns(void);
/* Wait approximately milliseconds; return zero when supported. */
int plat_sleep_ms(unsigned milliseconds);
/* Request/release the OS timer resolution used during replay. */
void plat_timer_begin(void);
void plat_timer_end(void);
/* Return the median observed one-millisecond sleep duration in microseconds. */
uint64_t plat_sleep_granularity_us(void);
/* Return the platform label in static storage. */
const char *plat_platform_name(void);
/* Return peak resident memory in KiB, or zero if unavailable. */
uint64_t plat_peak_memory_kb(void);
/* Enable terminal ANSI processing where required; return zero on success. */
int plat_enable_ansi(void);
/* Report whether this machine is a Raspberry Pi. */
int plat_is_raspberry_pi(void);
/* Return a static CPU governor label, or "n/a" where unavailable. */
const char *plat_cpu_governor(void);
/* Pin the current thread to a core; return nonzero if unsupported/failing. */
int plat_pin_to_core(int core);

#endif
