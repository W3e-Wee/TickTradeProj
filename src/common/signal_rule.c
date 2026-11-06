/* EPS-aware crossover decisions shared by all double precision versions. */
#include "signal_rule.h"
#include <math.h>
#include <stddef.h>

/* Require a prior classified difference before emitting a crossover signal. */
int signal_double_step(SignalDoubleState *state, double difference, double epsilon)
{
    double classified;
    int signal = SIGNAL_NONE;
    if (state == NULL || epsilon < 0.0 || !isfinite(difference)) return SIGNAL_NONE;
    classified = fabs(difference) < epsilon ? 0.0 : difference;
    if (state->ready) {
        if (state->previous_diff <= 0.0 && classified > 0.0) signal = SIGNAL_BUY;
        else if (state->previous_diff >= 0.0 && classified < 0.0) signal = SIGNAL_SELL;
    }
    state->previous_diff = classified;
    state->ready = 1;
    return signal;
}
