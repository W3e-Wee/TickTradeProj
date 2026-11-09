# Tick-to-Trade on the Edge

## What this project does

This C11 coursework project reads offline Binance-style trade CSV files, computes VWAP, one-second OHLC bars and moving-average crossover signals, and compares streaming, in-memory, replay and optimized implementations. It does not connect to an exchange or place orders.

## Supported platforms and prerequisites

The project uses GCC or Clang and GNU Make on 64-bit Linux, macOS, Windows and Raspberry Pi OS. MSVC is not supported. On Windows, use the MSYS2 UCRT64 terminal or invoke `mingw32-make` from cmd or PowerShell.

| Platform | Install the compiler and GNU Make | Optional plotting and stretch tools |
| --- | --- | --- |
| Debian or Ubuntu | `sudo apt install build-essential` | `sudo apt install gnuplot python3 python3-numpy` |
| Windows MSYS2 UCRT64 | `pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make` | `pacman -S mingw-w64-ucrt-x86_64-gnuplot mingw-w64-ucrt-x86_64-python mingw-w64-ucrt-x86_64-python-numpy` |
| macOS | `xcode-select --install`, then `brew install make` | `brew install gnuplot python numpy` |
| Raspberry Pi OS | `sudo apt install build-essential` | `sudo apt install gnuplot python3 python3-numpy linux-perf util-linux` |

For Homebrew Make, use `gmake` or add Homebrew's GNU Make directory to `PATH`. Install NumPy only for `stretch-python`; other C targets do not need Python. Package names and commands are also listed in [docs/PREREQUISITES.md](docs/PREREQUISITES.md). See the [MSYS2 package listings](https://packages.msys2.org/groups/mingw-w64-ucrt-x86_64-toolchain), [Homebrew Make](https://formulae.brew.sh/formula/make) and [Homebrew gnuplot](https://formulae.brew.sh/formula/gnuplot) for package details.

## Course data

Place `test20.csv`, `dev_1M.csv`, `stress_full.csv`, and `test20_expected.txt` under `data/`. Keep supplied input files unchanged. The large datasets are distributed separately and are ignored by Git; [data/README.md](data/README.md) lists the required names. Tests use the fixture when a course dataset is absent.

## Quick start

From the repository root, run:

```sh
make
make test
make run-stageA
```

On macOS, substitute `gmake` if GNU Make is installed under that name. In Windows PowerShell, use `mingw32-make` for each command. The VS Code tasks in *Terminal > Run Task…* cover the common build, test, benchmark and plot actions.

## Choose your group

`GROUP` selects the short and long moving-average windows in `config/groups.mk`. For example, `make GROUP=07 test-consistency` uses the configured windows for that group. `test20.csv` always uses windows 3 and 5.

## Stage guide and Make targets

| Stage | Main commands | Purpose |
| --- | --- | --- |
| Stage 0 | `make stage0-check`, `make run-stage0` | Check the supplied helpers and compare loop/recursive sums across optimization settings. |
| Stage A | `make run-stageA`, `make plot-signals` | Stream trades, compute indicators, compare with the course oracle and plot a signal slice. |
| Stage A+ | `make parser-compare-test`, `make parser-dev-test` | Compare the supplied reader with the hand-written parser. |
| Stage B | `make profile-data`, `make bench`, `make memory` | Profile numeric ranges, measure the ladder and record memory use. |
| Stage C | `make replay-realtime`, `make replay-100x`, `make replay-1000x`, `make asm` | Replay trades, collect latency and lag, and generate a host assembly listing. |
| Rival | `make rival`, `make plot-rival` | Compare fills over a reaction-time sweep. |
| Race | `make race-v0` and `make race-fast` | Run the baseline and fastest cores in separate terminals with the same file and speed. |
| Stretch | `make stretch`, `make stretch-python` | Run the NEON-intrinsic kernel, SoA engine and optional Python comparison. |

Use the four audited references for copy-paste commands, source mapping, required tools and output locations: [MAKE_COMMANDS](docs/MAKE_COMMANDS.md), [FILES_BY_STAGE](docs/FILES_BY_STAGE.md), [PREREQUISITES](docs/PREREQUISITES.md), and [RESULTS_LOCATIONS](docs/RESULTS_LOCATIONS.md). The developer overview is [STAGE_GUIDE](docs/STAGE_GUIDE.md); platform procedures are in [RUNBOOK](docs/RUNBOOK.md).

## Read the results

Each engine prints one `RESULT` line with trade count, parameters, VWAP, bars, signal counts, signal hash and timing fields. Compare signal counts and hash exactly; compare VWAP within the documented tolerance. Stage 0, parser, ladder, replay, rival and Python measurements are recorded under `results/`; generated CSV schemas are documented in [docs/output_format.md](docs/output_format.md). Measurement values are machine-specific and should be produced by rerunning the corresponding program.

## Benchmarking on a laptop or Pi

Run `make profile-data` before choosing fixed-point scales. `make bench REPEAT=5` performs a discarded warm-up and repeated runs, appends ladder timing rows, and plots them if gnuplot is installed. For Pi-only pinning, run `make PI=1 bench-pi PIN_CORE=1`; `make perf` requests Linux hardware counters. Record the model, compiler, flags, governor, core affinity and command with each team measurement. Use `SUDO=sudo` on Pi replay commands when access to the real LEDs requires it.

## Project map

```text
Makefile, README.md       Build and newcomer guide
config/                   Group windows and platform flags
data/                     Separately supplied datasets
provided/                 Unmodified course helper files
src/common/               Shared indicators, timing, replay and result modules
src/stage0/ stageA/       Early stages
src/stageA_plus/ ladder/  Parser and efficiency ladder
src/stageC/               Replay, race and rival programs
src/stretch/              NEON intrinsics, SoA and exactly two Python files
tests/                    C unit/integration checks and fixtures
scripts/                  Gnuplot programs
results/ build/           Generated, ignored output
docs/                     Design, runbook, stage and evidence documentation
```

## Add a ladder version

Add a front end under `src/ladder/`, connect it to the shared engine contract, add its source and compile flags to the Makefile, include it in `test-consistency`, benchmark rows and file/output documentation, then run `make test`. Keep crossover decisions on exact scaled-integer comparisons.

## Troubleshooting

Use GNU Make, not the BSD `make` shipped with macOS. On Windows, start the UCRT64 terminal or use `mingw32-make`; MSVC is unsupported. The Makefile adds `_GNU_SOURCE` on Linux and `_POSIX_C_SOURCE=200809L` on macOS; Windows receives neither. Missing gnuplot or Python only skips the plot or Python stretch target. Normal C builds and tests do not require Python.

## Stretch goals and academic integrity

The NEON comparison currently uses compiler intrinsics and a portable scalar `-O3` reference; the generated AArch64 machine code is not verified on this Windows host. SoA v9 participates in consistency checks. Replay reports p50/p99 latency. Pure Python and NumPy are isolated to `src/stretch/python/` and `make stretch-python`.

Before submission, complete the group’s AI-assistance disclosure under course rules and review every source file as a team. Prediction-logbook entries and signatures are intentionally left for the members.

## Manual quality gate

Before sharing, each member runs `make clean && make && make test` on their own machine. To confirm the same results on another machine, run `make test-test20` and `make test-consistency` on each machine and compare the `trades`, `bars`, `buys`, `sells` and `signal_hash` fields of the printed `RESULT` lines; only timings may differ. The `results/` subfolders exist so a fresh clone can write output, and Make recreates any required folder if it was removed.
