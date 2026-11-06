/* Ladder v5 adds compensated running-sum SMA updates. */
#include "engine.h"
/* Run v5 with the manual parser and compensated ring-buffer sums. */
int main(int argc, char **argv)
{
    const EngineConfig config = {"v5", 1, 1, 0, 0};
    return engine_program_main(argc, argv, &config);
}
