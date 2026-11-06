/* Ladder v8 counts rows, then allocates exactly the fixed-point trade array. */
#include "engine.h"
/* Run v8 using an exact-sized preloaded array. */
int main(int argc, char **argv)
{
    const EngineConfig config = {"v8", 1, 0, 1, 2};
    return engine_program_main(argc, argv, &config);
}
