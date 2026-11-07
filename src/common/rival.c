/* Position rules and reaction-time sweep used by rival_sweep. */
#include "rival.h"
#include <math.h>

/* Apply one buy/sell fill while respecting flat and holding positions. */
static void rival_fill(RivalStream *stream, double price, int side)
{
    if (side > 0 && !stream->holding) {
        stream->entry = price; stream->holding = 1; ++stream->trades;
    } else if (side < 0 && stream->holding) {
        stream->pnl += price - stream->entry; stream->holding = 0; ++stream->trades;
    }
}

/* Initialize the incremental R=1us P&L state used by live status output. */
void rival_stream_init(RivalStream *stream)
{
    if (stream != NULL) *stream = (RivalStream){0};
}

/* Apply delayed fills on the next trade and schedule slow reactions. */
void rival_stream_push(RivalStream *stream, double price, int signal,
                       uint64_t processing_ns, uint64_t reaction_ns)
{
    if (stream == NULL) return;
    if (stream->pending_side != 0) {
        rival_fill(stream, price, stream->pending_side);
        stream->pending_side = 0;
    }
    if (signal != 0) {
        if (processing_ns < reaction_ns) rival_fill(stream, price, signal);
        else stream->pending_side = signal;
    }
}

/* Copy the incremental trading totals into the common result structure. */
RivalResult rival_stream_result(const RivalStream *stream)
{
    if (stream == NULL) return (RivalResult){0};
    return (RivalResult){stream->pnl, stream->trades, stream->holding};
}

/* Apply flat/holding rules; equal reaction times use the next available trade. */
RivalResult rival_simulate(const double *prices, const int *signals,
                           const uint64_t *processing_ns, size_t count,
                           double reaction_us)
{
    RivalStream stream;
    if (prices == NULL || signals == NULL || processing_ns == NULL || reaction_us < 0.0)
        return (RivalResult){0};
    rival_stream_init(&stream);
    uint64_t reaction_ns = reaction_us >= (double)UINT64_MAX / 1000.0
        ? UINT64_MAX : (uint64_t)(reaction_us * 1000.0);
    for (size_t i = 0; i < count; ++i)
        rival_stream_push(&stream, prices[i], signals[i], processing_ns[i], reaction_ns);
    stream.pending_side = 0;
    return rival_stream_result(&stream);
}

/* Create an inclusive two-decade log sweep with a shared exact 1us endpoint. */
void rival_sweep_points(double *points, size_t count)
{
    if (points == NULL || count == 0) return;
    if (count == 25) {
        for (size_t i = 0; i <= 12; ++i) points[i] = pow(10.0, -1.0 + (double)i / 12.0);
        for (size_t i = 13; i < count; ++i) points[i] = pow(10.0, (double)(i - 12) * 2.0 / 12.0);
    } else {
        for (size_t i = 0; i < count; ++i) points[i] = count == 1 ? 0.1 : pow(10.0, -1.0 + 3.0 * (double)i / (double)(count - 1));
    }
}
