# Tick-to-Trade on the Edge

This C11 coursework project reads Binance-style trade CSV files, computes VWAP, one-second OHLC bars and moving-average crossover signals, then compares streaming and in-memory implementations. Replay is offline and does not connect to an exchange or place orders.

## Platforms and prerequisites

The project targets 64-bit Linux, macOS and Windows with GCC or Clang and GNU Make. Windows builds are tested with MSYS2 UCRT64 GCC/Make or `mingw32-make`. Optional tools are gnuplot for PNG output, Linux `perf` for hardware counters, and an AArch64 Raspberry Pi for the Pi benchmark.

Install a C compiler and Make using your platform package manager. Install gnuplot only when you want PNG plots. The four course data files are already in `data/`; see [data/README.md](data/README.md). Keep supplied inputs unchanged.

## Build and run

```sh
make
make test
make run-stage0
make run-stageA
make test-test20
make test-consistency
```

Select the group’s window sizes with `make GROUP=07 ...`. `config/groups.mk` records all 21 short/long window pairs. Individual ladder programs accept `--file`, `--short`, and `--long`; run `build/<platform>/v8 --help` for the usage line.

## Stages and targets

| Target | Result |
| --- | --- |
| `all` | Build Stage A, parser/profile/platform tools, Stage 0 variants and ladder v0–v8. |
| `stage0` / `run-stage0` | Build/run O0–O3 loop and recursion benchmarks, guarded and plain. |
| `stageA` / `run-stageA` | Build/run the streaming CSV strategy on test20 and development data. |
| `stageA_plus` / `parser_compare` | Build parser comparison; `make parser-dev-test` checks every development row. |
| `ladder` | Build v0–v8. v0/v1/v2 use O0/O2/O3, v3 accepts optional `PI_CFLAGS`, v4 uses the manual parser, v5 uses rolling sums, v6 uses fixed point, and v7/v8 preload bounded static/exact-sized arrays. |
| `test-test20` | Compare ladder outputs with each other and the supplied expected summary. |
| `verify-golden` | Check the supplied test20 summary against the ladder and Stage A. |
| `test-consistency` | Check development and stress datasets across ladder versions. |
| `profile-data` | Write measured numeric ranges and fixed-point sizing to `results/logs/data_profile.txt`. |
| `bench` / `memory` | Run selected ladder versions and report timing/peak-memory fields from their RESULT records. |
| `replay-realtime`, `replay-100x`, `replay-1000x` | Replay timestamps at the selected speed and summarize lag percentiles. |
| `race-v0` / `race-fast` | Run baseline and fastest-ladder candidates over the same development data. |
| `rival` | Generate a 25-point reaction-time sweep in `results/rival/sweep.csv`. |
| `asm` | Write a host compiler assembly listing to `results/asm/engine.s`. |
| `plot-signals` / `plots` | Export a 2,000-trade signal slice; render it when gnuplot is installed. |
| `stretch` / `stretch-python` | Run the SoA/NEON comparison and optional independent Python reference. |
| `perf` | Run Linux hardware counters; reports NOT VERIFIED on Windows. |
| `bench-pi` | Run the ladder benchmark workflow; Pi-specific results require running on a Raspberry Pi. |
| `strict` | Clean rebuild with warnings treated as errors and run the full test suite. |
| `clean` / `distclean` | Remove build products; distclean also removes generated results. |

All engines emit exactly one `RESULT` record; fields are described in [docs/output_format.md](docs/output_format.md). Timing values are machine-specific. Do not copy them into reports without rerunning the command on the named hardware. The C profiler measures the supplied files; it does not assume their names describe their row counts.

## Results and reports

Generated artifacts live under `results/`. `make plot-signals` creates `results/plots/signals.dat`; with gnuplot it also creates the PNG. `make rival` writes `results/rival/sweep.csv`. The corresponding gnuplot templates are under `scripts/`. The design, architecture, numerical policy, migration record and output contract are in `docs/`.

For Raspberry Pi measurements, record the exact model, compiler flags, governor, core affinity and command. `make perf` is Linux-only; hardware counters and Pi-specific timings are NOT VERIFIED on other hosts. The source uses `-ffp-contract=off`; it does not enable unsafe floating-point transformations.

## Layout

`src/common/` contains portable shared modules; `src/provided/` preserves the supplied helper files; `src/stage0/`, `src/stageA/`, `src/stageA_plus/`, `src/ladder/`, `src/stageC/`, `src/tools/` and `src/stretch/` contain their respective programs. `tests/` contains unit and integration checks. Course inputs remain in `data/`.

## Academic integrity

Before submission, complete the group’s AI-assistance disclosure in line with course rules and review every source file as a team.
