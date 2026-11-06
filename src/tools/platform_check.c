/* Report the portable platform services and optionally request CPU affinity. */
#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    if (argc == 3 && strcmp(argv[1], "--pin") == 0) {
        char *end = NULL;
        long core = strtol(argv[2], &end, 10);
        if (end == argv[2] || *end != '\0' || core < 0) return 2;
        if (plat_pin_to_core((int)core) != 0) {
            puts("NOT VERIFIED: CPU affinity is unavailable on this platform.");
            return 0;
        }
        printf("PASS pinned_core=%ld\n", core);
        return 0;
    }
    if (argc != 1) return 2;
    printf("platform=%s governor=%s raspberry_pi=%s peak_mem_kb=%llu\n",
           plat_platform_name(), plat_cpu_governor(), plat_is_raspberry_pi() ? "yes" : "no",
           (unsigned long long)plat_peak_memory_kb());
    return 0;
}
