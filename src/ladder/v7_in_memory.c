/* Ladder v7 loads all scaled trades into a bounded static Trade array. */
#include "engine.h"
/* Run v7 using the compiler-configured static storage capacity. */
int main(int argc, char **argv)
{
    const EngineConfig config = {"v7", 1, 0, 1, 1};
    return engine_program_main(argc, argv, &config);
}
