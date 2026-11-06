/* Manual decimal and CSV parsing avoids locale and formatted-conversion overhead. */
#include "fast_parser.h"
#include <limits.h>
#include <stdint.h>
#include <string.h>

#define FAST_LINE 512
#define FAST_DECIMAL_DIGITS 8U

/* Skip plain CSV whitespace without changing the input buffer. */
static void fast_spaces(const char **cursor)
{
    while (**cursor == ' ' || **cursor == '\t' || **cursor == '\r' || **cursor == '\n') ++*cursor;
}

/* Parse an unsigned integer while checking multiply-add overflow. */
static int fast_uint(const char **cursor, uint64_t *value)
{
    const char *p = *cursor; uint64_t result = 0;
    if (*p < '0' || *p > '9') return 0;
    do {
        unsigned digit = (unsigned)(*p++ - '0');
        if (result > (UINT64_MAX - digit) / 10U) return 0;
        result = result * 10U + digit;
    } while (*p >= '0' && *p <= '9');
    *cursor = p; *value = result; return 1;
}

/* Parse a nonnegative decimal of up to eight fractional digits. */
static int fast_decimal(const char **cursor, double *value)
{
    uint64_t whole, fraction = 0, scale = 1; unsigned digits = 0;
    const char *p = *cursor;
    if (!fast_uint(&p, &whole)) return 0;
    if (*p == '.') {
        ++p;
        if (*p < '0' || *p > '9') return 0;
        while (*p >= '0' && *p <= '9') {
            if (digits == FAST_DECIMAL_DIGITS) return 0;
            fraction = fraction * 10U + (unsigned)(*p++ - '0');
            ++digits;
        }
    }
    while (digits-- > 0) scale *= 10U;
    *value = (double)whole + (double)fraction / scale;
    *cursor = p; return 1;
}

/* Require one comma separator after a parsed CSV field. */
static int fast_comma(const char **cursor)
{
    fast_spaces(cursor);
    if (**cursor != ',') return 0;
    ++*cursor; fast_spaces(cursor); return 1;
}

/* Parse True/False without locale-sensitive case conversion. */
static int fast_bool(const char **cursor, int *is_true)
{
    const char *p = *cursor; char word[6]; size_t count = 0;
    while ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z')) {
        if (count + 1 >= sizeof word) return 0;
        char c = *p++;
        word[count++] = (char)(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
    }
    word[count] = '\0';
    if (strcmp(word, "true") == 0) *is_true = 1;
    else if (strcmp(word, "false") == 0) *is_true = 0;
    else return 0;
    *cursor = p; return 1;
}

/* Parse exactly seven fields and map buyer-maker False to is_buy one. */
int fast_parse_line(const char *line, long long *ts, double *price,
                    double *qty, int *is_buy)
{
    const char *p = line; uint64_t id, timestamp; double parsed_price, parsed_qty, quote;
    int buyer_maker, best_match;
    if (line == NULL || ts == NULL || price == NULL || qty == NULL || is_buy == NULL) return -1;
    fast_spaces(&p);
    if (*p < '0' || *p > '9') return 0;
    if (!fast_uint(&p, &id) || !fast_comma(&p) || !fast_decimal(&p, &parsed_price) ||
        !fast_comma(&p) || !fast_decimal(&p, &parsed_qty) || !fast_comma(&p) ||
        !fast_decimal(&p, &quote) || !fast_comma(&p) || !fast_uint(&p, &timestamp) ||
        !fast_comma(&p) || !fast_bool(&p, &buyer_maker) || !fast_comma(&p) ||
        !fast_bool(&p, &best_match)) return -1;
    fast_spaces(&p);
    if (*p != '\0' || id > LLONG_MAX || timestamp > LLONG_MAX || parsed_price <= 0.0 || parsed_qty <= 0.0 || quote < 0.0) return -1;
    *ts = (long long)timestamp; *price = parsed_price; *qty = parsed_qty;
    *is_buy = buyer_maker ? 0 : 1;
    (void)best_match;
    return 1;
}

/* Read and parse successive rows, tolerating a first-line header and CRLF. */
int fast_read_trade(FILE *fp, long long *ts, double *price, double *qty, int *is_buy)
{
    static FILE *stream; static int first = 1;
    char line[FAST_LINE];
    if (fp == NULL) return -1;
    if (stream != fp) { stream = fp; first = 1; }
    while (fgets(line, sizeof line, fp) != NULL) {
        size_t length = strlen(line);
        while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) line[--length] = '\0';
        int parsed = fast_parse_line(line, ts, price, qty, is_buy);
        if (first) { first = 0; if (parsed == 0) continue; }
        if (parsed < 0) { stream = NULL; return -1; }
        if (parsed == 0) { stream = NULL; return -1; }
        return 1;
    }
    stream = NULL;
    return ferror(fp) ? -1 : 0;
}
