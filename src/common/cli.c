/* Strict CLI numeric parsing; shared by stage programs and tools. */
#include "cli.h"
#include <limits.h>

/* Parse a decimal uint64_t without accepting signs, whitespace, or suffixes. */
int cli_parse_u64(const char *text, uint64_t *value_out)
{
    uint64_t value = 0;
    const unsigned char *cursor = (const unsigned char *)text;
    if (text == 0 || value_out == 0 || *cursor == '\0') {
        return 0;
    }
    while (*cursor != '\0') {
        unsigned digit;
        if (*cursor < '0' || *cursor > '9') {
            return 0;
        }
        digit = (unsigned)(*cursor - '0');
        if (value > (UINT64_MAX - digit) / 10U) {
            return 0;
        }
        value = value * 10U + digit;
        ++cursor;
    }
    *value_out = value;
    return 1;
}

/* Parse a positive integer bounded by maximum for safe window allocation. */
int cli_parse_positive_int(const char *text, int maximum, int *value_out)
{
    uint64_t value;
    if (maximum < 1 || !cli_parse_u64(text, &value) || value == 0 ||
        value > (uint64_t)maximum || value > (uint64_t)INT_MAX || value_out == 0) {
        return 0;
    }
    *value_out = (int)value;
    return 1;
}
