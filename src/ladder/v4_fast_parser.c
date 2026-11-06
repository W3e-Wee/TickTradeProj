/* Ladder v4 adds the manual decimal parser to the double baseline. */
#include "engine.h"
/* Run v4 with fast parsing and otherwise baseline computation. */
int main(int argc, char **argv)
{
    const EngineConfig config = {"v4", 1, 0, 0, 0};
    return engine_program_main(argc, argv, &config);
}
