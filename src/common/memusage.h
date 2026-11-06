/* Portable memory reporting wrapper for benchmark result records. */
#ifndef TICKTRADE_MEMUSAGE_H
#define TICKTRADE_MEMUSAGE_H
#include <stdint.h>
/* Return the platform layer's peak resident memory in KiB. */
uint64_t memusage_peak_kb(void);
#endif
