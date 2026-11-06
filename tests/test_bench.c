/* Exercise warm-up and repeated timing statistics without asserting durations. */
#include "check.h"
#include "bench.h"
typedef struct { unsigned calls; } BenchContext;
/* Increment a callback counter and return deterministic result metadata. */
static int fake_run(void *opaque, uint64_t *process_ns, uint64_t *trades)
{
    BenchContext *context = (BenchContext *)opaque;
    ++context->calls; *process_ns = 10; *trades = 4; return 1;
}
/* Confirm warm-up plus repetitions and well-formed timing statistics. */
int main(void)
{
    BenchContext context = {0}; BenchStats stats;
    uint64_t times[5], process[5];
    CHECK(bench_measure(fake_run, &context, 5, &stats, times, process));
    CHECK(context.calls == 6 && stats.trades == 4);
    CHECK(stats.best_ns <= stats.median_ns && stats.median_ns <= stats.worst_ns);
    CHECK(stats.median_process_ns == 10);
    return CHECK_SUMMARY();
}
