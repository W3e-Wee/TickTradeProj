# Stage status

| Stage | Status | Notes |
| --- | --- | --- |
| Stage 0 | IMPLEMENTED | Arithmetic program and guarded recursion check are in source. Paired laptop/Pi timing and Pi LED evidence are not locally visible. |
| Stage A baseline | IMPLEMENTED | Streaming VWAP, SMA, crossover, and OHLC logic is covered by local test20 verification. Group 05 windows are 3/5 for test20 and 30/60 for dev/stress. |
| Stage A+ parser work | IMPLEMENTED | v2 supplied sscanf, v3 pointer-output, and v4 manual digit readers are registered and covered by the local correctness workflow. Pi measurements are PI EVIDENCE REQUIRED / NOT LOCALLY VERIFIED. |
| Stage B optimization/storage | IMPLEMENTED | Naive/running SMA, double/fixed-point, O0/O2/O3, streaming/static-global/exact-malloc AoS, and separate exact-malloc SoA versions are registered. Pi controlled measurements are PI EVIDENCE REQUIRED / NOT LOCALLY VERIFIED. |
| Replay, latency, rival/race | IMPLEMENTED | Replay, latency reporting, rival simulation, and concurrent race workflows exist. Local Windows replay folders cover x1000 only; local laptop results do not establish Pi replay or LED evidence. |
| Pi final performance and assembly | PI EVIDENCE REQUIRED / NOT LOCALLY VERIFIED | The local `results/pi/` tree contains only its README. No Pi benchmark, perf, replay, race, or assembly artifact was available to inspect during this audit. |
| AArch64 kernel stretch | IMPLEMENTED | The unsigned dot-product contract, AArch64 leaf assembly, portable C fallback, correctness-gated harness, and listing annotation support exist. Native Pi execution is PI EVIDENCE REQUIRED / NOT LOCALLY VERIFIED. |
| Remaining submission work | MANUAL SUBMISSION ITEM | Complete Pi collection and inspection, report, videos, signed forms, peer evaluation, and prediction logbook; see the evidence inventory and submission checklist. |

This document is intentionally conservative: implemented status is not equated with full official-data validation.
