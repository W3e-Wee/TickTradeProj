# Tick-to-Trade on the Edge

Tick-to-Trade on the Edge is an educational CSC1103/CSC1104 group coursework repository. It contains a C market-data engine and registered implementation variants, a timestamp-paced replay harness, correctness and performance workflows, Python references, and an isolated AArch64 assembly exercise. It is designed for laptop development and Raspberry Pi 5 experiments using supplied or course-provided CSV data.

> **Academic and use notice:** This is educational coursework. It is not connected to a real exchange and does not perform real trading. It is not financial advice, carries no warranty, and offers no guarantee of profit. See [LICENSE_NOTICE.md](LICENSE_NOTICE.md).

## What the project does

The C workflows read Binance-style trade CSVs and calculate cumulative VWAP, short and long simple moving averages, crossover decisions, and one-second OHLC summaries. Versions v0-v10 compare registered parser, storage, arithmetic, and compiler choices. The project also checks results against the official test20 oracle, replays timestamped trades, models a coursework rival and concurrent race, profiles selected builds, and compares C with streaming Python and batch NumPy implementations. A separate unsigned dot-product kernel demonstrates portable C and AArch64 assembly; it is not a market-engine version.

## Architecture overview

`scripts/project.py` is the canonical build, test, validation, and evidence driver. It owns the registered version IDs, compiler arguments, build metadata, and most workflows. `Makefile` delegates common tasks to that driver. The C implementations are in `src/`, public contracts in `include/`, course-supplied support in `provided/`, and the Python engines in `python/`. `scripts/rival.py` implements the rival simulation. Tests live in `tests/`; generated programs and source-hash sidecars go in `build/`; evidence is written to `results/` in unique run directories.

## Supported platforms

- Windows 10/11 with **MSYS2 UCRT64** GCC and GNU Make for laptop build, test, and analysis workflows.
- Raspberry Pi 5 with 64-bit Raspberry Pi OS and AArch64 Linux for native Pi workflows, hardware LED access, `perf`, and the real AArch64 kernel.
- Python 3 and a GCC-compatible compiler are required for the main driver. Windows may use the portable C kernel fallback; that does not validate native assembly.

## Required tools

Required: Git, Python 3, GCC, and (for Makefile commands) GNU Make. Use a current GCC supported by the platform packages; the project records the compiler version and exact command with each build. Python's standard library is sufficient for the driver and plain-Python workflows. NumPy is optional and required only by `compare-python`/`market_numpy.py`. GNUplot is optional; the driver can write SVG plots without it. `perf`, `taskset`, and Linux CPU governor controls are for controlled Pi profiling and runs. See [Getting Started](docs/GETTING_STARTED.md) for installation and checks.

## Quick start

Clone the repository (replace the URL with the group's actual GitHub URL), then change into the cloned directory:

```sh
git clone <REPOSITORY_URL>
cd <REPOSITORY_FOLDER>
```

Install the platform tools using [docs/GETTING_STARTED.md](docs/GETTING_STARTED.md). Place the course datasets in the repository root, `data/`, or `datasets/`. Supported filename searches are `test20`, `dev_1M`, and `stress_full`; the official supplied test file in this checkout is named `test20 - Copy (6).csv`, so quote it when using the file path directly. The official oracle is `test20_expected - Copy (6).txt`. The driver also finds names containing `dev_1M` and `stress_full`; use the course-provided data rather than renaming or fabricating data.

```sh
python3 scripts/project.py doctor
python3 scripts/project.py build-all
python3 scripts/project.py test --suite quick
python3 scripts/project.py verify --dataset test20 --all-versions
python3 scripts/project.py replay --version v1_baseline_strtok_running_double_heap_O2 --dataset test20 --speed 1000 --limit 20
python3 scripts/project.py benchmark --dataset stress_full --repeats 5
```

The verify command requires the test20 CSV and oracle. The benchmark requires stress_full and can take a long time; laptop numbers are laptop evidence only. Build and test do not require NumPy. Check each command's output before relying on it; `doctor` reports missing tools and any discovered course dataset names.

When running the compiled baseline directly on Windows, quote the supplied filename because it contains spaces:

```powershell
.\build\release\market_baseline.exe "test20 - Copy (6).csv" 3 5
```

## Repository layout

| Path | Contents |
| --- | --- |
| `src/` | C market versions, replay engine/support, Stage 0 |
| `include/` | C interfaces and AArch64 kernel contract |
| `provided/` | Course helpers, LED/timing headers, and assignment PDF |
| `asm/` | Dot-product reference, benchmark, and AArch64 implementation |
| `python/` | Streaming Python, batch NumPy engine, and comparison helper |
| `scripts/` | Project driver and rival model |
| `tests/` | Python and C tests |
| `docs/` | Setup, workflows, development, evidence, and course handover |
| `config/` | Platform compiler flags and example environment file |
| `build/` | Generated binaries and build metadata; never commit these |
| `results/` | Existing and newly timestamped evidence; preserve raw files |

## Main command reference

Run commands from the repository root:

`doctor` shows detected tools and datasets:

```sh
python3 scripts/project.py doctor
```

Build the debug or release baseline:

```sh
python3 scripts/project.py build --profile debug
python3 scripts/project.py build --profile release
```

Build every registered offline and replay target:

```sh
python3 scripts/project.py build-all
```

Run the tests and verify all versions against test20:

```sh
python3 scripts/project.py test --suite quick
python3 scripts/project.py verify --dataset test20 --all-versions
```

Inspect replay or benchmark options:

```sh
python3 scripts/project.py replay --help
python3 scripts/project.py benchmark --help
```

Review source, evidence, and manual handover readiness:

```sh
python3 scripts/project.py readiness
```

The [User Guide](docs/USER_GUIDE.md) documents all major workflows and their inputs, outputs, platform requirements, and effects.

## Documentation index

- [Getting Started](docs/GETTING_STARTED.md): Windows/MSYS2 and Raspberry Pi setup, datasets, and troubleshooting.
- [User Guide](docs/USER_GUIDE.md): exact command interfaces and workflow outputs.
- [Developer Guide](docs/DEVELOPER_GUIDE.md): architecture, build/test layout, and safe contribution practices.
- [Reproducibility](docs/REPRODUCIBILITY.md): environment matrix and evidence reproduction protocol.
- [GitHub Handover](docs/GITHUB_HANDOVER.md): clone, branch, commit, review, and recovery steps.
- [Version manifest](docs/version_manifest.md): v0-v10 implementation identity and build selections.
- [Performance methodology](docs/performance_methodology.md): measurement boundaries and evidence interpretation.
- [AI assistance record](docs/ai_assistance.md): declared use and academic responsibility.
- [Known limitations](docs/known_limitations.md): platform evidence gaps and remaining team deliverables.

## Testing and validation

Run the portable checks after making changes:

```sh
python3 scripts/project.py doctor
python3 scripts/project.py readiness
python3 scripts/project.py test --suite quick
python3 scripts/project.py verify --dataset test20 --all-versions
```

`verify` needs the official test20 input and expected-answer file. Full test coverage may also use `dev_1M` and `stress_full`. Use the Pi-specific procedures for native AArch64, `perf`, LED, governor, and taskset checks. A Windows result cannot establish Pi behavior.

## Results and evidence policy

Do not overwrite or regenerate existing evidence. Workflows normally allocate unique run directories under `results/laptop/`, `results/pi/`, or `results/replays/`; inspect the manifest before sharing a result. Preserve raw output, source and dataset hashes, compiler/platform details, and measurement settings. Keep measured values distinct from predictions and laptop measurements distinct from Pi measurements. Large/private course CSVs are intentionally not ignored by Git: do not stage them unless the team and course rules explicitly require them. Inspect `git status` and add specific paths. See [Reproducibility](docs/REPRODUCIBILITY.md).

## Known limitations

- This checkout has laptop evidence and no locally inspectable Pi run evidence; Pi-only claims remain unverified here.
- NumPy processes batches offline and is not a replay-latency engine.
- The assembly exercise is an isolated dot product, not a replacement for the C market engine; scalar assembly may be slower than optimized C.
- LED output needs supported hardware and suitable Linux permissions. `perf` counters depend on kernel and PMU access.
- Rival P&L uses simplified coursework fills and omits exchange connectivity, fees, and real order execution.
- Course datasets may be large and may need to be obtained separately.

## Contribution workflow

Use a short feature branch, make a focused change, run applicable checks, inspect the full diff, and open a pull request for team review. Do not change frozen v0-v10, replay, rival, race, SoA, Python/NumPy, or assembly behavior as part of documentation work. Avoid `git add .`; stage explicit paths after checking status. Follow [GitHub Handover](docs/GITHUB_HANDOVER.md).

## AI assistance and academic integrity

AI assistance has been used in this repository and is recorded in [docs/ai_assistance.md](docs/ai_assistance.md). Team members must review and understand all submitted work, follow university/course disclosure rules, and ensure that submitted material represents permitted work. Do not present generated or unverified claims as personal measurements.

## License/coursework notice

This repository uses a restrictive coursework notice rather than granting an open-source license. Read [LICENSE_NOTICE.md](LICENSE_NOTICE.md) before reuse or redistribution.
