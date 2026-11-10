/* Exercise Stage A plot exports and check their text and crossing geometry. */
#include "check.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef BUILD_DIR
#define BUILD_DIR "build/linux"
#endif
#ifndef STAGEA_BIN
#define STAGEA_BIN "build/linux/stageA_stream"
#endif

typedef struct { int trade, signal; double price, s, l; } Row;
static Row rows[2000];

/* Load a data file and enforce five fields, consecutive trades, and legal signals. */
static int load_rows(const char *path, size_t *count)
{
    FILE *f = fopen(path, "r"); char line[256]; *count = 0;
    if (!f) return 0;
    while (fgets(line, sizeof line, f)) {
        Row *r;
        if (*count >= 2000) { fclose(f); return 0; }
        r = &rows[*count];
        int used = 0;
        if (sscanf(line, "%d %lf %lf %lf %d %n", &r->trade, &r->price, &r->s, &r->l, &r->signal, &used) != 5) { fclose(f); return 0; }
        char expected[256];
        (void)snprintf(expected, sizeof expected, "%d %.2f %.4f %.4f %d\n", r->trade, r->price, r->s, r->l, r->signal);
        if (strcmp(line, expected) != 0) { fclose(f); return 0; }
        while (line[used] && isspace((unsigned char)line[used])) ++used;
        if (line[used] != '\0') { fclose(f); return 0; }
        if ((r->signal != -1 && r->signal != 0 && r->signal != 1) ||
            (*count && r->trade != rows[*count - 1].trade + 1)) { fclose(f); return 0; }
        ++*count;
    }
    fclose(f); return 1;
}

/* Verify crossover rows and that each exported point lies on both rounded SMA segments. */
static int check_export(const char *dat, const char *points, int require_full)
{
    size_t n = 0; FILE *f; double x, y; int side, buys = 0, sells = 0; int ok = load_rows(dat, &n);
    CHECK(ok); if (!ok) return 0;
    CHECK(n > 1); if (require_full) { CHECK(n == 2000); CHECK(rows[0].trade >= 61); }
    for (size_t i = 1; i < n; ++i) {
        double a = rows[i-1].s - rows[i-1].l, b = rows[i].s - rows[i].l;
        if (rows[i].signal == 1) { ++buys; CHECK(a <= 0.00005 && b > -0.00005); }
        else if (rows[i].signal == -1) { ++sells; CHECK(a >= -0.00005 && b < 0.00005); }
        else CHECK(!((a < -0.00005 && b > 0.00005) || (a > 0.00005 && b < -0.00005)));
    }
    CHECK(buys > 0 && sells > 0);
    f = fopen(points, "r"); CHECK(f != NULL); if (!f) return 0;
    int seen_buy = 0, seen_sell = 0;
    while (fscanf(f, "%lf %lf %d", &x, &y, &side) == 3) {
        int found = 0; if (side == 1) ++seen_buy; else if (side == -1) ++seen_sell; else CHECK(0);
        for (size_t i = 1; i < n; ++i) if (rows[i].signal == side && x >= rows[i].trade - 1.0001 && x <= rows[i].trade + 0.0001) {
            double u = x - (rows[i].trade - 1.0);
            double ys = rows[i-1].s + u * (rows[i].s - rows[i-1].s);
            double yl = rows[i-1].l + u * (rows[i].l - rows[i-1].l);
            CHECK(fabs(y - ys) <= 0.0002); CHECK(fabs(y - yl) <= 0.0002); found = 1; break;
        }
        CHECK(found);
    }
    fclose(f); CHECK(seen_buy == buys); CHECK(seen_sell == sells); return 1;
}

/* Generate fixture and available development exports with the requested windows. */
int main(void)
{
    char command[1024], dat[512], points[512];
    snprintf(dat, sizeof dat, "%s/signals.dat", BUILD_DIR);
    snprintf(points, sizeof points, "%s/signal_points.dat", BUILD_DIR);
    snprintf(command, sizeof command, "\"%s\" --file tests/fixtures/trades.csv --short 3 --long 5 --export-signals \"%s\" --slice-len 2000", STAGEA_BIN, dat);
    int status = system(command);
    CHECK(status == 0); check_export(dat, points, 0);
    FILE *probe = fopen("data/dev_1M.csv", "r");
    if (probe) {
        fclose(probe);
        snprintf(command, sizeof command, "\"%s\" --file data/dev_1M.csv --short 30 --long 60 --export-signals \"%s\" --slice-len 2000", STAGEA_BIN, dat);
        status = system(command);
        CHECK(status == 0); check_export(dat, points, 1);
    }
    return CHECK_SUMMARY();
}
