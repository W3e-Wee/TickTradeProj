/* Output formatting and deterministic signal hash shared by engine programs. */
#include "result.h"

/* Hash one signal event with explicit byte order for cross-platform equality. */
uint64_t result_hash_signal(uint64_t hash, uint64_t index, int side)
{
    for (unsigned byte = 0; byte < 8; ++byte) {
        hash ^= (unsigned char)(index >> (byte * 8U));
        hash *= UINT64_C(1099511628211);
    }
    hash ^= (unsigned char)side;
    return hash * UINT64_C(1099511628211);
}

/* Print one RESULT record; callers write the human block and PASS separately. */
int result_print(FILE *stream, const RunResult *result)
{
    if (stream == NULL || result == NULL || result->version == NULL || result->file == NULL) return 0;
    return fprintf(stream,
        "RESULT version=%s file=%s trades=%llu short=%d long=%d vwap=%.6f bars=%llu buys=%llu sells=%llu signal_hash=%016llx load_ns=%llu process_ns=%llu total_ns=%llu peak_mem_kb=%llu\n",
        result->version, result->file, (unsigned long long)result->trades,
        result->short_window, result->long_window, result->vwap,
        (unsigned long long)result->bars, (unsigned long long)result->buys,
        (unsigned long long)result->sells, (unsigned long long)result->signal_hash,
        (unsigned long long)result->load_ns, (unsigned long long)result->process_ns,
        (unsigned long long)result->total_ns, (unsigned long long)result->peak_mem_kb) >= 0;
}

/* Avoid duplicate CSV headings while allowing each file to own its state. */
int result_csv_header_once(FILE *stream, int *written, const char *header)
{
    if (stream == NULL || written == NULL || header == NULL) return 0;
    if (*written) return 1;
    if (fprintf(stream, "%s\n", header) < 0) return 0;
    *written = 1;
    return 1;
}
