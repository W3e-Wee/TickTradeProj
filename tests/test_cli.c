/* Exercise strict numeric parsing shared by project command-line interfaces. */
#include "check.h"
#include "cli.h"

/* Verify valid and invalid unsigned values and bounded positive windows. */
int main(void)
{
    uint64_t parsed = 0;
    int window = 0;
    CHECK(cli_parse_u64("0", &parsed) && parsed == 0);
    CHECK(cli_parse_u64("123", &parsed) && parsed == 123);
    CHECK(!cli_parse_u64("-5", &parsed));
    CHECK(!cli_parse_u64("12abc", &parsed));
    CHECK(!cli_parse_u64("", &parsed));
    CHECK(cli_parse_positive_int("5", 100, &window) && window == 5);
    CHECK(!cli_parse_positive_int("0", 100, &window));
    CHECK(!cli_parse_positive_int("101", 100, &window));
    return CHECK_SUMMARY();
}
