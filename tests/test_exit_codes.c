/* Keep the shared error-code contract stable for every executable. */
#include "check.h"
#include "exit_codes.h"
/* Verify all specified process return values exactly. */
int main(void)
{
    CHECK(EXIT_OK == 0 && EXIT_BAD_INPUT == 2 && EXIT_OUT_OF_RANGE == 3);
    CHECK(EXIT_MISMATCH == 4 && EXIT_SELF_CHECK_FAIL == 5 && EXIT_IO_ERROR == 6);
    CHECK(EXIT_UNSUPPORTED == 7);
    return CHECK_SUMMARY();
}
