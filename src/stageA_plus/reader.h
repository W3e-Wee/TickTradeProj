/* Course-style sscanf parser used as the reference for Stage A+ comparisons. */
#ifndef TICKTRADE_READER_H
#define TICKTRADE_READER_H
#include <stdio.h>
/* Read timestamp, price, quantity, and side: 1 row, 0 EOF, negative error. */
int read_trade(FILE *fp, long long *ts, double *price, double *qty, int *is_buy);
#endif
