/* Check signal display initialization is safe without hardware support. */
#include "check.h"
#include "signal_display.h"
/* Initialize ANSI and LED handling; unavailable hardware falls back to text. */
int main(void)
{
    signal_display_init();
    CHECK(1);
    return CHECK_SUMMARY();
}
