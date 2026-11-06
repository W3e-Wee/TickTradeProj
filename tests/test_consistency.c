/* Compare each ladder configuration and optionally verify the supplied oracle. */
#include "engine.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { double vwap; uint64_t trades, bars, buys, sells; int complete; } Expected;
static const EngineConfig configs[] = {
    {"v0", 0, 0, 0, 0}, {"v1", 0, 0, 0, 0}, {"v2", 0, 0, 0, 0},
    {"v3", 0, 0, 0, 0}, {"v4", 1, 0, 0, 0}, {"v5", 1, 1, 0, 0}, {"v6", 1, 0, 1, 0},
    {"v7", 1, 0, 1, 1}, {"v8", 1, 0, 1, 2}
};

/* Read final oracle summary values directly from the supplied expected text. */
static int read_expected(const char *path, Expected *expected)
{
    FILE *file = fopen(path, "rb"); char line[512]; int bars_section = 0, table_header = 0;
    if (file == NULL) return 0;
    memset(expected, 0, sizeof *expected);
    while (fgets(line, sizeof line, file) != NULL) {
        if (sscanf(line, "Final VWAP: %lf", &expected->vwap) == 1) expected->complete |= 1;
        if (sscanf(line, "Signals: %llu BUY, %llu SELL",
                   (unsigned long long *)&expected->buys,
                   (unsigned long long *)&expected->sells) == 2) expected->complete |= 2;
        if (strstr(line, "One-second OHLC bars") != NULL) bars_section = 1;
        else if (bars_section && strstr(line, "second") != NULL) table_header = 1;
        else if (bars_section && table_header) {
            long long second; double open, high, low, close; unsigned long long count;
            int fields = sscanf(line, "%lld %lf %lf %lf %lf %llu", &second, &open, &high, &low, &close, &count);
            if (fields == 6) ++expected->bars;
        }
    }
    int failed = ferror(file); int closed = fclose(file) == 0;
    return !failed && closed && expected->complete == 3;
}

/* Parse required input and run every C engine configuration for comparison. */
int main(int argc, char **argv)
{
    const char *path = NULL, *expected_path = NULL; int short_window = 3, long_window = 5;
    for (int i = 1; i < argc; ++i) {
        if (i + 1 >= argc) return 2;
        const char *key = argv[i++], *value = argv[i];
        if (strcmp(key, "--file") == 0) path = value;
        else if (strcmp(key, "--expected") == 0) expected_path = value;
        else if (strcmp(key, "--short") == 0) short_window = atoi(value);
        else if (strcmp(key, "--long") == 0) long_window = atoi(value);
        else return 2;
    }
    if (path == NULL || short_window <= 0 || long_window < short_window) return 2;
    RunResult baseline; int failed = 0;
    if (!engine_execute(path, short_window, long_window, &configs[0], &baseline)) return 6;
    for (size_t i = 0; i < sizeof configs / sizeof configs[0]; ++i) {
        RunResult current;
        int ok = engine_execute(path, short_window, long_window, &configs[i], &current);
        if (!ok) { printf("%s FAIL\n", configs[i].version); failed = 1; continue; }
        int same = current.trades == baseline.trades && current.bars == baseline.bars &&
            current.buys == baseline.buys && current.sells == baseline.sells &&
            current.signal_hash == baseline.signal_hash &&
            fabs(current.vwap - baseline.vwap) <= fabs(baseline.vwap) * 1e-6;
        printf("%s %s\n", configs[i].version, same ? "PASS" : "FAIL");
        if (!same) printf("  trades=%llu/%llu bars=%llu/%llu buys=%llu/%llu sells=%llu/%llu hash=%016llx/%016llx vwap=%.9f/%.9f\n",
            (unsigned long long)current.trades, (unsigned long long)baseline.trades,
            (unsigned long long)current.bars, (unsigned long long)baseline.bars,
            (unsigned long long)current.buys, (unsigned long long)baseline.buys,
            (unsigned long long)current.sells, (unsigned long long)baseline.sells,
            (unsigned long long)current.signal_hash, (unsigned long long)baseline.signal_hash,
            current.vwap, baseline.vwap);
        if (!same) failed = 1;
    }
    if (expected_path != NULL) {
        Expected expected;
        if (!read_expected(expected_path, &expected)) { fprintf(stderr, "Unable to parse expected summary.\n"); return 6; }
        int same = baseline.vwap == baseline.vwap && fabs(baseline.vwap - expected.vwap) <= fabs(expected.vwap) * 1e-6 &&
            baseline.buys == expected.buys && baseline.sells == expected.sells && baseline.bars == expected.bars;
        printf("expected %s\n", same ? "PASS" : "FAIL");
        if (!same) failed = 1;
    }
    return failed ? 1 : 0;
}
