# Make commands

This reference was checked against `.PHONY`, `make help`, Makefile recipes and `make -n` on Windows. Recheck target coverage with `make help` whenever a target or recipe changes. Run all commands from the repository root; use `gmake` on macOS Homebrew unless GNU Make is on `PATH`, and use `mingw32-make` from Windows cmd/PowerShell.

## Variables

| Variable | Default | Meaning | Example |
| --- | --- | --- | --- |
| `CC` | `gcc` when Make's built-in default is unchanged | C compiler. | `make CC=clang all` |
| `OPT` | `-O2` | Default optimization for non-ladder C builds. | `make OPT=-O3 ladder` |
| `GROUP` | `01` | Selects the short/long pair from `config/groups.mk`. | `make GROUP=07 test-consistency` |
| `SHORT`, `LONG` | Group values | Explicit indicator window overrides. | `make SHORT=3 LONG=5 run-stageA` |
| `DATA` | `data` | Course data directory. | `make DATA=data run-stageA` |
| `FILE` | `$(DATA)/stress_full.csv` | Input for benchmark, replay, race and rival targets. | `make FILE=tests/fixtures/replay.csv replay-1000x` |
| `PYFILE` | `$(DATA)/stress_full.csv` | Input for the Python stretch. | `make PYFILE=data/test20.csv stretch-python` |
| `MEMBER` | `member` | Label included in result paths and rows. | `make MEMBER=grp05 bench` |
| `PI` | `0` | `1` enables Cortex-A76 tuning for v3 and AArch64 SIMD flags; requires Linux. | `make PI=1 bench-pi` |
| `PI_CFLAGS` | Empty | Additional v3 Pi compile flags. | `make PI=1 PI_CFLAGS=-mcpu=cortex-a76 ladder` |
| `NEON_CFLAGS` | Empty | Additional NEON-kernel compile flags. | `make PI=1 NEON_CFLAGS=-march=armv8-a+simd stretch` |
| `SUDO` | Empty | Optional command prefix for LED/replay targets. | `make SUDO=sudo DURATION=20 replay-realtime` |
| `REPEAT` | `5` | Stage 0 repeats and ladder benchmark repeats after warm-up. | `make REPEAT=7 bench` |
| `MAX_TRADES` | `10000000` | Capacity for the v7 static array. | `make MAX_TRADES=9000000 ladder` |
| `FASTEST` | `v8` | Label compiled into replay output and selects the ladder executable used as the Python C reference. | `make FASTEST=v8 stretch-python` |
| `CORE` | Empty | Replay selector: empty runs both, or choose `v0` or `fast`. | `make CORE=fast replay-1000x` |
| `SPEED` | `1000` | Race replay speed multiplier. | `make SPEED=100 race-v0` |
| `DURATION` | `600` | Market-time seconds selected by real-time replay. | `make DURATION=20 replay-realtime` |
| `PIN_CORE` | `1` | Linux CPU core for `bench-pi`. | `make PI=1 PIN_CORE=2 bench-pi` |
| `PYTHON` | Auto-detect `python3` then `python` | Interpreter used only by `stretch-python`. | `make PYTHON=python stretch-python` |
| `PYLIMIT` | Empty | Optional row limit for a quick Python comparison. | `make PYLIMIT=10000 stretch-python` |
| `PYREPEAT` | `3` | Repeat count for Python engine timing. | `make PYREPEAT=5 stretch-python` |
| `PROVIDED_DIR` | `provided` | Root location of untouched course helper files. | `make PROVIDED_DIR=provided all` |
| `V` | `0` | Set to `1` to print resolved compiler and platform settings. | `make V=1 all` |
| `PLATFORM`, `BUILD`, `EXE` | Auto-detected | Host label, `build/<platform>` directory, and executable suffix. | Override `PLATFORM` only for dry-run audits. |

The Makefile also selects `PYTHON_PATH`, `GNUPLOT_PATH`, `PI_CFLAGS` and `NEON_CFLAGS` from the platform and supplied overrides. Windows adds `-lpsapi -lwinmm`; Linux receives `_GNU_SOURCE`; macOS receives `_POSIX_C_SOURCE=200809L`.

## Stage 0

**Goal:** check the supplied timing/LED helpers and record optimization behavior. **Availability:** all supported C platforms. **Skip:** none; helper failures are errors.

```sh
make stage0-check
make run-stage0 REPEAT=5
```

`run-stage0` records loop rows under `results/bench/stage0_sum.csv` and recursive/iterative rows under `results/bench/stage0_recursion.csv`.

## Stage A

**Goal:** run the streaming engine on the course oracle and development file, then plot exported signals. **Availability:** requires the course CSVs; plotting additionally requires gnuplot. Plot skip: `SKIPPED: gnuplot not found`.

```sh
make stageA
make verify-golden
make run-stageA
make plot-signals
```

## Stage A+

**Goal:** compare the supplied parser and character-walking parser. The fixture check is short; the development check scans the full selected file. **Availability:** C compiler and input data. Missing development data falls back to the parser fixture.

```sh
make stageA_plus
make parser_compare
make parser-compare-test
make parser-dev-test
```

## Stage B

**Goal:** profile input precision, compare the ladder, and report per-engine memory. **Availability:** course data; `bench-pi` additionally needs Raspberry Pi hardware, Linux and `taskset`; `perf` needs Linux and perf; chart generation needs gnuplot.

```sh
make profile-data
make ladder
make test-consistency
make bench REPEAT=5
make memory
make plot-ladder
```

On Pi, run `make PI=1 PIN_CORE=1 bench-pi`. It checks Pi hardware, rebuilds v3 with the Cortex-A76 option, pins the nested benchmark with `taskset`, and otherwise prints one of `SKIPPED: Raspberry Pi benchmarking requires Linux.`, `SKIPPED: taskset not found`, or `SKIPPED: Raspberry Pi hardware not detected`. `make perf` uses `perf stat`; platform/tool skips are listed under [PREREQUISITES](PREREQUISITES.md#feature-to-package-matrix).

## Stage C

**Goal:** build and run the timestamp replay engines, collect latency distributions and emit host assembly. **Availability:** replay and host assembly work on supported laptops and Pi; the Pi annotated listing requires AArch64.

```sh
make stageC
make DURATION=20 replay-realtime
make FILE=data/dev_1M.csv replay-100x
make replay-1000x
make asm
```

`CORE=v0` or `CORE=fast` selects one replay engine; leave `CORE` empty to run both. `SUDO=sudo` is available for LED access. Real-time replay uses only the selected market-time window; accelerated replay consumes the selected file.

## Rival bot demonstration

**Goal:** generate the reaction-time P&L curve, then plot it. **Availability:** all supported platforms for the C sweep; plot output needs gnuplot. Plot skip: `SKIPPED: gnuplot not found`.

```sh
make rival FILE=data/stress_full.csv
make plot-rival
```

## Race demonstration

**Goal:** run baseline and fast replay processes independently at the same speed and input. **Availability:** all supported platforms; use the same `FILE`, `SPEED`, `GROUP`, and `MEMBER` values in both terminals.

Terminal one:

```sh
make FILE=data/stress_full.csv SPEED=1000 race-v0
```

Terminal two, started while the first is still running:

```sh
make FILE=data/stress_full.csv SPEED=1000 race-fast
```

## Stretch goals

### NEON kernel

**Goal:** check the NEON-intrinsic price/quantity sum against a portable C reference compiled at `-O3`. **Availability:** the scalar fallback works everywhere; AArch64 NEON execution requires an AArch64 build. There is no platform skip because the program has a portable fallback.

```sh
make stretch
```

On Pi, use `make PI=1 stretch` to add the AArch64 compiler flags.

### Struct-of-Arrays

**Goal:** run v9 and include its exact crossover sequence in consistency checks. **Availability:** all supported C platforms.

```sh
make stretch
make test-consistency
```

### Percentile latency

**Goal:** collect replay latency samples and report p50/p99 alongside average and worst latency. **Availability:** all supported replay platforms.

```sh
make FILE=tests/fixtures/replay.csv replay-1000x
make test-replay
```

### C versus Python

**Goal:** compare pure Python and NumPy Stage A with the C reference. **Availability:** Python is required, NumPy is optional; missing prerequisites print `SKIPPED: python not found`, `SKIPPED: numpy not installed`, or `SKIPPED: dataset not found: <path>`.

```sh
make PYFILE=data/test20.csv PYREPEAT=3 stretch-python
```

## Tests and verification

**Goal:** run unit/integration suites, course and generated goldens, parser comparisons and replay checks. Course-data consistency checks use the fixture when the data files are absent.

```sh
make test-unit
make test-test20
make test-consistency
make test-replay
make verify-golden
make strict
```

`make update-golden` regenerates deterministic references from v0 and is an explicit update operation; review its files before committing.

## General and housekeeping

**Goal:** build, inspect or clean the project. All C targets work without Python.

```sh
make help
make all
make tools
make plots
make clean
make distclean
```

`plots` invokes the signal, rival and ladder plot targets; each prints `SKIPPED: gnuplot not found` when the external program is missing. `clean` removes `build/`; `distclean` also removes generated result files while preserving the seven `.gitkeep` placeholders.

## Target coverage table

Each `.PHONY` target has one primary group below and appears in `make help`.

| Target | Primary group | Documented section |
| --- | --- | --- |
| `stage0` | Stage 0 | Stage 0 |
| `run-stage0` | Stage 0 | Stage 0 |
| `stage0-check` | Stage 0 | Stage 0 |
| `stageA` | Stage A | Stage A |
| `run-stageA` | Stage A | Stage A |
| `plot-signals` | Stage A | Stage A |
| `stageA_plus` | Stage A+ | Stage A+ |
| `parser_compare` | Stage A+ | Stage A+ |
| `parser-compare-test` | Stage A+ | Stage A+ |
| `parser-dev-test` | Stage A+ | Stage A+ |
| `ladder` | Stage B | Stage B |
| `profile-data` | Stage B | Stage B |
| `bench` | Stage B | Stage B |
| `bench-pi` | Stage B | Stage B |
| `perf` | Stage B | Stage B |
| `memory` | Stage B | Stage B |
| `plot-ladder` | Stage B | Stage B |
| `stageC` | Stage C | Stage C |
| `replay-realtime` | Stage C | Stage C |
| `replay-100x` | Stage C | Stage C |
| `replay-1000x` | Stage C | Stage C |
| `asm` | Stage C | Stage C |
| `rival` | Rival bot demonstration | Rival bot demonstration |
| `plot-rival` | Rival bot demonstration | Rival bot demonstration |
| `race-v0` | Race demonstration | Race demonstration |
| `race-fast` | Race demonstration | Race demonstration |
| `stretch` | Stretch goals | Stretch goals |
| `stretch-python` | Stretch goals | Stretch goals |
| `test` | Tests and verification | Tests and verification |
| `test-unit` | Tests and verification | Tests and verification |
| `test-test20` | Tests and verification | Tests and verification |
| `test-consistency` | Tests and verification | Tests and verification |
| `test-replay` | Tests and verification | Tests and verification |
| `verify-golden` | Tests and verification | Tests and verification |
| `update-golden` | Tests and verification | Tests and verification |
| `all` | General/housekeeping | General and housekeeping |
| `help` | General/housekeeping | General and housekeeping |
| `tools` | General/housekeeping | General and housekeeping |
| `plots` | General/housekeeping | General and housekeeping |
| `strict` | General/housekeeping | General and housekeeping |
| `clean` | General/housekeeping | General and housekeeping |
| `distclean` | General/housekeeping | General and housekeeping |
