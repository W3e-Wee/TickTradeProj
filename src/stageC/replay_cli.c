/* Timestamp-driven replay with preallocated lag samples. */
#include "fast_parser.h"
#include "latency.h"
#include "platform.h"
#include "replay.h"
#include "timing.h"
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int count_rows(const char *path, size_t *count)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL) return 0;
    size_t n = 0;
    for (;;) {
        long long ts; double price, qty; int side;
        int status = fast_read_trade(file, &ts, &price, &qty, &side);
        if (status == 0) break;
        if (status < 0 || n == SIZE_MAX) { fclose(file); return 0; }
        ++n;
    }
    int ok = !ferror(file) && fclose(file) == 0;
    *count = n;
    return ok;
}

/* Run replay at a positive integer speed and summarize nonnegative schedule lag. */
int main(int argc, char **argv)
{
    const char *path = NULL; unsigned speed = 1;
    for (int i = 1; i < argc; ++i) {
        if (i + 1 >= argc) return 2;
        const char *key = argv[i++], *value = argv[i];
        if (strcmp(key, "--file") == 0) path = value;
        else if (strcmp(key, "--speed") == 0) {
            char *end = NULL; unsigned long parsed = strtoul(value, &end, 10);
            if (end == value || *end != '\0' || parsed == 0 || parsed > UINT_MAX) return 2;
            speed = (unsigned)parsed;
        } else return 2;
    }
    if (path == NULL) return 2;
    size_t count = 0;
    if (!count_rows(path, &count) || count == 0 || count > SIZE_MAX / sizeof(uint64_t)) return 6;
    uint64_t *storage = (uint64_t *)malloc(count * sizeof *storage);
    if (storage == NULL) return 6;
    LatencySamples samples;
    if (!latency_init(&samples, storage, count)) { free(storage); return 6; }
    FILE *file = fopen(path, "rb");
    if (file == NULL) { free(storage); return 6; }
    long long ts, first_ts = 0; double price, qty; int side;
    uint64_t start = now_ns(); size_t seen = 0; int status;
    while ((status = fast_read_trade(file, &ts, &price, &qty, &side)) > 0) {
        if (seen == 0) first_ts = ts;
        uint64_t release;
        if (!replay_release_ns(start, first_ts, ts, speed, &release)) { status = -1; break; }
        replay_wait_until(release);
        if (!latency_push(&samples, replay_lag_ns(now_ns(), release))) { status = -1; break; }
        ++seen;
    }
    int close_ok = !ferror(file) && fclose(file) == 0;
    LatencySummary summary;
    if (status < 0 || !close_ok || seen != count || !latency_summarize(&samples, &summary)) {
        free(storage); return 6;
    }
    printf("REPLAY speed=%u trades=%zu avg_lag_ns=%.2f p50_ns=%" PRIu64 " p99_ns=%" PRIu64 " worst_ns=%" PRIu64 "\nPASS\n",
           speed, seen, summary.average_ns, summary.p50_ns, summary.p99_ns, summary.worst_ns);
    free(storage);
    return 0;
}
