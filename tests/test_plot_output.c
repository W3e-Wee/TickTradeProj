/* Validate registered plot inputs, logs, PNGs, summaries, and plot-specific data. */
#include "check.h"
#include "plot_registry.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef LADDER_SOURCE
#define LADDER_SOURCE "results/bench/ladder_linux_member.csv"
#endif

typedef struct { char *data; size_t size; } Blob;
typedef struct { char value[256]; } CsvCell;
static int quiet_expected_failures;

/* Read a complete file in binary mode so line-ending and PNG checks are portable. */
static Blob read_blob(const char *path)
{
    Blob b = {NULL, 0}; FILE *f = fopen(path, "rb");
    if (!f) return b;
    if (fseek(f, 0, SEEK_END) == 0) { long n = ftell(f); if (n >= 0 && fseek(f, 0, SEEK_SET) == 0) {
        b.data = (char *)malloc((size_t)n + 1); if (b.data) { b.size = fread(b.data, 1, (size_t)n, f); b.data[b.size] = '\0'; }
    }}
    fclose(f); return b;
}

/* Split one CSV row while handling quoted fields and escaped quote characters. */
static size_t split_csv(const char *line, CsvCell *cells, size_t max_cells)
{
    size_t count = 0, out = 0; int quoted = 0;
    if (!line || !cells || max_cells == 0) return 0;
    memset(cells, 0, max_cells * sizeof *cells);
    for (const char *p = line; ; ++p) {
        char c = *p;
        if (c == '"') { if (quoted && p[1] == '"') { if (out + 1 < sizeof cells[0].value) cells[count].value[out++] = '"'; ++p; } else quoted = !quoted; }
        else if ((c == ',' && !quoted) || c == '\0' || c == '\n' || c == '\r') {
            if (count < max_cells) { cells[count].value[out] = '\0'; ++count; }
            out = 0; if (c != ',') break;
        } else if (count < max_cells && out + 1 < sizeof cells[0].value) cells[count].value[out++] = c;
    }
    return count;
}

/* Return whether a text value parses as a finite number. */
static int finite_number(const char *value, double *number)
{
    char *end = NULL; double parsed;
    if (!value || !*value) return 0;
    parsed = strtod(value, &end);
    while (end && isspace((unsigned char)*end)) ++end;
    if (!end || end == value || *end || !isfinite(parsed)) return 0;
    if (number) *number = parsed;
    return 1;
}

/* Find a named CSV column in the parsed header. */
static int column_index(CsvCell *header, size_t count, const char *name)
{
    for (size_t i = 0; i < count; ++i) if (strcmp(header[i].value, name) == 0) return (int)i;
    return -1;
}

/* Scan a plot log for fatal gnuplot diagnostics while ignoring font notes. */
static int log_is_clean(const char *path)
{
    Blob b = read_blob(path); int clean = b.data != NULL;
    if (!b.data) return 0;
    size_t out = 0;
    for (size_t i = 0; i < b.size; ++i) if (b.data[i] != '\0') b.data[out++] = b.data[i];
    b.data[out] = '\0';
    char *p = b.data;
    if ((unsigned char)p[0] == 0xef && (unsigned char)p[1] == 0xbb && (unsigned char)p[2] == 0xbf) p += 3;
    else if ((unsigned char)p[0] == 0xff && (unsigned char)p[1] == 0xfe) p += 2;
    else if ((unsigned char)p[0] == 0xfe && (unsigned char)p[1] == 0xff) p += 2;
    for (char *line = p; *line;) {
        char *end = strchr(line, '\n'); if (end) *end = '\0';
        for (char *c = line; *c; ++c) *c = (char)tolower((unsigned char)*c);
        if (!strstr(line, "font") && (strstr(line, "error") || strstr(line, "unknown") || strstr(line, "invalid") ||
            strstr(line, "undefined") || strstr(line, "cannot") || strstr(line, "can't") || strstr(line, "bad data") ||
            strstr(line, "no data") || strstr(line, "all points y value undefined") || strstr(line, "warning: empty") ||
            strncmp(line, "error:", 6) == 0)) clean = 0;
        if (!end) break;
        line = end + 1;
    }
    free(b.data); return clean;
}

/* Validate each input's line endings, schema, row count, and plotted numeric values. */
static int validate_input(const PlotSpec *spec, const char *path, int second)
{
    Blob b = read_blob(path); CsvCell cells[32], header[32]; int good = 1; size_t rows = 0, headers = 0;
    const char *required = second ? "" : spec->headers;
    if (!b.data) { fprintf(stderr, "FAIL: missing plot input %s\n", path); return 0; }
    for (size_t i = 0; i < b.size; ++i) if (b.data[i] == '\r') good = 0;
    if (!good && !quiet_expected_failures) fprintf(stderr, "FAIL: carriage return in %s\n", path);
    char *line = b.data; char *next = strchr(line, '\n'); if (next) *next = '\0';
    if (*required) {
        headers = split_csv(line, header, 32);
        char wanted[512]; snprintf(wanted, sizeof wanted, "%s", required);
        for (char *name = strtok(wanted, ","); name; name = strtok(NULL, ",")) if (column_index(header, headers, name) < 0) {
            if (!quiet_expected_failures) fprintf(stderr, "FAIL: header column not found: %s in %s\n", name, path);
            good = 0;
        }
        line = next ? next + 1 : line + strlen(line) + 1;
    }
    while (line && *line) {
        next = strchr(line, '\n'); if (next) *next = '\0';
        if (*line) {
            size_t n = split_csv(line, cells, 32); ++rows;
            if (spec->name && strcmp(spec->name, "signals") == 0) {
                double a, b, c; int ti, si;
                if (second) { if (sscanf(line, "%lf %lf %d", &a, &b, &si) != 3 || !isfinite(a) || !isfinite(b) || (si != -1 && si != 1)) good = 0; }
                else if (sscanf(line, "%d %lf %lf %lf %d", &ti, &a, &b, &c, &si) != 5 || !isfinite(a) || !isfinite(b) || !isfinite(c) || (si != -1 && si != 0 && si != 1)) good = 0;
            } else {
                if (n != headers) good = 0;
                for (size_t i = 0; i < n; ++i) {
                    int text_column = (i == 0 || i == 1 || (strcmp(spec->name, "ladder") == 0 && (i == 2 || i == 3 || i >= 12)));
                    if (!text_column && !finite_number(cells[i].value, NULL)) good = 0;
                }
            }
        }
        if (!next) break;
        line = next + 1;
    }
    if (rows < (second ? 0u : (size_t)spec->minimum_rows)) { if (!quiet_expected_failures) fprintf(stderr, "FAIL: too few data rows in %s\n", path); good = 0; }
    free(b.data); return good;
}

/* Parse key/value summary output and return a numeric field. */
static int summary_value(const char *path, const char *key, double *value)
{
    Blob b = read_blob(path); int found = 0;
    if (!b.data) return 0;
    for (char *line = b.data; *line;) {
        char *end = strchr(line, '\n'); if (end) *end = '\0';
        size_t n = strlen(key);
        if (strncmp(line, key, n) == 0 && line[n] == '=' && finite_number(line + n + 1, value)) { found = 1; break; }
        if (!end) break;
        line = end + 1;
    }
    free(b.data); return found;
}

/* Confirm that a summary value matches the independently counted result. */
static int summary_equals(const char *path, const char *key, double expected)
{
    double actual; return summary_value(path, key, &actual) && fabs(actual - expected) <= 1e-9;
}

/* Check money sweep ordering, baseline-relative losses, and axis bounds. */
static int validate_money_sweep(const PlotSpec *spec, double rows, double xmin, double xmax, double ymin, double ymax)
{
    FILE *f = fopen(spec->input1, "rb"); char line[1024]; CsvCell h[32], c[32]; size_t hn, count = 0;
    int rcol, pcol, mcol; double previous = -INFINITY, base_pnl = 0, xlo = INFINITY, xhi = -INFINITY, ylo = INFINITY, yhi = -INFINITY;
    int saw01 = 0, saw1 = 0, saw100 = 0, ok = f != NULL;
    if (!f) return 0;
    if (!fgets(line, sizeof line, f)) { fclose(f); return 0; }
    hn = split_csv(line, h, 32); rcol = column_index(h, hn, "reaction_us"); pcol = column_index(h, hn, "pnl"); mcol = column_index(h, hn, "money_lost");
    if (rcol < 0 || pcol < 0 || mcol < 0) ok = 0;
    while (ok && fgets(line, sizeof line, f)) {
        size_t n = split_csv(line, c, 32); double reaction, pnl, lost;
        if ((size_t)rcol >= n || (size_t)pcol >= n || (size_t)mcol >= n || !finite_number(c[rcol].value,&reaction) || !finite_number(c[pcol].value,&pnl) || !finite_number(c[mcol].value,&lost)) { ok = 0; break; }
        if (count == 0) base_pnl = pnl;
        if (reaction <= previous || fabs(lost - (base_pnl - pnl)) > 1e-6) { ok = 0; break; }
        previous = reaction;
        if (fabs(reaction-.1)<1e-9) saw01=1;
        if (fabs(reaction-1)<1e-9) saw1=1;
        if (fabs(reaction-100)<1e-9) saw100=1;
        if (reaction < xlo) xlo = reaction;
        if (reaction > xhi) xhi = reaction;
        if (lost < ylo) ylo = lost;
        if (lost > yhi) yhi = lost;
        ++count;
    }
    fclose(f);
    return ok && count >= 25 && count == (size_t)rows && xlo > 0 && saw01 && saw1 && saw100 && xmin <= xlo && xmax >= xhi && ymin <= ylo && ymax >= yhi && summary_equals(spec->summary,"points_money_lost",(double)count);
}

/* Check ladder data includes each rung, positive timings, and bounded axes. */
static int validate_ladder_data(const PlotSpec *spec, double rows, double xmin, double xmax, double ymin, double ymax)
{
    FILE *f = fopen(spec->input1, "rb"); char line[2048]; CsvCell h[32], c[32]; size_t hn, count = 0; int tcol, vcol;
    double lo = INFINITY, hi = -INFINITY; int have[10] = {0}; int ok = f != NULL;
    if (!f) return 0;
    if (!fgets(line,sizeof line,f)) { fclose(f); return 0; }
    hn=split_csv(line,h,32); tcol=column_index(h,hn,"median_total_ns"); vcol=column_index(h,hn,"version"); if (tcol<0||vcol<0) ok=0;
    while (ok && fgets(line,sizeof line,f)) { size_t n=split_csv(line,c,32); double timing;
        if ((size_t)tcol>=n || (size_t)vcol>=n || !finite_number(c[tcol].value,&timing) || timing<=0) { ok=0; break; }
        if (strcmp(c[vcol].value,"v0")==0) have[0]=1; else if (strcmp(c[vcol].value,"v1")==0) have[1]=1;
        else if (strcmp(c[vcol].value,"v2")==0) have[2]=1; else if (strcmp(c[vcol].value,"v3")==0) have[3]=1;
        else if (strcmp(c[vcol].value,"v4")==0) have[4]=1; else if (strcmp(c[vcol].value,"v5")==0) have[5]=1;
        else if (strcmp(c[vcol].value,"v6")==0) have[6]=1; else if (strcmp(c[vcol].value,"v7")==0) have[7]=1;
        else if (strcmp(c[vcol].value,"v8")==0) have[8]=1; else if (strcmp(c[vcol].value,"v9_soa")==0) have[9]=1;
        if (timing < lo) lo = timing;
        if (timing > hi) hi = timing;
        ++count;
    }
    fclose(f); int all=1; for(size_t i=0;i<10;++i) all &= have[i];
    return ok && all && count==10 && rows==10 && xmin<=0 && xmax>=count-1 && ymin<=lo && ymax>=hi && summary_equals(spec->summary,"points_median_total_ns",(double)count);
}

/* Validate a PNG against its registered dimensions, signature, and size threshold. */
static int png_is_valid(const char *path, const PlotSpec *spec)
{
    Blob png = read_blob(path); int valid = png.data != NULL;
    const unsigned char signature[8] = {137,80,78,71,13,10,26,10};
    if (!valid) return 0;
    if (png.size < 24 || memcmp(png.data, signature, 8) != 0 || png.size <= (size_t)spec->minimum_png_bytes) valid = 0;
    if (png.size >= 24) { unsigned char *p = (unsigned char *)png.data;
        unsigned w = (p[16]<<24)|(p[17]<<16)|(p[18]<<8)|p[19], h=(p[20]<<24)|(p[21]<<16)|(p[22]<<8)|p[23];
        if (w != spec->width || h != spec->height) valid = 0;
    }
    free(png.data); return valid;
}

/* Copy the selected platform/member ladder CSV into the plot's fixed input path. */
static int prepare_ladder(void)
{
    FILE *in = fopen(LADDER_SOURCE, "rb"), *out; int ok = 1; char header_line[4096], line[4096], version_rows[10][4096] = {{0}}; CsvCell header[32], cells[32]; size_t count;
    if (!in) { (void)remove("results/bench/ladder_plot_input.csv"); return 0; }
    if (!fgets(header_line, sizeof header_line, in)) { fclose(in); return 0; }
    count = split_csv(header_line, header, 32);
    int version_col = column_index(header, count, "version");
    while (version_col >= 0 && fgets(line, sizeof line, in)) {
        size_t fields = split_csv(line, cells, 32); int slot = -1;
        if ((size_t)version_col < fields) {
            if (strcmp(cells[version_col].value,"v0")==0) slot=0; else if (strcmp(cells[version_col].value,"v1")==0) slot=1;
            else if (strcmp(cells[version_col].value,"v2")==0) slot=2; else if (strcmp(cells[version_col].value,"v3")==0) slot=3;
            else if (strcmp(cells[version_col].value,"v4")==0) slot=4; else if (strcmp(cells[version_col].value,"v5")==0) slot=5;
            else if (strcmp(cells[version_col].value,"v6")==0) slot=6; else if (strcmp(cells[version_col].value,"v7")==0) slot=7;
            else if (strcmp(cells[version_col].value,"v8")==0) slot=8; else if (strcmp(cells[version_col].value,"v9_soa")==0) slot=9;
        }
        if (slot >= 0) { snprintf(version_rows[slot],sizeof version_rows[slot],"%s",line); }
    }
    if (ferror(in)) ok = 0;
    if (fclose(in) != 0) ok = 0;
    out = fopen("results/bench/ladder_plot_input.csv", "wb"); if (!out) return 0;
    for (char *p=header_line;*p;++p) if (*p!='\r' && fputc(*p,out)==EOF) ok=0;
    for (size_t i=0;i<10;++i) if (version_rows[i][0]) for(char *p=version_rows[i];*p;++p) if(*p!='\r' && fputc(*p,out)==EOF)ok=0;
    if (fclose(out) != 0) ok = 0;
    return ok;
}

/* Verify signal trace formats, crossovers, and interpolated points. */
static int check_signals(int *buy_count, int *sell_count, double *xlo, double *xhi, double *ylo, double *yhi)
{
    FILE *f = fopen("results/plots/signals.dat", "rb"); double price[2000], shortv[2000], longv[2000]; int sig[2000], trade[2000]; size_t n = 0; int buys = 0, sells = 0;
    if (!f) return 0;
    char line[256];
    while (fgets(line, sizeof line, f) && n < 2000) {
        if (sscanf(line, "%d %lf %lf %lf %d", &trade[n], &price[n], &shortv[n], &longv[n], &sig[n]) != 5) { fclose(f); return 0; }
        if (n && trade[n] != trade[n - 1] + 1) { fclose(f); return 0; }
        if (sig[n] == 1) ++buys; else if (sig[n] == -1) ++sells; else if (sig[n] != 0) { fclose(f); return 0; }
        if (n) { double before = shortv[n-1] - longv[n-1], now = shortv[n] - longv[n];
            if (sig[n] == 1 && !(before <= 0.00005 && now > -0.00005)) { fclose(f); return 0; }
            if (sig[n] == -1 && !(before >= -0.00005 && now < 0.00005)) { fclose(f); return 0; }
            if (sig[n] == 0 && ((before < -0.00005 && now > 0.00005) || (before > 0.00005 && now < -0.00005))) { fclose(f); return 0; }
        }
        ++n;
    }
    fclose(f); if (!n) return 0;
    *buy_count = buys; *sell_count = sells; *xlo = trade[0]; *xhi = trade[n-1]; *ylo = INFINITY; *yhi = -INFINITY;
    for (size_t i=0;i<n;++i) { double vals[3]={price[i],shortv[i],longv[i]}; for(size_t k=0;k<3;++k){if(vals[k]<*ylo)*ylo=vals[k];if(vals[k]>*yhi)*yhi=vals[k];} }
    f = fopen("results/plots/signal_points.dat", "rb"); if (!f) return 0;
    int point_buy = 0, point_sell = 0; double x, y; int side;
    while (fscanf(f, "%lf %lf %d", &x, &y, &side) == 3) {
        int found = 0; if (side == 1) ++point_buy; else if (side == -1) ++point_sell; else { fclose(f); return 0; }
        for (size_t i = 1; i < n; ++i) if (sig[i] == side && x >= trade[i]-1.0001 && x <= trade[i]+0.0001) {
            double u = x - (trade[i]-1.0), a = shortv[i-1] + u * (shortv[i] - shortv[i-1]), b = longv[i-1] + u * (longv[i] - longv[i-1]);
            found = fabs(y-a) <= .0002 && fabs(y-b) <= .0002; break;
        }
        if (!found) { fclose(f); return 0; }
    }
    fclose(f); return buys > 0 && sells > 0 && point_buy == buys && point_sell == sells;
}

/* Validate PNG, diagnostics, summary ranges/counts, and per-plot invariants. */
static int check_plot(const PlotSpec *spec, int require, int data_only)
{
    FILE *probe = fopen(spec->input1, "rb");
    if (!probe) { if (require) { fprintf(stderr, "FAIL: required input missing: %s\n", spec->input1); return 0; } printf("SKIPPED: %s not generated\n", spec->input1); return 1; }
    fclose(probe);
    if (!validate_input(spec, spec->input1, 0)) return 0;
    if (spec->input2[0]) {
        FILE *second = fopen(spec->input2, "rb");
        if (!second) { if (require) { fprintf(stderr, "FAIL: required input missing: %s\n", spec->input2); return 0; }
            printf("SKIPPED: %s not generated\n", spec->input2); return 1; }
        fclose(second);
        if (!validate_input(spec, spec->input2, 1)) return 0;
    }
    if (data_only) { printf("PASS data %s\n", spec->name); return 1; }
    Blob png = read_blob(spec->png); if (!png.data) { if (require) { fprintf(stderr, "FAIL: required PNG missing: %s\n", spec->png); return 0; } puts("SKIPPED: PNG not generated"); return 1; }
    free(png.data);
    int ok = png_is_valid(spec->png, spec);
    if (!log_is_clean(spec->log)) { fprintf(stderr, "FAIL: plot log contains a failure: %s\n", spec->log); ok = 0; }
    double rows, count, xmin, xmax, ymin, ymax;
    if (!summary_value(spec->summary, "rows", &rows) || !summary_value(spec->summary, "xmin", &xmin) ||
        !summary_value(spec->summary, "xmax", &xmax) || !summary_value(spec->summary, "ymin", &ymin) || !summary_value(spec->summary, "ymax", &ymax)) ok = 0;
    if (ok && strcmp(spec->name, "signals") == 0) {
        int buys=0,sells=0; double data_xmin,data_xmax,data_ymin,data_ymax;
        if (!check_signals(&buys,&sells,&data_xmin,&data_xmax,&data_ymin,&data_ymax) || rows != 2000 ||
            !summary_equals(spec->summary,"points_trade_price",rows) ||
            !summary_value(spec->summary, "points_short_sma", &count) || count != rows ||
            !summary_value(spec->summary, "points_long_sma", &count) || count != rows ||
            !summary_equals(spec->summary,"points_buy",(double)buys) || !summary_equals(spec->summary,"points_sell",(double)sells) ||
            xmin > data_xmin || xmax < data_xmax || ymin > data_ymin || ymax < data_ymax) ok = 0;
    } else if (ok && strcmp(spec->name, "money-lost") == 0) {
        if (!validate_money_sweep(spec, rows, xmin, xmax, ymin, ymax)) ok = 0;
    } else if (ok && strcmp(spec->name, "ladder") == 0) {
        if (!validate_ladder_data(spec, rows, xmin, xmax, ymin, ymax)) ok = 0;
    }
    if (!ok) fprintf(stderr, "FAIL: plot validation failed for %s\n", spec->name);
    else printf("PASS plot %s\n", spec->name);
    return ok;
}

/* Exercise key checker failures using generated files under the build directory. */
static int run_self_tests(void)
{
    FILE *f; const char *path = "build/plot_checker_selftest.txt"; int ok = 1;
    quiet_expected_failures = 1;
    f = fopen(path, "wb"); if (!f) return 0; fputs("ERROR: deliberate checker self-test\n", f); fclose(f);
    CHECK(!log_is_clean(path));
    f = fopen(path, "wb"); if (!f) return 0; { const unsigned char utf16[] = {0xff,0xfe,'e',0,'r',0,'r',0,'o',0,'r',0}; fwrite(utf16,1,sizeof utf16,f); } fclose(f);
    CHECK(!log_is_clean(path));
    f = fopen(path, "wb"); if (!f) return 0; fputs("platform,member,reaction_us,pnl,money_lost,trades\r\nx,y,1,2,0,3\n", f); fclose(f);
    PlotSpec bad = plot_registry[1]; bad.minimum_rows = 1; CHECK(!validate_input(&bad,path,0));
    f = fopen("build/plot_bad.png", "wb"); if (!f) return 0; fputs("not a png", f); fclose(f);
    CHECK(!png_is_valid("build/plot_bad.png", &plot_registry[0]));
    f = fopen("build/plot_bad.png", "wb"); if (!f) return 0; fclose(f);
    CHECK(!png_is_valid("build/plot_bad.png", &plot_registry[0]));
    f = fopen("build/plot_bad.png", "wb"); if (!f) return 0; { unsigned char wrong[10100] = {137,80,78,71,13,10,26,10}; fwrite(wrong,1,sizeof wrong,f); } fclose(f);
    CHECK(!png_is_valid("build/plot_bad.png", &plot_registry[0]));
    f = fopen(path, "wb"); if (!f) return 0; fputs("header\n1,nan\n", f); fclose(f);
    CsvCell cells[4]; size_t nc = split_csv("wrong,column", cells, 4);
    CHECK(column_index(cells, nc, "reaction_us") < 0); CHECK(!finite_number("nan", NULL));
    f = fopen(path, "wb"); if (!f) return 0; fputs("platform,member,reaction_us,pnl,money_lost,trades\nx,y,1,nan,0,3\n",f); fclose(f);
    CHECK(!validate_input(&bad,path,0));
    f = fopen(path, "wb"); if (!f) return 0; fputs("platform,member,pnl,money_lost,trades\nx,y,1,0,3\n",f); fclose(f);
    CHECK(!validate_input(&bad,path,0));
    f = fopen(path, "wb"); if (!f) return 0; fputs("rows=2\npoints_x=0\n", f); fclose(f);
    CHECK(!summary_equals(path,"points_x",2));
    quiet_expected_failures = 0;
    printf("%s plot checker self-tests\n", ok ? "PASS" : "FAIL"); return ok;
}

/* Select registered plots by name, or validate every registered plot by default. */
int main(int argc, char **argv)
{
    const char *selected = NULL; int require = 0, selftest = 0, data_only = 0, ok = 1, matched = 0;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--plot") == 0 && i + 1 < argc) selected = argv[++i];
        else if (strcmp(argv[i], "--require") == 0) require = 1;
        else if (strcmp(argv[i], "--self-test") == 0) selftest = 1;
        else if (strcmp(argv[i], "--data-only") == 0) data_only = 1;
        else if (strcmp(argv[i], "--prepare") == 0 && i + 1 < argc) {
            const char *name = argv[++i]; if (strcmp(name, "ladder") != 0) return 1;
            (void)prepare_ladder();
            return 0;
        }
        else { fprintf(stderr, "Unknown plot checker option: %s\n", argv[i]); return 2; }
    }
    if (selftest) ok &= run_self_tests();
    for (size_t i = 0; i < PLOT_REGISTRY_COUNT; ++i) if (!selected || strcmp(selected, plot_registry[i].name) == 0) {
        matched = 1; if (!check_plot(&plot_registry[i], require, data_only)) ok = 0;
    }
    if (!matched && selected) { fprintf(stderr, "Unknown plot name: %s\n", selected); ok = 0; }
    return ok ? 0 : 1;
}
