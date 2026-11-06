/* C data profiler for decimal scales, ranges, timestamps, and fixed-point budget.
 * Usage: data_profile --output PATH CSV [CSV ...]; exits 2 for arguments, 6 for I/O.
 */
#include "exit_codes.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROFILE_LINE 512
#define PROFILE_FIELDS 7

typedef struct {
    uint64_t rows;
    double min_price, max_price, min_qty, max_qty;
    int price_decimals, qty_decimals;
    int64_t min_time, max_time;
} Profile;

/* Count decimal digits in a token while ignoring exponent notation. */
static int decimal_places(const char *value)
{
    const char *dot = strchr(value, '.');
    if (dot == NULL) return 0;
    int count = 0;
    for (++dot; *dot >= '0' && *dot <= '9'; ++dot) ++count;
    return count;
}

/* Split one mutable CSV line into the required seven plain fields. */
static int split_csv(char *line, char *fields[PROFILE_FIELDS])
{
    size_t count = 0;
    char *cursor = line;
    while (count < PROFILE_FIELDS) {
        fields[count++] = cursor;
        char *comma = strchr(cursor, ',');
        if (comma == NULL) break;
        *comma = '\0'; cursor = comma + 1;
    }
    return count == PROFILE_FIELDS && strchr(fields[PROFILE_FIELDS - 1], ',') == NULL;
}

/* Scan one data file and maintain ranges and precision from actual input rows. */
static int scan_file(const char *path, Profile *profile)
{
    FILE *file = fopen(path, "rb");
    char line[PROFILE_LINE];
    if (file == NULL) { perror(path); return 0; }
    profile->rows = 0;
    profile->min_price = profile->min_qty = 1.0e300;
    profile->max_price = profile->max_qty = 0.0;
    profile->min_time = INT64_MAX; profile->max_time = 0;
    profile->price_decimals = profile->qty_decimals = 0;
    while (fgets(line, sizeof line, file) != NULL) {
        char *fields[PROFILE_FIELDS];
        long long id, timestamp;
        double price, qty, quote;
        char *end;
        size_t length = strlen(line);
        while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) line[--length] = '\0';
        if (!split_csv(line, fields)) {
            if (profile->rows == 0 && (line[0] < '0' || line[0] > '9')) continue;
            fprintf(stderr, "%s: malformed CSV row\n", path); fclose(file); return 0;
        }
        id = strtoll(fields[0], &end, 10);
        if (end == fields[0] || *end != '\0') {
            if (profile->rows == 0) continue;
            fprintf(stderr, "%s: invalid trade ID\n", path); fclose(file); return 0;
        }
        price = strtod(fields[1], &end); if (end == fields[1] || *end != '\0') { fclose(file); return 0; }
        qty = strtod(fields[2], &end); if (end == fields[2] || *end != '\0') { fclose(file); return 0; }
        quote = strtod(fields[3], &end); if (end == fields[3] || *end != '\0') { fclose(file); return 0; }
        timestamp = strtoll(fields[4], &end, 10); if (end == fields[4] || *end != '\0') { fclose(file); return 0; }
        (void)id; (void)quote;
        if (price < profile->min_price) profile->min_price = price;
        if (price > profile->max_price) profile->max_price = price;
        if (qty < profile->min_qty) profile->min_qty = qty;
        if (qty > profile->max_qty) profile->max_qty = qty;
        if (timestamp < profile->min_time) profile->min_time = timestamp;
        if (timestamp > profile->max_time) profile->max_time = timestamp;
        int pd = decimal_places(fields[1]), qd = decimal_places(fields[2]);
        if (pd > profile->price_decimals) profile->price_decimals = pd;
        if (qd > profile->qty_decimals) profile->qty_decimals = qd;
        ++profile->rows;
    }
    int read_failed = ferror(file);
    int close_failed = fclose(file) != 0;
    int failed = read_failed || close_failed;
    return !failed && profile->rows > 0;
}

/* Print one file's measured ranges and candidate scale overflow budget. */
static int print_profile(FILE *output, const char *path, const Profile *profile)
{
    long double price_scale = 1.0L, qty_scale = 1.0L;
    for (int i = 0; i < profile->price_decimals; ++i) price_scale *= 10.0L;
    for (int i = 0; i < profile->qty_decimals; ++i) qty_scale *= 10.0L;
    long double single = (long double)profile->max_price * price_scale * (long double)profile->max_qty * qty_scale;
    long double total = single * (long double)profile->rows;
    return fprintf(output,
        "file=%s\nrows=%" PRIu64 "\nprice_min=%.12g\nprice_max=%.12g\nqty_min=%.12g\nqty_max=%.12g\nprice_decimals=%d\nqty_decimals=%d\ntime_min_us=%" PRId64 "\ntime_max_us=%" PRId64 "\nPRICE_SCALE=%.0f\nQTY_SCALE=%.0f\nsingle_scaled_product_approx=%.8e\nfull_file_scaled_numerator_approx=%.8e\nsingle_fits_int64=%s\nfull_file_fits_int64=%s\n\n",
        path, profile->rows, profile->min_price, profile->max_price, profile->min_qty,
        profile->max_qty, profile->price_decimals, profile->qty_decimals,
        profile->min_time, profile->max_time, (double)price_scale, (double)qty_scale, (double)single, (double)total,
        single <= 9.2e18L ? "yes" : "no", total <= 9.2e18L ? "yes" : "no") >= 0;
}

/* Parse output path and scan each requested file into one measurement log. */
int main(int argc, char **argv)
{
    const char *output_path = NULL;
    int first_file = 1;
    if (argc >= 4 && strcmp(argv[1], "--output") == 0) { output_path = argv[2]; first_file = 3; }
    else { fprintf(stderr, "Usage: %s --output PATH CSV [CSV ...]\n", argv[0]); return EXIT_BAD_INPUT; }
    FILE *output = fopen(output_path, "wb");
    if (output == NULL) { perror(output_path); return EXIT_IO_ERROR; }
    for (int i = first_file; i < argc; ++i) {
        Profile profile;
        if (!scan_file(argv[i], &profile) || !print_profile(output, argv[i], &profile)) {
            fprintf(stderr, "Profile failed for %s\n", argv[i]); fclose(output); return EXIT_IO_ERROR;
        }
    }
    return fclose(output) == 0 ? EXIT_OK : EXIT_IO_ERROR;
}
