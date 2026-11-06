/* Common crossover decision interface for double and fixed-point engines. */
#ifndef TICKTRADE_SIGNAL_RULE_H
#define TICKTRADE_SIGNAL_RULE_H
typedef enum { SIGNAL_SELL = -1, SIGNAL_NONE = 0, SIGNAL_BUY = 1 } SignalSide;
typedef struct { double previous_diff; int ready; } SignalDoubleState;
/* Classify one double SMA difference and emit a thresholded crossover. */
int signal_double_step(SignalDoubleState *state, double difference, double epsilon);
#endif
