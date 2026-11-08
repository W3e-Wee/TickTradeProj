/* Stage A result interface for the SoA v9 stretch engine. */
#ifndef TICKTRADE_V9_SOA_H
#define TICKTRADE_V9_SOA_H
#include "result.h"

/* Execute v9 and fill a common RESULT record; return zero on input or I/O error. */
int soa_execute(const char *path, int short_window, int long_window, RunResult *result);

#endif
