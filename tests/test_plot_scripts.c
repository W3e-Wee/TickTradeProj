/* Enforce fixed-path, headless, shared-style rules for registered gnuplot scripts. */
#include "check.h"
#include "plot_registry.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int quiet_expected_failure;

/* Match a complete token without confusing terminal names with prose. */
static int contains_word(const char *text, const char *word)
{
    size_t n = strlen(word);
    for (const char *p = text; (p = strstr(p, word)) != NULL; ++p) {
        int left = p == text || !isalnum((unsigned char)p[-1]);
        int right = !isalnum((unsigned char)p[n]);
        if (left && right) return 1;
    }
    return 0;
}

/* Read a script into a terminated buffer. */
static char *read_text(const char *path)
{
    FILE *f = fopen(path, "rb"); long n; char *text;
    if (!f || fseek(f, 0, SEEK_END) != 0 || (n = ftell(f)) < 0 || fseek(f, 0, SEEK_SET) != 0) { if (f) fclose(f); return NULL; }
    text = (char *)malloc((size_t)n + 1); if (!text) { fclose(f); return NULL; }
    size_t got = fread(text, 1, (size_t)n, f); fclose(f); text[got] = '\0'; return text;
}

/* Return whether a script contains a prohibited construct or path. */
static int script_is_valid(const char *path, int shared)
{
    char *text = read_text(path); int valid = text != NULL; size_t plot_at = (size_t)-1, output_at = (size_t)-1, line_no = 0; int reason = 0, saw_unset = 0; char last_line[512] = "";
    if (!text) return 0;
    if (!shared && strncmp(text, "load \"scripts/common.gp\"", strlen("load \"scripts/common.gp\"")) != 0) { valid = 0; reason = 1; }
    if (strstr(text, "system(") || strstr(text, "persist") || strstr(text, "pause")) { valid = 0; reason = 2; }
    if (!shared && strstr(text, "set terminal")) { valid = 0; reason = 3; }
    if (shared && (!strstr(text,"GPVAL_VERSION") || !strstr(text,"GPVAL_TERMINALS") || !strstr(text,"set decimalsign '.'") || !strstr(text,"pngcairo") || !strstr(text,"png size"))) valid = 0;
    for (char *line = text; *line;) {
        char *end = strchr(line, '\n'); if (end) *end = '\0'; ++line_no;
        char lower[512]; size_t n = strlen(line); if (n >= sizeof lower) n = sizeof lower - 1;
        for (size_t i = 0; i < n; ++i) lower[i] = (char)tolower((unsigned char)line[i]);
        lower[n] = '\0';
        snprintf(last_line,sizeof last_line,"%s",lower);
        int in_quote = 0; char quote = '\0';
        for (size_t i = 0; i < n; ++i) {
            if ((line[i] == '"' || line[i] == '\'') && (!in_quote || line[i] == quote)) { in_quote = !in_quote; quote = in_quote ? line[i] : '\0'; }
            if (in_quote && line[i] == '\\') valid = 0;
        }
        if (contains_word(lower,"wxt") || contains_word(lower,"qt") || contains_word(lower,"x11") || contains_word(lower,"windows") || contains_word(lower,"aqua") ||
            strstr(lower, "\"/") || strstr(lower, "'/") || strstr(lower, "\"~") || strstr(lower, "'~") ||
            (n > 2 && isalpha((unsigned char)lower[0]) && lower[1] == ':' && lower[2] == '/')) { valid = 0; reason = 4; }
        for (size_t i=0;i+2<n;++i) if (isalpha((unsigned char)lower[i]) && lower[i+1]==':' && lower[i+2]=='/') {valid=0;reason=4;}
        if (strstr(lower, "plot ") && plot_at == (size_t)-1) plot_at = line_no;
        if (strncmp(lower, "set output", 10) == 0) output_at = line_no;
        if (strncmp(lower, "unset output", 12) == 0) saw_unset = 1;
        if (!end) break;
        line = end + 1;
    }
    if (!shared && (plot_at == (size_t)-1 || output_at == (size_t)-1 || output_at > plot_at || !saw_unset || strcmp(last_line,"unset output") != 0)) { valid = 0; reason = 5; }
    if (!valid && !quiet_expected_failure) {
        fprintf(stderr, "Invalid plot script: %s (plot_line=%lu output_line=%lu reason=%d)\n", path,
                (unsigned long)plot_at, (unsigned long)output_at, reason);
    }
    free(text); return valid;
}

/* Verify the Makefile has no gnuplot command-line expressions. */
static int recipes_are_valid(void)
{
    char *text = read_text("Makefile"); int valid = text != NULL;
    if (text) { char *p = text; while ((p = strstr(p, "gnuplot")) != NULL) { char *end = strchr(p, '\n'); if (!end) end = p + strlen(p);
        char *flag = strstr(p, "-e");
        if (flag && flag < end) valid = 0;
        ++p; }
        free(text);
    }
    return valid;
}

/* Generate a forbidden temporary script and prove the scanner rejects it. */
static int scanner_self_test(void)
{
    FILE *f = fopen("build/plot_script_forbidden.gp", "wb"); if (!f) return 0;
    fputs("load \"scripts/common.gp\"\nset output \"results/plots/x.png\"\npause 1\nplot sin(x)\nunset output\n", f); fclose(f);
    quiet_expected_failure = 1; int detected = !script_is_valid("build/plot_script_forbidden.gp", 0); quiet_expected_failure = 0;
    return detected;
}

/* Scan the shared file, every registered plot script, and Make invocations. */
int main(void)
{
    CHECK(script_is_valid("scripts/common.gp", 1));
    for (size_t i = 0; i < PLOT_REGISTRY_COUNT; ++i) {
        CHECK(script_is_valid(plot_registry[i].script, 0));
        FILE *f = fopen(plot_registry[i].script, "rb"); CHECK(f != NULL); if (f) fclose(f);
    }
    CHECK(recipes_are_valid()); CHECK(scanner_self_test());
    return CHECK_SUMMARY();
}
