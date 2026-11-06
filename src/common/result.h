/* Shared machine-readable result record, signal hashing, and CSV writer. */
#ifndef TICKTRADE_RESULT_H
#define TICKTRADE_RESULT_H
#include <stdint.h>
#include <stdio.h>

typedef struct {
    const char *version;
    const char *file;
    uint64_t trades;
    int short_window;
    int long_window;
    double vwap;
    uint64_t bars;
    uint64_t buys;
    uint64_t sells;
    uint64_t signal_hash;
    uint64_t load_ns;
    uint64_t process_ns;
    uint64_t total_ns;
    uint64_t peak_mem_kb;
} RunResult;
/* Update FNV-1a with an index encoded in eight little-endian bytes and a side. */
uint64_t result_hash_signal(uint64_t hash, uint64_t index, int side);
/* Print the exact machine-readable RESULT contract to the selected stream. */
int result_print(FILE *stream, const RunResult *result);
/* Write a CSV header once per output stream using caller-owned state. */
int result_csv_header_once(FILE *stream, int *written, const char *header);

#endif
