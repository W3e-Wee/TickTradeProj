# Laptop report draft

## Measured observations

- The Stage 0 program builds and runs on the current host.
- The market baseline on `test20 - Copy (6).csv` produced a final VWAP of `75436.93671742` and one BUY + one SELL signal.
- Local laptop stress benchmark artifacts are present under `results/laptop/`; use their raw rows and manifests for laptop-only claims. The local `results/pi/` tree has no Pi run evidence beyond README.md, so Pi performance, replay, LED, perf, and native AArch64 results remain PI EVIDENCE REQUIRED / NOT LOCALLY VERIFIED.

Group 05 window sizes are short/long = 3/5 for test20 and 30/60 for dev/stress. Confirm the exact dataset and window sizes in each result manifest before reporting its values.

## Expectations and trade-offs

- This project is intentionally simple and readable to make the assignment reviewable.
- The baseline stream parser is not a heavy framework; it prioritises clarity over micro-optimisation.
- Performance ranking is left open because the project does not claim a universal fastest engine across all hardware, compiler flags, and datasets.
- NumPy timings describe batch/offline Stage A processing, not timestamp-paced replay latency. Scalar hand-written assembly may also lose to GCC -O3; report the measured outcome either way.
