# Known limitations and evidence gaps

- The authoritative coursework PDF is present at `provided/CSC1103-1104_Mini_Project_2026-27_Tick-to-Trade_final.pdf`. The requirements matrix and report map use it as the assignment source.
- Local laptop stress benchmark evidence exists under `results/laptop/`. It supports laptop-specific observations only; it is not evidence of Pi performance.
- The local `results/pi/` tree contains only `README.md`. Pi benchmark, perf, replay, LED, race, compiler assembly, and native AArch64 kernel results are therefore **PI EVIDENCE REQUIRED / NOT LOCALLY VERIFIED** in this audit.
- The handoff target describes Raspberry Pi 5, AArch64, GCC 14.2.0, performance governor, and taskset CPU 3 for controlled collection. Verify the actual device, compiler, governor, and applied affinity in each Pi run manifest before reporting them as measured conditions.
- Windows x1000 replay files and Windows portable-kernel fallback results do not establish Pi replay behavior or native AArch64 assembly execution.
- NumPy comparison is batch/offline processing; its timing is not timestamp-paced replay latency. No parser, storage layout, C/Python engine, or scalar assembly implementation is presumed to win.
- The root-level zero-byte `signals.png` is suspicious because `plots/signals.png` is the canonical plot. It was not removed. See the evidence inventory for a safe manual removal command that first checks the canonical file.
- Report, presentation/demo videos, signed declarations, separate peer evaluation, and completed signed prediction logbook entries remain team deliverables.
