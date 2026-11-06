/* Ladder v6 replaces double decision arithmetic with scaled integers. */
#include "engine.h"
/* Run v6 with exact fixed-point VWAP and crossover comparisons. */
int main(int argc, char **argv)
{
    const EngineConfig config = {"v6", 1, 0, 1, 0};
    return engine_program_main(argc, argv, &config);
}
