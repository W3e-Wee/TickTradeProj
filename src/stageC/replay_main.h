/* Shared entry point used by the baseline and fast replay front ends. */
#ifndef TICKTRADE_REPLAY_MAIN_H
#define TICKTRADE_REPLAY_MAIN_H
/* Parse options and run one configured replay core. */
int replay_program_main(int argc, char **argv, const char *default_core);
#endif
