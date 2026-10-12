# Reproducibility

This guide describes how to reproduce documented repository workflows from a clean clone. Use the course's actual CSVs and retain the exact files received. Do not overwrite existing evidence. Commands are run from the repository root; use `python` in PowerShell and `python3` in MSYS2/Linux as installed.

## Environment matrix

| Environment | Supported work | Required/optional tools |
| --- | --- | --- |
| Windows 10/11, MSYS2 UCRT64 | Build, unit tests, offline verification/benchmark, replay, Python workflows, portable kernel | UCRT64 GCC and GNU Make, Python 3 (3.12 baseline), Git; NumPy optional for compare-python; gnuplot optional |
| Raspberry Pi 5, Raspberry Pi OS 64-bit, AArch64 | Native builds, replay, LED where supported, perf, real AArch64 assembly/kernel, Pi performance evidence | GCC (handoff target was 14.2.0; use and record installed version), GNU Make, Python 3, NumPy optional for comparison, gnuplot optional, `perf` and `taskset` for controlled profiling/affinity |
| Other Linux/macOS hosts | Portable subset where compiler/platform support permits | GCC-compatible C11 compiler, Python 3, optional Make/NumPy/gnuplot. Host output is not Pi evidence. |

The project records compiler version and flags in build metadata. Python's standard library is sufficient except for NumPy workflows. GNU Make is a wrapper convenience; direct driver commands do not require Make. Windows UCRT64 is the supported Windows target. No exact GCC minimum is enforced by the CLI; prefer the distro toolchain, keep `-std=c11` support, and report the actual version rather than claiming an unverified range.

## Dataset preparation

Put course CSVs in the repository root, `data/`, or `datasets/`. The driver searches names containing `test20`, `dev_1M`, and `stress_full`; it does not promise arbitrary filenames. In the current checkout the official test20 file is `test20 - Copy (6).csv`, with oracle `test20_expected - Copy (6).txt`; the current dev filename is `dev_1M - Copy (6).csv`, and stress is `stress_full.csv`. If the course supplies a differently named file, confirm the supported naming convention with the group before renaming. The tool's `--dataset` is a substring; direct paths containing spaces must be quoted, e.g. `"test20 - Copy (6).csv"` in PowerShell or `'test20 - Copy (6).csv'` in Bash.

Defaults: test20 short/long windows 3/5; Group 05 dev/stress windows 30/60. Confirm presence with:

```sh
python3 scripts/project.py doctor
```

The ignore file does not blanket-ignore CSVs or evidence. Large/private CSV files may appear in `git status`; leave them unstaged and use named `git add` paths. Do not add raw datasets unless the team/course has explicitly chosen to distribute them through Git.

## Build reproduction

From a fresh clone with GCC available:

```sh
python3 scripts/project.py clean-build
python3 scripts/project.py build-all
python3 scripts/project.py build-variants
python3 scripts/project.py build-replay
```

`clean-build` is optional and only removes generated build output. `build-all` already builds debug and release baselines, all normal variants, and replay targets. Individual alternatives are provided to isolate work; running all build commands sequentially repeats compilation. Expected targets and JSON freshness sidecars are in `build/debug/`, `build/release/`, `build/variants/`, and `build/replay/`. A sidecar records exact compiler command/version and hashes for all declared source inputs. After source changes rebuild the selected target; never use a binary with stale/missing metadata. `doctor` reports detected compiler and Make.

## Test reproduction

```sh
python3 scripts/project.py test --suite quick
python3 scripts/project.py test --suite full
python3 scripts/project.py replay-test
python3 scripts/project.py verify --dataset test20 --all-versions
```

The CLI accepts `--suite quick` and `--suite full`, but the current dispatcher runs the same complete build, C support test, and Python `unittest discover` suite for either value. Official parity needs test20 CSV and oracle; dev/stress tests need those datasets. When NumPy is absent, NumPy-specific tests skip (four skips in the validated environment); a skip means unavailable, not passed. Pi-only LED/native-assembly/perf evidence cannot be produced by Windows. Investigate assertion/compiler errors; do not rerun by deleting or replacing evidence. Tests may create uniquely named workflow-test files beneath `results/laptop/` as well as build outputs; preserve existing evidence and review any new artifacts separately.

## Benchmark reproduction

Run correctness verification first, then benchmark at least five times per version and report medians. Use the project's correctness-gated workflow:

```sh
python3 scripts/project.py verify --dataset test20 --all-versions
python3 scripts/project.py benchmark --dataset stress_full --repeats 5 --platform-label laptop-repro
```

On Pi, for controlled final measurement, inspect every CPU governor, warm the workload, and pin to a valid CPU (CPU 3 is the handoff example):

```sh
python3 scripts/project.py benchmark --dataset stress_full --repeats 5 --platform-label pi-controlled --taskset-cpus 3 --final-controlled
```

The driver records whether pinning was applied; `--final-controlled` requires the Pi, a performance governor on all CPUs, taskset, and at least five repeats. It does not change system governor settings. If unavailable, run exploratory and label it honestly. Do not print progress or write files inside timed loops. The workflow warms targets as implemented and records raw repeat rows plus medians. Keep each new run's automatically unique directory, raw output, manifest, platform/dataset hashes, and conditions. Never rename old raw evidence as a new run. Use the median, report spread where available, and compare only matching datasets, windows, compiler/flags, and host conditions.

## Replay, rival, and race reproduction

Build replay targets first. For a short accelerated demonstration:

```sh
python3 scripts/project.py replay --version v1_baseline_strtok_running_double_heap_O2 --dataset test20 --speed 1000 --limit 20
```

For real-speed replay, use the same command with `--speed 1` and no small limit when the intended run permits it. Speed values are 1, 100, or 1000. Output is written to a unique replay directory with latency/run metadata; optional decision CSV/log flags produce decision evidence. Group 05 windows are selected from the dataset name unless overridden.

Rival model and reaction sweep:

```sh
python3 scripts/project.py rival --dataset test20 --baseline-version v0_baseline_strtok_running_double_heap_O0 --fast-version v1_baseline_strtok_running_double_heap_O2 --speed 1000 --rival-reaction-us 1 --sweep
```

It replays both versions and calculates simplified fills/P&L from measured processing duration. Provide fast version explicitly when no eligible Pi benchmark result exists. Race:

```sh
python3 scripts/project.py race --dataset test20 --baseline-version v0_baseline_strtok_running_double_heap_O0 --fast-version v1_baseline_strtok_running_double_heap_O2 --speed 1000 --log-only
```

Race runs concurrently and both processes contend for resources; its times are not isolated benchmarks. These software workflows can run on laptops, but Pi latency/race/LED claims require runs on the Pi. Physical LED mode is Pi-only and may need sudo/device permissions; enable `--hardware-led` only when the hardware and permissions are ready. Each command creates new result evidence; retain manifests and raw files.

## Profile reproduction

Use Linux `perf` on the Pi for hardware counters. Inspect governor and pinning, then run:

```sh
python3 scripts/project.py profile --version v1_baseline_strtok_running_double_heap_O2 --dataset stress_full --repeats 1 --taskset-cpus 3 --platform-label pi-profile
```

Perf permission/PMU availability depends on OS settings. Raw perf files and a manifest are preserved under a fresh profile run folder. Unsupported, missing, or malformed counters remain blank/null with notes; they are never converted to zero. To regenerate a normalized summary from an existing run without rerunning perf, pass that existing directory:

```sh
python3 scripts/project.py refresh-perf-summary --profile-dir results/pi/profile-runs/pi-profile/<RUN-ID>
```

The normalized summary is derived output in the existing profile directory, while raw files should remain unchanged. Record the actual governor, affinity application, and missing counters. Do not run Pi profiling claims from Windows.

## Assembly reproduction

Compiler-generated assembly uses registered sources/flags and creates a unique folder:

```sh
python3 scripts/project.py assembly --version replay-engine --platform-label pi-assembly
```

Run this on AArch64 Pi to collect AArch64 assembly. Annotate an actual generated path, keeping the `.s` and hash manifest intact:

```sh
python3 scripts/project.py annotate-assembly --assembly results/pi/assembly-runs/pi-assembly/<RUN-ID>/assembly/replay_engine.s --version replay-engine
```

Portable/native kernel test and benchmark:

```sh
python3 scripts/project.py asm-kernel --mode test
python3 scripts/project.py asm-kernel --mode benchmark --repeats 5 --platform-label pi-kernel --taskset-cpus 3
```

The benchmark tests correctness first, warms both kernels, alternates order, and compares the hand-written kernel against C `-O3` using matching runtime-populated inputs. It writes raw repeats, medians, checksums, and a manifest under a new assembly-kernel run. Windows uses a portable C fallback; only native AArch64 execution on Pi is assembly evidence. Scalar assembly can lose to C `-O3`; report what was measured without assuming a win.

## Python and NumPy reproduction

The plain Python engine is streaming and can be run directly:

```sh
python3 python/market_python.py --help
```

The NumPy engine is a separate batch/offline implementation. Install the optional dependency and inspect its actual CLI:

```sh
python3 -m pip install -r requirements-python.txt
python3 python/market_numpy.py --help
```

Run C/Python/NumPy parity gates and comparison:

```sh
python3 scripts/project.py compare-python --dataset test20 --repeats 1 --platform-label laptop-smoke-test20
python3 scripts/project.py compare-python --dataset stress_full --repeats 5 --platform-label pi-python-final --taskset-cpus 3
```

The comparison requires NumPy and test20 oracle; it writes raw repeats, medians, Markdown, SVG, and manifest to a unique smoke (1-4 repeats) or comparison (5+) directory. It measures offline processing/process wall, not replay latency. `compare-python` can run on Windows or Pi; platform-labelled outputs must be described as their actual platform. Do not infer that NumPy or any engine is faster from one run.

## Evidence policy

- Preserve all existing evidence. Each run gets a new timestamped location; do not overwrite files or fabricate missing hardware data.
- Preserve raw output and manifests alongside summaries. Record source/dataset hashes, compiler, flags, OS/architecture, governor, taskset request/application, and repeat count.
- Clearly distinguish measured values from predictions, Pi measurements from laptop measurements, and offline batch time from replay latency.
- A laptop assembly listing, portable kernel, or benchmark is not evidence for native Pi behavior.
- Result data may be large; consult course/team rules before distributing it and never add large/private datasets by accident.
