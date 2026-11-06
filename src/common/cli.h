/* Strict command-line parsing helpers used by every executable. */
#ifndef TICKTRADE_CLI_H
#define TICKTRADE_CLI_H
#include <stdint.h>

/* Parse a nonnegative decimal integer; return 1 on success and 0 otherwise. */
int cli_parse_u64(const char *text, uint64_t *value_out);
/* Parse a positive integer no larger than maximum; reject signs and suffixes. */
int cli_parse_positive_int(const char *text, int maximum, int *value_out);

#endif
