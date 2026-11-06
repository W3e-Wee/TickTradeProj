/* Shared ladder execution contract; version files select only explicit rungs. */
#ifndef TICKTRADE_ENGINE_H
#define TICKTRADE_ENGINE_H
#include "result.h"
#ifndef DEFAULT_SHORT_WINDOW
#define DEFAULT_SHORT_WINDOW 10
#endif
#ifndef DEFAULT_LONG_WINDOW
#define DEFAULT_LONG_WINDOW 60
#endif
typedef struct {
    const char *version;
    int fast_parser;
    int running_sum;
    int fixed_point;
    int storage_mode;
} EngineConfig;
/* Process one input file and return a complete result without printing. */
int engine_execute(const char *path, int short_window, int long_window,
                   const EngineConfig *config, RunResult *result);
/* Parse common options, run the configured engine, and print the shared contract. */
int engine_program_main(int argc, char **argv, const EngineConfig *config);
#endif
