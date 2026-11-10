# Laptop and Raspberry Pi runbook

This runbook lists commands from the repository root. GNU Make is required; on Windows use MSYS2 UCRT64 with `mingw32-make` from PowerShell or cmd. See [Prerequisites](PREREQUISITES.md) for installation and platform verification.

## First build

```sh
make clean
make
make test
```

The project builds its C programs without Python. `make stretch-python` is the only Python-dependent target. Course datasets belong in `data/` and are not part of the repository; fixture-backed parser and consistency checks remain available without them.

This project is configured for Group 05 with short window 30 and long window 60. `test20` always uses 3/5. Pass both `SHORT=` and `LONG=` to experiment with other windows; a non-05 `GROUP` also requires both overrides.

## Stage 0 and Stage A

```sh
make stage0-check
make run-stage0 REPEAT=5
make run-stageA
make plot-signals
```

Stage 0 runs loop and recursive sums at each optimization level. Stage A uses test20 with the required 3/5 window and the development file with the configured group window. The signal plot is optional when gnuplot is unavailable; the target prints `SKIPPED: gnuplot not found`.

## Parser, ladder, and profiling

```sh
make parser-compare-test
make parser-dev-test
make profile-data
make test-consistency
make bench REPEAT=5 MEMBER=member
make memory MEMBER=member
make plot-ladder
```

Set `DATA`, `FILE`, `SHORT`, and `LONG` as needed. `bench` appends engine rows to `results/bench/ladder_<platform>_<member>.csv`; `memory` appends peak-memory rows to `results/bench/memory_<platform>.csv`. The chart step skips if gnuplot is missing.

## Replay and race

```sh
make DURATION=20 replay-realtime
make FILE=data/dev_1M.csv replay-100x
make replay-1000x
make race-v0
make race-fast
make asm
```

By default replay targets run both the baseline and fast engines. Set `CORE=v0` or `CORE=fast` to run one. `FILE`, `SPEED`, `DURATION`, `SHORT`, `LONG`, and `MEMBER` control the run. Real-time replay plays the selected market-time window; accelerated replay consumes the input file. Results are written under `results/replay/`. Laptop output reports LED state in the terminal; supported Pi hardware can drive the provided LED helper. `make asm` emits a listing named for the compiler architecture. Only an AArch64 Pi build supplies the CSC1104 architecture listing.

For the race demonstration, use two terminals with identical parameters. Start `make FILE=data/stress_full.csv SPEED=1000 race-v0` in one, then `make FILE=data/stress_full.csv SPEED=1000 race-fast` in the other.

## Rival and optional platform tools

```sh
make rival FILE=data/stress_full.csv
make plot-rival
make perf FILE=data/stress_full.csv
make PI=1 PIN_CORE=1 bench-pi
```

`rival` generates its CSV on supported platforms. Plots require gnuplot. `perf` requires Linux and the `perf` tool. `bench-pi` requires Raspberry Pi hardware, Linux, and `taskset`; otherwise it reports the missing external prerequisite and exits successfully.

## Stretch goals

```sh
make stretch
make test-consistency
make PYFILE=data/test20.csv PYREPEAT=3 stretch-python
make FILE=tests/fixtures/replay.csv replay-1000x
```

`stretch` runs the portable scalar/NEON-intrinsic kernel check and v9 SoA. AArch64 builds can execute NEON instructions; other platforms use the scalar path. The replay output includes latency percentiles. `stretch-python` compares the C engine with pure Python and optional NumPy; missing Python, NumPy, or input data is reported with `SKIPPED`.

## Result collection and handoff

Use [Make commands](MAKE_COMMANDS.md) for target and variable details, [Results locations](RESULTS_LOCATIONS.md) for generated files and CSV headers, and [Stage guide](STAGE_GUIDE.md) for the stage index. Members record their own measurements and predictions in [the logbook](logbook.md); do not copy measurements into design notes by hand.

Before handoff, run `make clean && make && make test` on the development machine. To confirm the same results on another machine, run `make test-test20` and `make test-consistency` on each machine and compare the `trades`, `bars`, `buys`, `sells` and `signal_hash` fields of the printed `RESULT` lines; only timings may differ. Record other operating systems and Raspberry Pi as unverified unless those builds and commands were actually run there.
