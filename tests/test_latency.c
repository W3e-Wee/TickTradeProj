/* Check latency average, worst, and percentile calculations. */
#include "check.h"
#include "latency.h"
/* Summarize a small known set using preallocated storage. */
int main(void)
{
    uint64_t storage[4]; LatencySamples samples; LatencySummary summary;
    CHECK(latency_init(&samples, storage, 4));
    CHECK(latency_push(&samples, 40)); CHECK(latency_push(&samples, 10));
    CHECK(latency_push(&samples, 30)); CHECK(latency_push(&samples, 20));
    CHECK(!latency_push(&samples, 1));
    CHECK(latency_summarize(&samples, &summary));
    CHECK_NEAR(summary.average_ns, 25.0, 1e-12);
    CHECK(summary.worst_ns == 40 && summary.p50_ns == 20 && summary.p99_ns == 40);
    return CHECK_SUMMARY();
}
