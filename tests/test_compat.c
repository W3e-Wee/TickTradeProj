/* Check portable compiler-feature macros have valid safe fallback forms. */
#include "check.h"
#include "compat.h"
/* Compile and execute guarded/noinline declarations on the active compiler. */
int main(void)
{
    int value = 3;
    COMPAT_OPTIMIZER_GUARD(value);
    CHECK(value == 3);
    CHECK(COMPAT_HAS_INT128 == 0 || COMPAT_HAS_INT128 == 1);
    return CHECK_SUMMARY();
}
