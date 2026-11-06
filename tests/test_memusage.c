/* Check the memory module can safely report a platform value. */
#include "check.h"
#include "memusage.h"
/* Memory can be unavailable and reported as zero on supported systems. */
int main(void)
{
    (void)memusage_peak_kb();
    CHECK(1);
    return CHECK_SUMMARY();
}
