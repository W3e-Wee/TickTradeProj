/* Keep programs independent of OS-specific memory APIs. */
#include "memusage.h"
#include "platform.h"
/* Delegate peak memory measurement to the common platform boundary. */
uint64_t memusage_peak_kb(void)
{
    return plat_peak_memory_kb();
}
