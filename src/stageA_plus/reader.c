/* Buffered reference CSV reader; header recognition applies to each open stream. */
#include "reader.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define READER_LINE 512
static FILE *reader_stream;
static int reader_first_row;

/* Decode True/False case-insensitively into the course is_buy convention. */
static int reader_boolean(const char *text, int *is_buy)
{
    char normalized[6]; size_t length = strlen(text);
    if (length >= sizeof normalized) return 0;
    for (size_t i = 0; i <= length; ++i) normalized[i] = (char)tolower((unsigned char)text[i]);
    if (strcmp(normalized, "false") == 0) { *is_buy = 1; return 1; }
    if (strcmp(normalized, "true") == 0) { *is_buy = 0; return 1; }
    return 0;
}

/* Parse one sscanf reference row after header detection. */
static int reader_parse(const char *line, long long *ts, double *price,
                        double *qty, int *is_buy)
{
    long long id, timestamp;
    double parsed_price, parsed_qty, quote;
    char side[16], best[16], tail;
    int fields = sscanf(line, " %lld , %lf , %lf , %lf , %lld , %15[^,] , %15s %c",
                        &id, &parsed_price, &parsed_qty, &quote, &timestamp,
                        side, best, &tail);
    if (fields != 7 || id < 0 || timestamp < 0 || parsed_price <= 0.0 || parsed_qty <= 0.0 || quote < 0.0) return -1;
    int ignored_best;
    if (!reader_boolean(side, is_buy) || !reader_boolean(best, &ignored_best)) return -1;
    *ts = timestamp; *price = parsed_price; *qty = parsed_qty;
    return 1;
}

/* Read lines until a parsed row, EOF, or malformed record is encountered. */
int read_trade(FILE *fp, long long *ts, double *price, double *qty, int *is_buy)
{
    char line[READER_LINE];
    if (fp == NULL || ts == NULL || price == NULL || qty == NULL || is_buy == NULL) return -1;
    if (reader_stream != fp) { reader_stream = fp; reader_first_row = 1; }
    while (fgets(line, sizeof line, fp) != NULL) {
        size_t length = strlen(line);
        while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) line[--length] = '\0';
        if (reader_first_row) {
            const unsigned char *first = (const unsigned char *)line;
            while (isspace(*first)) ++first;
            reader_first_row = 0;
            if (*first < '0' || *first > '9') continue;
        }
        int status = reader_parse(line, ts, price, qty, is_buy);
        if (status < 0) { reader_stream = NULL; return -1; }
        return status;
    }
    reader_stream = NULL;
    return ferror(fp) ? -1 : 0;
}
