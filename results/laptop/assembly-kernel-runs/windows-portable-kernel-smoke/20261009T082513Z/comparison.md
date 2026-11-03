# AArch64 dot-product kernel comparison

- Execution: portable C fallback; Pi assembly benchmark pending
- Repeats: 5; each timing contains 128 calls over 262144 uint64_t pairs.
- Inputs were populated at runtime. Correctness tests and a full-input equality gate ran before warm-up and timing.
- Timed sections include only repeated kernel calls and monotonic clock reads; allocation, initialization, printing, and correctness checks are outside.
- Median time ratio (assembly-labelled kernel / C reference): 1.096982.
- No performance winner is assumed. GCC -O3 may match or beat scalar assembly through scheduling, unrolling, or vectorization.

| Kernel | Median ns/call | Median elements/s | Checksum |
|---|---:|---:|---:|
| c_reference | 192537.50 | 1361521781.47 | 14406837647454548480 |
| portable_c_fallback | 211210.16 | 1241152436.30 | 14406837647454548480 |
