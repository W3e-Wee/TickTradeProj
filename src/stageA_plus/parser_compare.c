/* Compare sscanf and fast Stage A+ parsers and record real parse timings.
 * Usage: parser_compare --file CSV [--repeat N]; exit codes follow exit_codes.h.
 */
#include "exit_codes.h"
#include "fast_parser.h"
#include "reader.h"
#include "timing.h"
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PARSER_MAX_REPEATS 100U

/* Sort run durations for a median over the required repeated parses. */
static void parser_sort(uint64_t *values, unsigned count)
{
    for (unsigned i = 1; i < count; ++i) {
        uint64_t value = values[i]; unsigned j = i;
        while (j > 0 && values[j - 1] > value) { values[j] = values[j - 1]; --j; }
        values[j] = value;
    }
}

/* Compare every exposed field between independent streams, row by row. */
static int compare_all(const char *path, uint64_t *rows_out)
{
    FILE *reference = fopen(path, "rb"), *fast = fopen(path, "rb");
    uint64_t rows = 0; int ok = 1;
    if (reference == NULL || fast == NULL) { if (reference) fclose(reference); if (fast) fclose(fast); return 0; }
    for (;;) {
        long long rt = 0, ft = 0; double rp = 0, fp = 0, rq = 0, fq = 0; int rb = 0, fb = 0;
        int rs = read_trade(reference, &rt, &rp, &rq, &rb);
        int fs = fast_read_trade(fast, &ft, &fp, &fq, &fb);
        if (rs != fs) { ok = 0; break; }
        if (rs == 0) break;
        if (rs < 0 || rt != ft || rb != fb || fabs(rp - fp) > 1e-9 || fabs(rq - fq) > 1e-9) { ok = 0; break; }
        ++rows;
    }
    if (fclose(reference) != 0 || fclose(fast) != 0) ok = 0;
    *rows_out = rows;
    return ok;
}

/* Time one parser over a full data file, including streaming input reads. */
static int parser_scan(const char *path, int use_fast, uint64_t *elapsed, uint64_t *rows_out)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL) return 0;
    uint64_t rows = 0, start = now_ns();
    for (;;) {
        long long ts; double price, qty; int is_buy;
        int status = use_fast ? fast_read_trade(file, &ts, &price, &qty, &is_buy)
                              : read_trade(file, &ts, &price, &qty, &is_buy);
        if (status == 0) break;
        if (status < 0) { fclose(file); return 0; }
        ++rows;
    }
    *elapsed = now_ns() - start;
    *rows_out = rows;
    return fclose(file) == 0;
}

/* Append one parser comparison row, writing a heading for a new file only. */
static int write_result(const char *path, uint64_t rows, unsigned repeats,
                        uint64_t reference_ns, uint64_t fast_ns)
{
    const char *csv = "results/bench/parser.csv";
    FILE *probe = fopen(csv, "rb");
    int header = probe == NULL;
    if (probe != NULL && fclose(probe) != 0) return 0;
    FILE *output = fopen(csv, "ab");
    if (output == NULL) return 0;
    int ok = 1;
    if (header) ok = fprintf(output, "file,rows,repeats,sscanf_median_ns,fast_median_ns,speedup\n") >= 0;
    if (ok) ok = fprintf(output, "%s,%" PRIu64 ",%u,%" PRIu64 ",%" PRIu64 ",%.6f\n",
                         path, rows, repeats, reference_ns, fast_ns,
                         fast_ns == 0 ? 0.0 : (double)reference_ns / fast_ns) >= 0;
    return fclose(output) == 0 && ok;
}

/* Parse the command line, prove row-level equality, and report median timings. */
int main(int argc, char **argv)
{
    const char *path = NULL; unsigned repeats = 5;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0) { printf("Usage: %s --file CSV [--repeat N]\n", argv[0]); return EXIT_OK; }
        if (i + 1 >= argc) return EXIT_BAD_INPUT;
        if (strcmp(argv[i], "--file") == 0) path = argv[++i];
        else if (strcmp(argv[i], "--repeat") == 0) {
            char *end = NULL; unsigned long value = strtoul(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || value < 5 || value > PARSER_MAX_REPEATS) return EXIT_BAD_INPUT;
            repeats = (unsigned)value;
        } else return EXIT_BAD_INPUT;
    }
    if (path == NULL) return EXIT_BAD_INPUT;
    uint64_t rows = 0, reference_rows = 0, fast_rows = 0;
    if (!compare_all(path, &rows)) { fprintf(stderr, "Parser field mismatch: %s\n", path); return EXIT_MISMATCH; }
    uint64_t reference_times[PARSER_MAX_REPEATS], fast_times[PARSER_MAX_REPEATS];
    for (unsigned i = 0; i < repeats; ++i) {
        if (!parser_scan(path, 0, &reference_times[i], &reference_rows) ||
            !parser_scan(path, 1, &fast_times[i], &fast_rows) || reference_rows != rows || fast_rows != rows) return EXIT_IO_ERROR;
    }
    parser_sort(reference_times, repeats); parser_sort(fast_times, repeats);
    uint64_t ref_median = reference_times[repeats / 2], fast_median = fast_times[repeats / 2];
    printf("parser rows=%" PRIu64 " repeats=%u sscanf_median_ns=%" PRIu64 " fast_median_ns=%" PRIu64 " speedup=%.6f\n",
           rows, repeats, ref_median, fast_median, fast_median == 0 ? 0.0 : (double)ref_median / fast_median);
    return write_result(path, rows, repeats, ref_median, fast_median) ? EXIT_OK : EXIT_IO_ERROR;
}
