/* Minimal assertion helpers for dependency-free C unit tests. */
#ifndef TICKTRADE_CHECK_H
#define TICKTRADE_CHECK_H
#include <math.h>
#include <stdio.h>
static int check_failures;
/* Record an assertion failure without stopping later test cases. */
static void check_record(int condition, const char *expression, const char *file, int line)
{
    if (!condition) {
        fprintf(stderr, "FAIL %s:%d: %s\n", file, line, expression);
        ++check_failures;
    }
}
#define CHECK(expr) check_record((expr) != 0, #expr, __FILE__, __LINE__)
#define CHECK_NEAR(a, b, tolerance) check_record(fabs((a) - (b)) <= (tolerance), #a " ~= " #b, __FILE__, __LINE__)
#define CHECK_SUMMARY() (fprintf(check_failures ? stderr : stdout, "%s (%d failures)\n", check_failures ? "FAIL" : "PASS", check_failures), check_failures != 0)
#endif
