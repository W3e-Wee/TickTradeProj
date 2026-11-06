/* Locale-independent hand-written parser for Binance trade CSV rows. */
#ifndef TICKTRADE_FAST_PARSER_H
#define TICKTRADE_FAST_PARSER_H
#include <stdio.h>
/* Parse one line: 1 row, 0 nonnumeric header, negative malformed input. */
int fast_parse_line(const char *line, long long *ts, double *price,
                    double *qty, int *is_buy);
/* Read rows using the fast parser with the common 1/0/negative convention. */
int fast_read_trade(FILE *fp, long long *ts, double *price, double *qty, int *is_buy);
#endif
