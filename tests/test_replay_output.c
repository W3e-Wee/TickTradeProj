/* Check replay CSV headers and row widths without platform-specific helpers. */
#include <stdio.h>
#include <string.h>

static const char expected_header[] =
    "platform,member,core,speed,trades_replayed,market_seconds,wall_seconds,"
    "avg_latency_ns,worst_latency_ns,p50_latency_ns,p99_latency_ns,"
    "avg_processing_ns,max_lag_us,pct_behind,dropped_events,buys,sells,"
    "pnl_vs_rival_r1";

static int replay_csv_valid(const char *path)
{
    char line[1024];
    FILE *file = fopen(path, "rb");
    if (file == NULL) return 0;
    if (fgets(line, sizeof line, file) == NULL) { fclose(file); return 0; }
    line[strcspn(line, "\r\n")] = '\0';
    if (strcmp(line, expected_header) != 0) { fclose(file); return 0; }
    unsigned rows = 0;
    while (fgets(line, sizeof line, file) != NULL) {
        unsigned columns = 1;
        for (char *cursor = line; *cursor != '\0'; ++cursor)
            if (*cursor == ',') ++columns;
        if (columns != 18U) { fclose(file); return 0; }
        ++rows;
    }
    int read_failed = ferror(file);
    int close_failed = fclose(file) != 0;
    int ok = !read_failed && !close_failed && rows > 0U;
    return ok;
}

int main(int argc, char **argv)
{
    if (argc < 2) return 2;
    for (int i = 1; i < argc; ++i) {
        if (!replay_csv_valid(argv[i])) {
            fprintf(stderr, "replay CSV schema check failed: %s\n", argv[i]);
            return 1;
        }
    }
    puts("PASS replay CSV schema");
    return 0;
}
