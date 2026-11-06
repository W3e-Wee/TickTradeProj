/* Compare reference and manual parsers on headers, CRLF, decimals, and booleans. */
#include "check.h"
#include "fast_parser.h"
#include "reader.h"
#include <stdio.h>
#include <string.h>
/* Verify both APIs preserve every exposed field from the same fixture row. */
int main(void)
{
    const char *header = "tradeId,price,qty,quoteQty,time,isBuyerMaker,isBestMatch\r\n";
    const char *line = "7,65012.12345678,0.0015,97.51818518,1000001,tRuE,fAlSe\r\n";
    long long ta = 0, tb = 0; double pa = 0, pb = 0, qa = 0, qb = 0; int ba = 0, bb = 1;
    CHECK(fast_parse_line(header, &tb, &pb, &qb, &bb) == 0);
    CHECK(fast_parse_line(line, &tb, &pb, &qb, &bb) == 1);
    CHECK(tb == 1000001 && bb == 0);
    CHECK_NEAR(pb, 65012.12345678, 1e-9); CHECK_NEAR(qb, 0.0015, 1e-12);
    FILE *file = tmpfile(); CHECK(file != NULL);
    if (file != NULL) {
        CHECK(fwrite(header, 1, strlen(header), file) == strlen(header));
        CHECK(fwrite(line, 1, strlen(line), file) == strlen(line));
        rewind(file);
        CHECK(read_trade(file, &ta, &pa, &qa, &ba) == 1);
        CHECK(ta == tb && ba == bb); CHECK_NEAR(pa, pb, 1e-9); CHECK_NEAR(qa, qb, 1e-12);
        CHECK(read_trade(file, &ta, &pa, &qa, &ba) == 0);
        CHECK(fclose(file) == 0);
    }
    file = tmpfile(); CHECK(file != NULL);
    if (file != NULL) {
        CHECK(fwrite(header, 1, strlen(header), file) == strlen(header));
        CHECK(fwrite(line, 1, strlen(line), file) == strlen(line));
        rewind(file);
        CHECK(fast_read_trade(file, &ta, &pa, &qa, &ba) == 1);
        CHECK(ta == tb && ba == bb); CHECK_NEAR(pa, pb, 1e-9); CHECK_NEAR(qa, qb, 1e-12);
        CHECK(fast_read_trade(file, &ta, &pa, &qa, &ba) == 0);
        CHECK(fclose(file) == 0);
    }
    return CHECK_SUMMARY();
}
