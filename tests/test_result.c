/* Check stable RESULT output, FNV hashing, and header-once CSV behavior. */
#include "check.h"
#include "result.h"
#include <string.h>
/* Exercise output helpers using a temporary C stream. */
int main(void)
{
    FILE *file = tmpfile();
    int written = 0;
    char buffer[256] = {0};
    uint64_t hash = UINT64_C(14695981039346656037);
    RunResult result = {"v0", "test.csv", 20, 3, 5, 100.75, 4, 1, 1, 0, 2, 3, 5, 6};
    CHECK(file != NULL);
    CHECK(result_csv_header_once(file, &written, "a,b"));
    CHECK(result_csv_header_once(file, &written, "a,b"));
    CHECK(result_print(file, &result));
    CHECK(fseek(file, 0, SEEK_SET) == 0);
    CHECK(fgets(buffer, sizeof buffer, file) != NULL && strcmp(buffer, "a,b\n") == 0);
    CHECK(fgets(buffer, sizeof buffer, file) != NULL && strncmp(buffer, "RESULT version=v0", 17) == 0);
    hash = result_hash_signal(hash, 7, 1);
    CHECK(hash != UINT64_C(14695981039346656037));
    CHECK(fclose(file) == 0);
    return CHECK_SUMMARY();
}
