# Project progress

## Last verification session

- Date: 2026-11-08.
- Host: Windows 64-bit, MSYS2 UCRT64, GCC and GNU Make.
- `make clean`, `make`, `make test`, `make verify-golden`, and `make strict` completed successfully. The strict target rebuilt all C targets with `-Werror` and reran the full test suite.
- Full-file consistency passed for v0–v9 on the development and stress datasets; the course expected file and generated goldens passed.
- Stage 0 helper check, Stage A on test20 and development data, Stage 0 sum run, C NEON fallback, SoA stretch, parser comparison, bench/memory fixture smoke, replay integration, real-time replay, full-file 100× replay, 1000× replay, both race cores, rival sweep, and host assembly generation completed.
- Pure Python stretch passed on the fixture. NumPy was unavailable and the NumPy program printed `SKIPPED: numpy not installed`.
- Plot targets printed `SKIPPED: gnuplot not found`; `perf` and `bench-pi` printed their Windows platform skips.
- The five course helper files were moved to root `provided/`; Git blob hashes were confirmed unchanged. `.git` is read-only in this workspace, so the filesystem move is not staged.

## Milestones

| Milestone | Status | Verification / remaining |
| --- | --- | --- |
| Housekeeping | DONE | Root `provided/` has unchanged helper bytes; seven results placeholders; no `.github` or YAML automation files. |
| Foundation checks | DONE on Windows | C11 build, `-ffp-contract=off`, Windows timer/link path, test suite and strict build passed. Linux/macOS flag branches were inspected but not compiled here. |
| Stage 0, A, A+ | DONE on Windows | Helper, sums, streaming, parser fixture/development and golden checks passed. |
| Ladder, profiling, consistency, benchmark, memory, golden | DONE on Windows | v0–v9 consistency/golden passed; profiler and benchmark/memory smoke ran. Full group benchmark on final member data remains a team measurement. |
| Stage C laptop replay | DONE on Windows | Real-time window, full development file at 100×, full stress file at 1000×, both race cores, rival sweep and host assembly completed. Replay reports successful self-checks. |
| Stretch goals | PARTIAL | SoA v9, replay p50/p99 and Python fixture path work. NEON currently uses compiler intrinsics with scalar fallback rather than a hand-written `.S` kernel. AArch64 code generation, Raspberry Pi hardware, Linux and macOS remain unverified. NumPy was unavailable. |
| Core and audited documentation | PARTIAL | README, design, architecture, output format, runbook, stage guide, progress, commands, file inventory, prerequisites and results locations are present and updated. Source/header and function-comment checks passed; final relative-link and output-path audit remains. |
| Final acceptance | PARTIAL | Windows strict/full tests passed. Four-document coverage: all 42 phony targets appear in help and the command table; 92 files under `src/`, `tests/`, `scripts/`, `config/` are indexed; four stretch topics are represented. Detailed output-path status coverage remains partial. |

## Platform status

| Platform/tool | Status |
| --- | --- |
| Windows UCRT64 / GCC / GNU Make | VERIFIED this session |
| gnuplot | NOT VERIFIED; not installed, plot targets skipped |
| Python pure implementation | VERIFIED on fixture |
| NumPy | NOT VERIFIED; unavailable, NumPy row skipped |
| Linux / Linux `perf` | NOT VERIFIED; unavailable |
| macOS | NOT VERIFIED; unavailable |
| Raspberry Pi 5 / AArch64 / NEON | NOT VERIFIED; unavailable |

The generated measurement CSVs and replay outputs are program-produced under `results/`; do not copy measured values into this document. Members should fill the prediction logbook themselves.
