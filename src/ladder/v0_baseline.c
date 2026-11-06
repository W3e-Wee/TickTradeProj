/* Frozen baseline engine front end: helper-style parsing, double, naive SMA. */
#include "engine.h"
#define ENGINE_STRINGIFY_INNER(value) #value
#define ENGINE_STRINGIFY(value) ENGINE_STRINGIFY_INNER(value)
#ifndef ENGINE_VERSION
#define ENGINE_VERSION v0
#endif
/* Run the baseline rung compiled at the selected optimization level. */
int main(int argc, char **argv)
{
    const EngineConfig config = {ENGINE_STRINGIFY(ENGINE_VERSION), 0, 0, 0, 0};
    return engine_program_main(argc, argv, &config);
}
