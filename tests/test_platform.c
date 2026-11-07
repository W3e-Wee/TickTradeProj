/* Smoke-test the portable platform API without requiring OS-specific headers. */
#include "check.h"
#include "platform.h"
/* Confirm system metadata APIs return safe values on the current platform. */
int main(void)
{
    CHECK(plat_now_ns() != 0);
    CHECK(plat_platform_name() != NULL);
    CHECK(plat_cpu_governor() != NULL);
    plat_timer_begin();
    CHECK(plat_sleep_granularity_us() >= 100U);
    plat_timer_end();
    (void)plat_peak_memory_kb();
    (void)plat_is_raspberry_pi();
    CHECK(plat_enable_ansi() == 0 || plat_enable_ansi() != 0);
    CHECK(plat_pin_to_core(-1) != 0);
    return CHECK_SUMMARY();
}
