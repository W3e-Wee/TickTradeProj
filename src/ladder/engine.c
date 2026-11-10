/* Ladder engine core shared by the frozen v0-v8 front ends. */
#include "engine.h"
#include "cli.h"
#include "exit_codes.h"
#include "fast_parser.h"
#include "fixed_point.h"
#include "indicators.h"
#include "memusage.h"
#include "platform.h"
#include "reader.h"
#include "signal_rule.h"
#include "trade.h"
#include "timing.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAX_TRADES
#define MAX_TRADES 10000000U
#endif
#define ENGINE_MAX_WINDOW 100000
#define ENGINE_MAX_REPEATS 1000U
#ifndef ENGINE_BUILD_FLAGS
#define ENGINE_BUILD_FLAGS "Makefile defaults"
#endif
#if defined(ENGINE_STATIC_STORAGE)
static Trade engine_static_trades[MAX_TRADES];
#endif

typedef struct {
    IndVwap vwap;
    IndOhlc bar;
    uint64_t bars, buys, sells, hash, count;
    double *price_ring;
    double *short_sma_storage, *long_sma_storage;
    IndSma short_sma, long_sma;
    int64_t *short_ring, *long_ring;
    int64_t short_sum, long_sum, qty_sum;
    FpAccumulator numerator;
    SignalDoubleState double_signal;
    int previous_fixed_sign, fixed_ready;
} EngineState;

/* Read a single record with the parser selected by this ladder rung. */
static int engine_read_row(FILE *file, int fast, long long *ts, double *price,
                           double *qty, int *is_buy)
{
    return fast ? fast_read_trade(file, ts, price, qty, is_buy)
                : read_trade(file, ts, price, qty, is_buy);
}

/* Convert parsed decimal doubles to exact scaled fields for fixed-point rungs. */
static int engine_scaled_trade(long long ts, double price, double qty, int is_buy, Trade *trade)
{
    double scaled_price = price * (double)PRICE_SCALE;
    double scaled_qty = qty * (double)QTY_SCALE;
    if (trade == NULL || !isfinite(scaled_price) || !isfinite(scaled_qty) ||
        scaled_price < 0.5 || scaled_qty < 0.5 ||
        scaled_price > (double)INT64_MAX || scaled_qty > (double)INT64_MAX) return 0;
    trade->ts_us = ts;
    trade->price = (long long)llround(scaled_price);
    trade->qty = (long long)llround(scaled_qty);
    trade->is_buy = is_buy;
    return 1;
}

/* Validate a parsed decimal before converting it to the shared signal scale. */
static int engine_scale_value(double value, int64_t scale, int64_t *scaled_out)
{
    double scaled;
    if (scaled_out == NULL || !isfinite(value) || value <= 0.0) return 0;
    scaled = value * (double)scale;
    if (!isfinite(scaled) || scaled < 0.5 || scaled >= (double)INT64_MAX) return 0;
    *scaled_out = (int64_t)llround(scaled);
    return 1;
}

/* Count a fast-parser pass for exact-sized v8 storage before allocation. */
static int engine_count_file(const char *path, uint64_t *count_out)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL) return 0;
    uint64_t count = 0; int status;
    for (;;) {
        long long ts; double price, qty; int is_buy;
        status = fast_read_trade(file, &ts, &price, &qty, &is_buy);
        if (status == 0) break;
        if (status < 0 || count == UINT64_MAX) { fclose(file); return 0; }
        ++count;
    }
    int failed = ferror(file); int closed = fclose(file) == 0;
    *count_out = count;
    return !failed && closed;
}

/* Load parsed rows into bounded static or exact-sized fixed-point storage. */
static int engine_load_memory(const char *path, const EngineConfig *config,
                              Trade **trades_out, uint64_t *count_out, uint64_t *load_ns)
{
    uint64_t expected = 0, start = now_ns();
    Trade *trades = NULL;
    if (config->storage_mode == 2) {
        if (!engine_count_file(path, &expected) || expected > SIZE_MAX / sizeof *trades) return 0;
        trades = expected == 0 ? NULL : (Trade *)malloc((size_t)expected * sizeof *trades);
        if (expected != 0 && trades == NULL) return 0;
    } else {
#if defined(ENGINE_STATIC_STORAGE)
        trades = engine_static_trades;
#else
        return 0;
#endif
    }
    FILE *file = fopen(path, "rb");
    if (file == NULL) { if (config->storage_mode == 2) free(trades); return 0; }
    uint64_t count = 0; int status = 1;
    while (status > 0) {
        long long ts; double price, qty; int is_buy;
        status = fast_read_trade(file, &ts, &price, &qty, &is_buy);
        if (status <= 0) break;
        if (count >= (config->storage_mode == 1 ? (uint64_t)MAX_TRADES : expected) ||
            !engine_scaled_trade(ts, price, qty, is_buy, &trades[count])) {
            status = -1; break;
        }
        ++count;
    }
    int failed = ferror(file); int closed = fclose(file) == 0;
    *load_ns = now_ns() - start;
    if (status < 0 || failed || !closed ||
        (config->storage_mode == 2 && count != expected)) {
        if (config->storage_mode == 2) free(trades);
        return 0;
    }
    *trades_out = trades; *count_out = count;
    return 1;
}

/* Update one indicator window using a checked integer rolling sum. */
static int engine_fixed_sma_push(int64_t *ring, int window, uint64_t index,
                                 int64_t price, int64_t *sum_out)
{
    size_t slot = (size_t)(index % (uint64_t)window);
    int64_t sum = *sum_out;
    if (index >= (uint64_t)window) {
        int64_t without_old;
        if (!fp_checked_add(sum, -ring[slot], &without_old)) return -1;
        sum = without_old;
    }
    ring[slot] = price;
    if (!fp_checked_add(sum, price, &sum)) return -1;
    *sum_out = sum;
    return index + 1U >= (uint64_t)window;
}

/* Update rolling bar count, VWAP, SMA, and crossover sequence for one trade. */
static int engine_step(const EngineConfig *config, EngineState *state,
                       int short_window, int long_window, long long ts,
                       double price, double qty, int is_buy)
{
    double short_average = 0.0, long_average = 0.0;
    int signal = 0;
    uint64_t index = state->count;
    IndOhlc completed;
    int64_t fixed_price;
    if (!engine_scale_value(price, PRICE_SCALE, &fixed_price)) return 0;
    int short_ready = engine_fixed_sma_push(state->short_ring, short_window, index,
                                             fixed_price, &state->short_sum);
    int long_ready = engine_fixed_sma_push(state->long_ring, long_window, index,
                                            fixed_price, &state->long_sum);
    if (short_ready < 0 || long_ready < 0) return 0;
    if (short_ready && long_ready) {
        int sign = fp_compare_products((uint64_t)state->short_sum, (uint64_t)long_window,
                                       (uint64_t)state->long_sum, (uint64_t)short_window);
        if (state->fixed_ready) {
            if (state->previous_fixed_sign <= 0 && sign > 0) signal = 1;
            else if (state->previous_fixed_sign >= 0 && sign < 0) signal = -1;
        }
        state->previous_fixed_sign = sign; state->fixed_ready = 1;
    }
    if (config->fixed_point) {
        int64_t p = fixed_price;
        int64_t q;
        if (!engine_scale_value(qty, QTY_SCALE, &q)) return 0;
        if (!fp_acc_add_product(&state->numerator, (uint64_t)p, (uint64_t)q) ||
            !fp_checked_add(state->qty_sum, q, &state->qty_sum)) return 0;
    } else {
        ind_vwap_push(&state->vwap, price, qty);
        if (config->running_sum) {
            (void)ind_sma_push(&state->short_sma, price, &short_average);
            (void)ind_sma_push(&state->long_sma, price, &long_average);
        } else {
            state->price_ring[index % (uint64_t)long_window] = price;
            if (short_ready) {
                for (int j = 0; j < short_window; ++j) {
                    size_t at = (size_t)((index + (uint64_t)long_window - (uint64_t)short_window + 1U + (uint64_t)j) % (uint64_t)long_window);
                    short_average += state->price_ring[at];
                }
                short_average /= (double)short_window; short_ready = 1;
            }
            if (long_ready) {
                for (int j = 0; j < long_window; ++j) {
                    size_t at = (size_t)((index + 1U + (uint64_t)j) % (uint64_t)long_window);
                    long_average += state->price_ring[at];
                }
                long_average /= (double)long_window; long_ready = 1;
            }
        }
        (void)short_average;
        (void)long_average;
    }
    if (signal > 0) { ++state->buys; state->hash = result_hash_signal(state->hash, index + 1U, 1); }
    if (signal < 0) { ++state->sells; state->hash = result_hash_signal(state->hash, index + 1U, -1); }
    if (ind_ohlc_push(&state->bar, ts, price, &completed)) ++state->bars;
    ++state->count;
    (void)is_buy;
    return 1;
}

/* Execute streaming or preloaded trades and build the machine-readable summary. */
int engine_execute(const char *path, int short_window, int long_window,
                   const EngineConfig *config, RunResult *result)
{
    EngineState state = {0};
    Trade *trades = NULL; uint64_t trade_count = 0, load_ns = 0;
    FILE *file = NULL; uint64_t total_start = now_ns();
    if (path == NULL || config == NULL || result == NULL || short_window <= 0 ||
        long_window < short_window || long_window > ENGINE_MAX_WINDOW) return 0;
    state.hash = UINT64_C(14695981039346656037);
    state.short_ring = (int64_t *)calloc((size_t)short_window, sizeof *state.short_ring);
    state.long_ring = (int64_t *)calloc((size_t)long_window, sizeof *state.long_ring);
    if (state.short_ring == NULL || state.long_ring == NULL) goto fail;
    if (config->fixed_point) {
        /* Exact rolling sums are also the shared crossover decision path. */
    } else if (config->running_sum) {
        state.short_sma_storage = (double *)calloc((size_t)short_window, sizeof *state.short_sma_storage);
        state.long_sma_storage = (double *)calloc((size_t)long_window, sizeof *state.long_sma_storage);
        if (state.short_sma_storage == NULL || state.long_sma_storage == NULL ||
            !ind_sma_init(&state.short_sma, state.short_sma_storage, (size_t)short_window) ||
            !ind_sma_init(&state.long_sma, state.long_sma_storage, (size_t)long_window)) goto fail;
    } else {
        state.price_ring = (double *)calloc((size_t)long_window, sizeof *state.price_ring);
        if (state.price_ring == NULL) goto fail;
    }
    if (config->storage_mode != 0) {
        if (!engine_load_memory(path, config, &trades, &trade_count, &load_ns)) goto fail;
    } else {
        file = fopen(path, "rb");
        if (file == NULL) goto fail;
    }
    uint64_t process_start = now_ns();
    if (config->storage_mode == 0) {
        for (;;) {
            long long ts; double price, qty; int is_buy;
            int status = engine_read_row(file, config->fast_parser, &ts, &price, &qty, &is_buy);
            if (status == 0) break;
            if (status < 0 || !engine_step(config, &state, short_window, long_window, ts, price, qty, is_buy)) goto fail;
        }
        int read_failed = ferror(file);
        int close_failed = fclose(file) != 0;
        if (read_failed || close_failed) { file = NULL; goto fail; }
        file = NULL;
    } else {
        for (uint64_t i = 0; i < trade_count; ++i) {
            Trade *t = &trades[i];
            double price = (double)t->price / (double)PRICE_SCALE;
            double qty = (double)t->qty / (double)QTY_SCALE;
            EngineConfig fixed = *config; fixed.fixed_point = 1;
            if (!engine_step(&fixed, &state, short_window, long_window, t->ts_us, price, qty, t->is_buy)) goto fail;
        }
    }
    uint64_t process_ns = now_ns() - process_start;
    IndOhlc final_bar;
    if (ind_ohlc_flush(&state.bar, &final_bar)) ++state.bars;
    *result = (RunResult){config->version, path, state.count, short_window, long_window,
        config->fixed_point ? (state.qty_sum == 0 ? 0.0 : fp_acc_to_double(&state.numerator) /
            (double)state.qty_sum / (double)PRICE_SCALE) : ind_vwap_value(&state.vwap),
        state.bars, state.buys, state.sells, state.hash, load_ns, process_ns,
        now_ns() - total_start, memusage_peak_kb()};
    free(state.price_ring); free(state.short_ring); free(state.long_ring);
    free(state.short_sma_storage); free(state.long_sma_storage);
    if (config->storage_mode == 2) free(trades);
    return state.count > 0;
fail:
    if (file != NULL) fclose(file);
    free(state.price_ring); free(state.short_ring); free(state.long_ring);
    free(state.short_sma_storage); free(state.long_sma_storage);
    if (config != NULL && config->storage_mode == 2) free(trades);
    return 0;
}

/* Sort small benchmark arrays so every reported median is reproducible. */
static void engine_sort_times(uint64_t *values, unsigned count)
{
    for (unsigned i = 1; i < count; ++i) {
        uint64_t value = values[i];
        unsigned j = i;
        while (j > 0 && values[j - 1U] > value) { values[j] = values[j - 1U]; --j; }
        values[j] = value;
    }
}

/* Quote one CSV field while preserving commas and embedded quote characters. */
static int engine_csv_field(FILE *file, const char *value)
{
    if (fputc('"', file) == EOF) return 0;
    for (const char *at = value; *at != '\0'; ++at) {
        if (*at == '"' && fputc('"', file) == EOF) return 0;
        if (fputc(*at, file) == EOF) return 0;
    }
    return fputc('"', file) != EOF;
}

/* Append one benchmark summary row and create its header for a new file. */
static int engine_write_bench_csv(const char *path, const char *member,
                                  const RunResult *result, unsigned repeats,
                                  uint64_t best_ns, uint64_t worst_ns)
{
    static const char header[] = "platform,member,version,file,trades,repeats,median_total_ns,best_total_ns,worst_total_ns,median_process_ns,trades_per_sec,peak_mem_kb,compiler,flags";
    FILE *file = fopen(path, "ab+");
    if (file == NULL) return 0;
    if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return 0; }
    long end = ftell(file);
    int written = end > 0;
    if (end < 0 || !result_csv_header_once(file, &written, header)) { fclose(file); return 0; }
    int ok = engine_csv_field(file, plat_platform_name()) && fputc(',', file) != EOF &&
        engine_csv_field(file, member) && fprintf(file, ",") >= 0 &&
        engine_csv_field(file, result->version) && fputc(',', file) != EOF &&
        engine_csv_field(file, result->file) &&
        fprintf(file, ",%llu,%u,%llu,%llu,%llu,%llu,%.6f,%llu,",
            (unsigned long long)result->trades, repeats,
            (unsigned long long)result->total_ns, (unsigned long long)best_ns,
            (unsigned long long)worst_ns, (unsigned long long)result->process_ns,
            result->total_ns == 0 ? 0.0 : (double)result->trades * 1e9 / result->total_ns,
            (unsigned long long)result->peak_mem_kb) >= 0 &&
        engine_csv_field(file, __VERSION__) && fputc(',', file) != EOF &&
        engine_csv_field(file, ENGINE_BUILD_FLAGS) && fputc('\n', file) != EOF;
    return fclose(file) == 0 && ok;
}

/* Parse engine options, optionally run warm-up/repeats, and emit shared output. */
int engine_program_main(int argc, char **argv, const EngineConfig *config)
{
    const char *path = NULL, *csv_path = NULL, *member = "member";
    int short_window = DEFAULT_SHORT_WINDOW, long_window = DEFAULT_LONG_WINDOW;
    unsigned repeats = 1;
    int benchmark = 0;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s --file CSV [--short N] [--long N] [--repeat N --csv PATH]\n", argv[0]);
            return EXIT_OK;
        }
        if (i + 1 >= argc) return EXIT_BAD_INPUT;
        const char *key = argv[i++], *value = argv[i];
        if (strcmp(key, "--file") == 0) path = value;
        else if (strcmp(key, "--short") == 0) { if (!cli_parse_positive_int(value, ENGINE_MAX_WINDOW, &short_window)) return EXIT_BAD_INPUT; }
        else if (strcmp(key, "--long") == 0) { if (!cli_parse_positive_int(value, ENGINE_MAX_WINDOW, &long_window)) return EXIT_BAD_INPUT; }
        else if (strcmp(key, "--repeat") == 0) {
            int parsed;
            if (!cli_parse_positive_int(value, (int)ENGINE_MAX_REPEATS, &parsed)) return EXIT_BAD_INPUT;
            repeats = (unsigned)parsed;
            benchmark = 1;
        } else if (strcmp(key, "--csv") == 0) { csv_path = value; benchmark = 1; }
        else if (strcmp(key, "--member") == 0) member = value;
        else return EXIT_BAD_INPUT;
    }
    if (path == NULL || short_window > long_window || (benchmark && csv_path == NULL)) return EXIT_BAD_INPUT;
    printf("Tick-to-Trade engine %s\nfile=%s short=%d long=%d\n", config->version, path, short_window, long_window);
    RunResult result;
    if (benchmark) {
        uint64_t *totals = (uint64_t *)malloc((size_t)repeats * sizeof *totals);
        uint64_t *processes = (uint64_t *)malloc((size_t)repeats * sizeof *processes);
        RunResult warmup;
        if (totals == NULL || processes == NULL) { free(totals); free(processes); return EXIT_IO_ERROR; }
        if (!engine_execute(path, short_window, long_window, config, &warmup)) { free(totals); free(processes); return EXIT_IO_ERROR; }
        uint64_t best = UINT64_MAX, worst = 0;
        for (unsigned i = 0; i < repeats; ++i) {
            if (!engine_execute(path, short_window, long_window, config, &result)) {
                free(totals); free(processes); return EXIT_IO_ERROR;
            }
            totals[i] = result.total_ns;
            processes[i] = result.process_ns;
            if (result.total_ns < best) best = result.total_ns;
            if (result.total_ns > worst) worst = result.total_ns;
        }
        engine_sort_times(totals, repeats);
        engine_sort_times(processes, repeats);
        result.total_ns = totals[repeats / 2U];
        result.process_ns = processes[repeats / 2U];
        free(totals); free(processes);
        if (!engine_write_bench_csv(csv_path, member, &result, repeats, best, worst)) return EXIT_IO_ERROR;
        printf("benchmark warmup=discarded repeats=%u best_ns=%llu median_ns=%llu worst_ns=%llu\n",
               repeats, (unsigned long long)best, (unsigned long long)result.total_ns,
               (unsigned long long)worst);
    } else if (!engine_execute(path, short_window, long_window, config, &result)) {
        return EXIT_IO_ERROR;
    }
    if (!result_print(stdout, &result)) return EXIT_IO_ERROR;
    puts("PASS");
    return EXIT_OK;
}
