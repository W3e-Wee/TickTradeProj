/* Shared replay command; timing, statistics and output are OS-independent. */
#include "timing.h"
#include "cli.h"
#include "engine.h"
#include "exit_codes.h"
#include "fast_parser.h"
#include "fixed_point.h"
#include "latency.h"
#include "platform.h"
#include "reader.h"
#include "replay.h"
#include "replay_engine.h"
#include "replay_events.h"
#include "result.h"
#include "rival.h"
#include "signal_display.h"
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef DEFAULT_SHORT_WINDOW
#define DEFAULT_SHORT_WINDOW 30 /* Group 05 default; Make may override it. */
#endif
#ifndef DEFAULT_LONG_WINDOW
#define DEFAULT_LONG_WINDOW 60 /* Group 05 default; Make may override it. */
#endif
#ifndef REPLAY_FAST_LABEL
#define REPLAY_FAST_LABEL "v8"
#endif

#define REPLAY_EVENT_CAPACITY 1024U
#define REPLAY_RENDER_LIMIT 50U
#define REPLAY_IDLE_NS UINT64_C(2000000)
#define REPLAY_RENDER_NS UINT64_C(250000000)
#define REPLAY_STATUS_NS UINT64_C(100000000)
#define REPLAY_EXIT_INTERRUPTED 130

static volatile sig_atomic_t replay_stop_requested;

typedef struct
{
    const char *file;
    const char *out_dir;
    const char *member;
    const char *core_option;
    uint64_t speed;
    uint64_t duration_sec;
    uint64_t offset_sec;
    uint64_t stop_after_ms;
    int duration_set;
    int short_window;
    int long_window;
    int pin_core;
    int pin_set;
    int no_display;
    int test_stop_success;
    int test_pacing;
} ReplayOptions;

typedef struct
{
    uint64_t total_rows;
    uint64_t selected_rows;
    uint64_t first_selected_index;
    int64_t first_timestamp;
    int64_t last_timestamp;
    int64_t end_timestamp;
} ReplayCount;

typedef struct
{
    uint64_t *latency_ns;
    uint64_t *processing_ns;
    double *prices;
    int *signals;
    int64_t *short_ring;
    int64_t *long_ring;
} ReplayStorage;

/* Set only a signal-safe flag so cleanup remains in the replay thread. */
static void replay_handle_sigint(int signal_number)
{
    (void)signal_number;
    replay_stop_requested = 1;
}

/* Parse a replay row with the selected baseline or fast parser. */
static int replay_read_row(FILE *file, int fast, long long *timestamp,
                           double *price, double *qty, int *is_buy)
{
    return fast ? fast_read_trade(file, timestamp, price, qty, is_buy)
                : read_trade(file, timestamp, price, qty, is_buy);
}

/* Count eligible rows and determine the precise selected market-time range. */
static int replay_count_file(const char *path, uint64_t offset_sec,
                             uint64_t duration_sec, int duration_set,
                             ReplayCount *count)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL)
        return 0;
    memset(count, 0, sizeof *count);
    int64_t first_timestamp = 0;
    int have_first = 0;
    uint64_t offset_us;
    if (offset_sec > (uint64_t)INT64_MAX / 1000000U)
    {
        fclose(file);
        return 0;
    }
    offset_us = offset_sec * 1000000U;
    for (;;)
    {
        long long timestamp;
        double price, qty;
        int is_buy;
        int status = fast_read_trade(file, &timestamp, &price, &qty, &is_buy);
        if (status == 0)
            break;
        if (status < 0 || count->total_rows == UINT64_MAX)
        {
            fclose(file);
            return 0;
        }
        ++count->total_rows;
        if (!have_first)
        {
            first_timestamp = timestamp;
            have_first = 1;
        }
        if (timestamp < first_timestamp)
        {
            fclose(file);
            return 0;
        }
        if (first_timestamp > INT64_MAX - (int64_t)offset_us)
        {
            fclose(file);
            return 0;
        }
        int64_t start_timestamp = first_timestamp + (int64_t)offset_us;
        if (timestamp < start_timestamp)
            continue;
        if (count->selected_rows == 0)
        {
            count->first_timestamp = timestamp;
            count->first_selected_index = count->total_rows;
            if (duration_set)
            {
                if (duration_sec > (uint64_t)(INT64_MAX - timestamp) / 1000000U)
                {
                    fclose(file);
                    return 0;
                }
                count->end_timestamp = timestamp + (int64_t)(duration_sec * 1000000U);
            }
            else
                count->end_timestamp = INT64_MAX;
        }
        if (timestamp > count->end_timestamp)
            continue;
        ++count->selected_rows;
        count->last_timestamp = timestamp;
    }
    int read_failed = ferror(file);
    int close_failed = fclose(file) != 0;
    return !read_failed && !close_failed && count->selected_rows > 0;
}

/* Allocate every per-trade sample array before replay begins. */
static int replay_storage_init(ReplayStorage *storage, size_t count,
                               int short_window, int long_window)
{
    memset(storage, 0, sizeof *storage);
    if (count > SIZE_MAX / sizeof(uint64_t) || count > SIZE_MAX / sizeof(double) ||
        count > SIZE_MAX / sizeof(int))
        return 0;
    storage->latency_ns = (uint64_t *)malloc(count * sizeof *storage->latency_ns);
    storage->processing_ns = (uint64_t *)malloc(count * sizeof *storage->processing_ns);
    storage->prices = (double *)malloc(count * sizeof *storage->prices);
    storage->signals = (int *)malloc(count * sizeof *storage->signals);
    storage->short_ring = (int64_t *)calloc((size_t)short_window, sizeof *storage->short_ring);
    storage->long_ring = (int64_t *)calloc((size_t)long_window, sizeof *storage->long_ring);
    return storage->latency_ns != NULL && storage->processing_ns != NULL &&
           storage->prices != NULL && storage->signals != NULL &&
           storage->short_ring != NULL && storage->long_ring != NULL;
}

/* Release all replay-owned arrays, including partial allocation failures. */
static void replay_storage_free(ReplayStorage *storage)
{
    free(storage->latency_ns);
    free(storage->processing_ns);
    free(storage->prices);
    free(storage->signals);
    free(storage->short_ring);
    free(storage->long_ring);
    memset(storage, 0, sizeof *storage);
}

/* Sanitize member labels before using them as output path components. */
static int replay_valid_member(const char *member)
{
    if (member == NULL || *member == '\0')
        return 0;
    for (const unsigned char *p = (const unsigned char *)member; *p != '\0'; ++p)
        if (!((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') ||
              (*p >= '0' && *p <= '9') || *p == '_' || *p == '-'))
            return 0;
    return 1;
}

/* Join a caller-provided output directory with one filename safely. */
static int replay_join_path(char *buffer, size_t size, const char *directory,
                            const char *filename)
{
    size_t length = strlen(directory);
    const char *separator = length > 0 && (directory[length - 1] == '/' ||
                                           directory[length - 1] == '\\')
                                ? ""
                                : "/";
    int written = snprintf(buffer, size, "%s%s%s", directory, separator, filename);
    return written >= 0 && (size_t)written < size;
}

/* Print one concise error naming the file or output folder that failed. */
static void replay_io_error(const char *operation, const char *path)
{
    fprintf(stderr, "replay: %s '%s': %s\n", operation, path, strerror(errno));
}

/* Render no more than fifty queued events, then refresh status at most 10Hz. */
static void replay_render_visit(ReplayEventQueue *queue, int no_display,
                                uint64_t trades, double market_seconds,
                                uint64_t current_lag_ns, uint64_t buys,
                                uint64_t sells, double pnl,
                                uint64_t now, uint64_t *last_status)
{
    if (no_display)
        return;
    ReplayEvent events[REPLAY_RENDER_LIMIT];
    size_t count = replay_events_drain(queue, events, REPLAY_RENDER_LIMIT);
    for (size_t i = 0; i < count; ++i)
        signal_display_signal(events[i].side, events[i].trade_index, events[i].price);
    if (now - *last_status >= REPLAY_STATUS_NS)
    {
        signal_display_status(trades, market_seconds, current_lag_ns / 1000U,
                              buys, sells, pnl);
        *last_status = now;
    }
}

/* Parse shared replay options; all front ends accept the same command line. */
static int replay_parse_options(int argc, char **argv, const char *default_core,
                                ReplayOptions *options)
{
    *options = (ReplayOptions){0};
    options->speed = 1;
    options->short_window = DEFAULT_SHORT_WINDOW;
    options->long_window = DEFAULT_LONG_WINDOW;
    options->pin_core = -1;
    options->member = "member";
    options->out_dir = "results/replay";
    options->core_option = default_core;
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--help") == 0)
        {
            printf("Usage: %s --file CSV [--short N] [--long N] [--speed N] "
                   "[--duration-sec N] [--start-offset-sec N] [--pin CORE] "
                   "[--no-display] [--out-dir DIR] [--member NAME]\n",
                   argv[0]);
            return 2;
        }
        if (strcmp(argv[i], "--no-display") == 0)
        {
            options->no_display = 1;
            continue;
        }
        if (strcmp(argv[i], "--test-stop-success") == 0)
        {
            options->test_stop_success = 1;
            continue;
        }
        if (strcmp(argv[i], "--test-pacing") == 0)
        {
            options->test_pacing = 1;
            continue;
        }
        if (i + 1 >= argc)
            return 0;
        const char *key = argv[i++], *value = argv[i];
        if (strcmp(key, "--file") == 0)
            options->file = value;
        else if (strcmp(key, "--out-dir") == 0)
            options->out_dir = value;
        else if (strcmp(key, "--member") == 0)
            options->member = value;
        else if (strcmp(key, "--core") == 0)
            options->core_option = value;
        else if (strcmp(key, "--speed") == 0)
        {
            if (!cli_parse_u64(value, &options->speed) || options->speed == 0)
                return 0;
        }
        else if (strcmp(key, "--duration-sec") == 0)
        {
            if (!cli_parse_u64(value, &options->duration_sec) || options->duration_sec == 0)
                return 0;
            options->duration_set = 1;
        }
        else if (strcmp(key, "--start-offset-sec") == 0)
        {
            if (!cli_parse_u64(value, &options->offset_sec))
                return 0;
        }
        else if (strcmp(key, "--stop-after-ms") == 0)
        {
            if (!cli_parse_u64(value, &options->stop_after_ms) || options->stop_after_ms == 0)
                return 0;
        }
        else if (strcmp(key, "--short") == 0)
        {
            if (!cli_parse_positive_int(value, 100000, &options->short_window))
                return 0;
        }
        else if (strcmp(key, "--long") == 0)
        {
            if (!cli_parse_positive_int(value, 100000, &options->long_window))
                return 0;
        }
        else if (strcmp(key, "--pin") == 0)
        {
            uint64_t pin;
            if (!cli_parse_u64(value, &pin) || pin > INT_MAX)
                return 0;
            options->pin_core = (int)pin;
            options->pin_set = 1;
        }
        else
            return 0;
    }
    if (options->file == NULL || options->long_window < options->short_window ||
        !replay_valid_member(options->member) || *options->out_dir == '\0')
        return 0;
    if (!options->duration_set && options->speed == 1)
    {
        options->duration_sec = 600;
        options->duration_set = 1;
    }
    return 1;
}

/* Select the same streaming or fixed strategy used for the end hash check. */
static EngineConfig replay_check_config(int fast_core, const char *label)
{
    (void)fast_core;
    /* Both replay paths classify crossover direction with this exact ladder path. */
    return (EngineConfig){label, 1, 0, 1, 0};
}

/* Write the common replay CSV row to either the per-run or summary file. */
static int replay_write_csv_row(FILE *file, int header_needed,
                                const char *platform, const char *member,
                                const char *core, uint64_t speed, uint64_t trades,
                                double market_seconds, double wall_seconds,
                                const LatencySummary *latency,
                                double average_processing_ns, uint64_t max_lag_ns,
                                double pct_behind, uint64_t dropped,
                                uint64_t buys, uint64_t sells, double pnl)
{
    if (header_needed && fprintf(file,
                                 "platform,member,core,speed,trades_replayed,market_seconds,wall_seconds,"
                                 "avg_latency_ns,worst_latency_ns,p50_latency_ns,p99_latency_ns,"
                                 "avg_processing_ns,max_lag_us,pct_behind,dropped_events,buys,sells,pnl_vs_rival_r1\n") < 0)
        return 0;
    return fprintf(file, "%s,%s,%s,%" PRIu64 ",%" PRIu64 ",%.6f,%.6f,%.3f,"
                         "%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%.3f,%.3f,%.6f,%" PRIu64 ","
                         "%" PRIu64 ",%" PRIu64 ",%.8f\n",
                   platform, member, core, speed, trades,
                   market_seconds, wall_seconds, latency->average_ns, latency->worst_ns,
                   latency->p50_ns, latency->p99_ns, average_processing_ns,
                   (double)max_lag_ns / 1000.0, pct_behind, dropped, buys, sells, pnl) >= 0;
}

/* Append to the member summary and replace the current run's CSV. */
static int replay_write_outputs(const ReplayOptions *options, const char *platform,
                                const char *core, const char *run_path,
                                const char *summary_path, uint64_t speed,
                                uint64_t trades, double market_seconds,
                                double wall_seconds, const LatencySummary *latency,
                                double average_processing_ns, uint64_t max_lag_ns,
                                double pct_behind, uint64_t dropped,
                                uint64_t buys, uint64_t sells, double pnl)
{
    FILE *run_file = fopen(run_path, "wb");
    if (run_file == NULL)
    {
        replay_io_error("cannot write output in directory", options->out_dir);
        return 0;
    }
    int run_ok = replay_write_csv_row(run_file, 1, platform, options->member, core,
                                      speed, trades, market_seconds, wall_seconds, latency, average_processing_ns,
                                      max_lag_ns, pct_behind, dropped, buys, sells, pnl);
    if (fclose(run_file) != 0)
        run_ok = 0;
    if (!run_ok)
    {
        replay_io_error("cannot finish output in directory", options->out_dir);
        return 0;
    }
    FILE *summary = fopen(summary_path, "ab+");
    if (summary == NULL)
    {
        replay_io_error("cannot append summary in directory", options->out_dir);
        return 0;
    }
    if (fseek(summary, 0, SEEK_END) != 0)
    {
        fclose(summary);
        return 0;
    }
    int header_needed = ftell(summary) == 0;
    int summary_ok = replay_write_csv_row(summary, header_needed, platform, options->member,
                                          core, speed, trades, market_seconds, wall_seconds, latency, average_processing_ns,
                                          max_lag_ns, pct_behind, dropped, buys, sells, pnl);
    if (fclose(summary) != 0)
        summary_ok = 0;
    if (!summary_ok)
        replay_io_error("cannot finish summary in directory", options->out_dir);
    return summary_ok;
}

/* Run replay for either the helper baseline or selected fast ladder core. */
int replay_program_main(int argc, char **argv, const char *default_core)
{
    ReplayOptions options;
    int parsed = replay_parse_options(argc, argv, default_core, &options);
    if (parsed == 2)
        return EXIT_OK;
    if (parsed == 0)
        return EXIT_BAD_INPUT;
    if (options.pin_set && plat_pin_to_core(options.pin_core) != 0)
    {
        fprintf(stderr, "replay: cannot pin to core %d on this platform\n", options.pin_core);
        return EXIT_UNSUPPORTED;
    }
    int fast_core = strcmp(options.core_option, "v0") != 0;
    if (fast_core && strcmp(options.core_option, "fast") != 0 &&
        strcmp(options.core_option, REPLAY_FAST_LABEL) != 0)
        return EXIT_BAD_INPUT;
    const char *core_label = fast_core ? REPLAY_FAST_LABEL : "v0";
    uint64_t preflight_start = now_ns();
    ReplayCount count;
    if (!replay_count_file(options.file, options.offset_sec, options.duration_sec,
                           options.duration_set, &count))
    {
        fprintf(stderr, "replay: no valid trades in '%s' or input could not be read\n", options.file);
        return EXIT_IO_ERROR;
    }
    if (count.selected_rows > SIZE_MAX)
        return EXIT_OUT_OF_RANGE;
    ReplayStorage storage;
    if (!replay_storage_init(&storage, (size_t)count.selected_rows,
                             options.short_window, options.long_window))
    {
        replay_storage_free(&storage);
        fprintf(stderr, "replay: unable to allocate latency, processing, trade, signal, or SMA arrays\n");
        return EXIT_IO_ERROR;
    }
    ReplayEngine engine;
    if (!replay_engine_init(&engine, options.short_window, options.long_window,
                            fast_core, storage.short_ring, storage.long_ring))
    {
        replay_storage_free(&storage);
        return EXIT_OUT_OF_RANGE;
    }
    FILE *input = fopen(options.file, "rb");
    if (input == NULL)
    {
        replay_io_error("cannot open input", options.file);
        replay_storage_free(&storage);
        return EXIT_IO_ERROR;
    }
    long long timestamp;
    double price, qty;
    int is_buy;
    int status;
    uint64_t source_index = 0;
    int64_t offset_timestamp = count.first_timestamp;
    while ((status = replay_read_row(input, fast_core, &timestamp, &price, &qty, &is_buy)) > 0)
    {
        ++source_index;
        if (timestamp >= offset_timestamp)
            break;
    }
    if (status <= 0)
    {
        fclose(input);
        replay_storage_free(&storage);
        fprintf(stderr, "replay: selected start trade disappeared from '%s'\n", options.file);
        return EXIT_IO_ERROR;
    }
    int close_input_failed = 0;
    uint64_t preflight_ns = now_ns() - preflight_start;
    (void)signal(SIGINT, replay_handle_sigint);
    replay_stop_requested = 0;
    signal_display_init();
    plat_timer_begin();
    uint64_t granularity_us = plat_sleep_granularity_us();
    uint64_t run_start = now_ns();
    uint64_t stop_deadline = 0;
    if (options.stop_after_ms > 0)
    {
        if (options.stop_after_ms > (UINT64_MAX - run_start) / 1000000U)
        {
            fclose(input);
            signal_display_cleanup();
            plat_timer_end();
            replay_storage_free(&storage);
            return EXIT_OUT_OF_RANGE;
        }
        stop_deadline = run_start + options.stop_after_ms * 1000000U;
    }
    ReplayEvent event_storage[REPLAY_EVENT_CAPACITY];
    ReplayEventQueue events;
    (void)replay_events_init(&events, event_storage, REPLAY_EVENT_CAPACITY);
    RivalStream rival;
    rival_stream_init(&rival);
    uint64_t current_lag_ns = 0, max_lag_ns = 0, behind_count = 0;
    size_t seen = 0;
    int64_t last_processed_timestamp = count.first_timestamp;
    uint64_t last_render = run_start, last_status = run_start;
    int loop_error = 0, interrupted = 0;
    for (;;)
    {
        uint64_t release_ns;
        if (!replay_release_ns(run_start, count.first_timestamp, timestamp,
                               options.speed, &release_ns))
        {
            loop_error = 1;
            break;
        }
        if (!replay_wait_until(release_ns, granularity_us,
                               &replay_stop_requested, stop_deadline))
        {
            interrupted = 1;
            break;
        }
        uint64_t processing_start = now_ns();
        int signal_value = 0;
        if (!replay_engine_step(&engine, timestamp, price, qty, &signal_value))
        {
            loop_error = 1;
            break;
        }
        uint64_t decision_ns = now_ns();
        if (decision_ns < release_ns)
        {
            loop_error = 1;
            break;
        }
        uint64_t elapsed = decision_ns - processing_start;
        uint64_t latency = decision_ns - release_ns;
        if (seen >= (size_t)count.selected_rows)
        {
            loop_error = 1;
            break;
        }
        storage.latency_ns[seen] = latency;
        storage.processing_ns[seen] = elapsed;
        storage.prices[seen] = price;
        storage.signals[seen] = signal_value;
        last_processed_timestamp = timestamp;
        current_lag_ns = latency;
        if (latency > max_lag_ns)
            max_lag_ns = latency;
        if (latency > 0)
            ++behind_count;
        rival_stream_push(&rival, price, signal_value, elapsed, 1000U);
        if (signal_value != 0)
        {
            ReplayEvent event = {source_index, signal_value, price, decision_ns};
            (void)replay_events_push(&events, event);
        }
        ++seen;
        int64_t current_timestamp = timestamp;
        status = replay_read_row(input, fast_core, &timestamp, &price, &qty, &is_buy);
        if (status < 0)
        {
            loop_error = 1;
            break;
        }
        if (status == 0)
            break;
        ++source_index;
        if (timestamp < current_timestamp)
        {
            loop_error = 1;
            break;
        }
        if (timestamp < count.first_timestamp || timestamp > count.end_timestamp)
        {
            status = 0;
            break;
        }
        uint64_t next_release;
        if (!replay_release_ns(run_start, count.first_timestamp, timestamp,
                               options.speed, &next_release))
        {
            loop_error = 1;
            break;
        }
        uint64_t after_read = now_ns();
        uint64_t idle_gap = next_release > after_read ? next_release - after_read : 0;
        if (idle_gap > REPLAY_IDLE_NS || after_read - last_render >= REPLAY_RENDER_NS)
        {
            RivalResult pnl = rival_stream_result(&rival);
            double market_seconds = (double)(current_timestamp - count.first_timestamp) / 1000000.0;
            replay_render_visit(&events, options.no_display, engine.trades,
                                market_seconds, current_lag_ns, engine.buys, engine.sells,
                                pnl.pnl, after_read, &last_status);
            last_render = after_read;
        }
        if (replay_stop_requested || (stop_deadline != 0 && now_ns() >= stop_deadline))
        {
            interrupted = 1;
            break;
        }
    }
    if (ferror(input))
        loop_error = 1;
    if (fclose(input) != 0)
        close_input_failed = 1;
    if (close_input_failed)
        loop_error = 1;
    if (!options.no_display)
    {
        while (events.count > 0)
        {
            uint64_t now = now_ns();
            RivalResult pnl = rival_stream_result(&rival);
            double market_seconds = seen == 0 ? 0.0 : (double)(last_processed_timestamp - count.first_timestamp) / 1000000.0;
            replay_render_visit(&events, 0, engine.trades, market_seconds,
                                current_lag_ns, engine.buys, engine.sells, pnl.pnl, now, &last_status);
        }
        uint64_t now = now_ns();
        RivalResult pnl = rival_stream_result(&rival);
        replay_render_visit(&events, 0, engine.trades,
                            seen == 0 ? 0.0 : (double)(last_processed_timestamp - count.first_timestamp) / 1000000.0,
                            current_lag_ns, engine.buys, engine.sells, pnl.pnl, now, &last_status);
    }
    uint64_t run_end = now_ns();
    double wall_seconds = run_end >= run_start ? (double)(run_end - run_start) / 1e9 : 0.0;
    double market_seconds = seen == 0 ? 0.0 : (double)(last_processed_timestamp - count.first_timestamp) / 1000000.0;
    signal_display_cleanup();
    plat_timer_end();

    LatencySamples samples;
    LatencySummary summary = {0.0, 0, 0, 0};
    if (seen > 0)
    {
        if (!latency_init(&samples, storage.latency_ns, seen))
            loop_error = 1;
        else
        {
            for (size_t i = 0; i < seen; ++i)
            {
                uint64_t value = storage.latency_ns[i];
                if (!latency_push(&samples, value))
                    loop_error = 1;
            }
            if (!latency_summarize(&samples, &summary))
                loop_error = 1;
        }
    }
    long double process_total = 0.0L;
    uint64_t worst_processing = 0;
    for (size_t i = 0; i < seen; ++i)
    {
        process_total += storage.processing_ns[i];
        if (storage.processing_ns[i] > worst_processing)
            worst_processing = storage.processing_ns[i];
    }
    (void)worst_processing;
    double average_processing = seen == 0 ? 0.0 : (double)(process_total / seen);
    double pct_behind = seen == 0 ? 0.0 : (double)behind_count * 100.0 / (double)seen;
    RivalResult pnl_result = rival_simulate(storage.prices, storage.signals,
                                            storage.processing_ns, seen, 1.0);
    int whole_file = options.offset_sec == 0 &&
                     count.selected_rows == count.total_rows && seen == (size_t)count.selected_rows;
    if (options.test_pacing && !interrupted && pct_behind == 0.0 &&
        (wall_seconds < market_seconds || wall_seconds > market_seconds * 1.15 + 0.5))
        loop_error = 1;
    if (options.test_pacing && pct_behind > 0.0)
        puts("NOTE: machine too busy; pacing bound was not applied");
    int verification_ok = 1;
    int reference_ok = 1;
    RunResult reference = {0};
    if (whole_file && !interrupted && !loop_error)
    {
        EngineConfig config = replay_check_config(fast_core, core_label);
        reference_ok = engine_execute(options.file, options.short_window, options.long_window,
                                      &config, &reference) &&
                       reference.trades == engine.trades &&
                       reference.bars == engine.bars && reference.buys == engine.buys &&
                       reference.sells == engine.sells && reference.signal_hash == engine.signal_hash;
        verification_ok = reference_ok;
    }
    char run_filename[256], summary_filename[256], run_path[1024], summary_path[1024];
    const char *platform = plat_platform_name();
    int run_name_length = snprintf(run_filename, sizeof run_filename,
                                   "%s_%s_%s_%" PRIu64 "x.csv", platform, options.member, core_label, options.speed);
    int summary_name_length = snprintf(summary_filename, sizeof summary_filename,
                                       "summary_%s_%s.csv", platform, options.member);
    if (run_name_length < 0 || (size_t)run_name_length >= sizeof run_filename ||
        summary_name_length < 0 || (size_t)summary_name_length >= sizeof summary_filename ||
        !replay_join_path(run_path, sizeof run_path, options.out_dir, run_filename) ||
        !replay_join_path(summary_path, sizeof summary_path, options.out_dir, summary_filename))
    {
        replay_storage_free(&storage);
        return EXIT_OUT_OF_RANGE;
    }
    if (!replay_write_outputs(&options, platform, core_label, run_path, summary_path,
                              options.speed, engine.trades, market_seconds, wall_seconds, &summary,
                              average_processing, max_lag_ns, pct_behind, events.dropped,
                              engine.buys, engine.sells, pnl_result.pnl))
        loop_error = 1;
    RunResult result = {core_label, options.file, engine.trades,
                        options.short_window, options.long_window, replay_engine_vwap(&engine),
                        engine.bars, engine.buys, engine.sells, engine.signal_hash, preflight_ns,
                        (uint64_t)process_total, preflight_ns + run_end - run_start,
                        plat_peak_memory_kb()};
    printf("REPLAY platform=%s member=%s core=%s speed=%" PRIu64
           " wall_seconds=%.6f market_seconds=%.6f avg_latency_ns=%.3f "
           "worst_latency_ns=%" PRIu64 " max_lag_us=%.3f pct_behind=%.6f "
           "dropped_events=%" PRIu64 " pnl_vs_rival_r1=%.8f\n",
           platform, options.member, core_label, options.speed, wall_seconds,
           market_seconds, summary.average_ns, summary.worst_ns,
           (double)max_lag_ns / 1000.0, pct_behind, events.dropped, pnl_result.pnl);
    (void)result_print(stdout, &result);
    if (loop_error || !verification_ok)
    {
        fprintf(stderr, "replay: self-check failed (loop_error=%d reference_ok=%d trades=%" PRIu64 "/%" PRIu64 " bars=%" PRIu64 "/%" PRIu64 " buys=%" PRIu64 "/%" PRIu64 " sells=%" PRIu64 "/%" PRIu64 " hash=%016" PRIx64 "/%016" PRIx64 ")\n",
                loop_error, reference_ok, engine.trades, reference.trades,
                engine.bars, reference.bars, engine.buys, reference.buys,
                engine.sells, reference.sells, engine.signal_hash, reference.signal_hash);
        puts("FAIL replay self-check");
    }
    else
        puts("PASS");
    replay_storage_free(&storage);
    if (loop_error || !verification_ok)
        return loop_error ? EXIT_IO_ERROR : EXIT_MISMATCH;
    if (interrupted)
        return options.test_stop_success ? EXIT_OK : REPLAY_EXIT_INTERRUPTED;
    return EXIT_OK;
}
