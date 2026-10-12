# Codebase map

## Folders and data flow

- `src/`: Stage 0 demonstration, frozen offline baseline, compile-time comparison ladder, and timestamp-paced replay engine/support.
- `include/`: shared market and replay interfaces. `provided/` contains course-supplied helpers and retains its original source/provenance.
- `scripts/project.py`: build matrix, correctness gates, benchmark/profile/assembly orchestration, replay launch, and report generation.
- `asm/aarch64_dotprod.S`, `asm/aarch64_dotprod_ref.c`, and `asm/aarch64_dotprod_bench.c`: isolated unsigned dot-product assembly, portable C reference/fallback, correctness cases, and kernel-only benchmark.
- `scripts/rival.py`: compact replay signal ingestion, rival execution-price rule, position/P&L transitions, and reaction sweep.
- `python/market_python.py`: streaming plain-Python Stage A engine with machine-readable rows, signals, VWAP, and ordered OHLC bars.
- `python/market_numpy.py`: separate batch NumPy-assisted engine; array VWAP/OHLC operations use NumPy, while ordered SMA/crossover decisions stay in Python.
- `tests/`: C support tests plus Python market, replay, rival, and workflow regression tests.
- `docs/`: requirements, design decisions, stage notes, methodology, and handoff material.
- `results/pi/`: location for native Pi replay, performance, profile, and assembly evidence. This checkout currently contains only README.md there; Pi evidence is PI EVIDENCE REQUIRED / NOT LOCALLY VERIFIED.
- `results/laptop/`: host-specific evidence; Windows and other non-Pi outputs must remain here.
- `results/replays/`: replay artifacts from hosts that use the replay-specific convention.
- `plots/`: signal plot inputs and generated project plots. Benchmark graphs are stored with each benchmark run.

Flow: CSV dataset -> selected C build -> correctness gate -> isolated computation or timestamp-paced replay -> unique evidence directory -> CSV/JSON/Markdown reports. The offline benchmark uses `--bench-phases`; replay timing separates load, processing duration, and release-to-decision latency.

## Build flow

`python3 scripts/project.py build-all` builds Stage 0 and v0/v1 in debug/release profiles, v2-v10, eleven replay targets, and the replay-support C test executable. `make all` delegates to this same driver and canonical BuildSpecs; `make soa` builds only v10. Each C version sidecar records the compiler command, profile, source/header input hashes, and implementation selection. `make clean-build` only removes `build/`.

## Version matrix

| ID | Main change |
|---|---|
| v0 | Frozen `strtok`/`strtod`, running double SMA, heap trade array, `-O0` |
| v1 | Same frozen baseline at `-O2` |
| v2 | Supplied `sscanf` getter, naive SMA, double, streaming ring |
| v3 | Own pointer-output reader, naive SMA, double, streaming ring |
| v4 | Manual digit parser, naive SMA, double, streaming ring |
| v5 | Manual parser, running double SMA, double, streaming ring |
| v6 | Manual parser, running checked fixed-point SMA, streaming ring |
| v7 | v5 configuration at `-O3` |
| v8 | v5 configuration with a global static trade array |
| v9 | v5 configuration with exact two-pass heap allocation |
| v10 | v9 parser/SMA/numeric/compiler setup with four exact-sized SoA arrays |

The canonical compiler flags and detailed selection strings live in `scripts/project.py` and `docs/version_manifest.md`. v10 stores timestamps, prices, quantities, and buy/sell flags in separate arrays; it does not wrap or store `Trade` records. Its replay target is compatible with the v10 strategy and input contract, while replay itself explicitly continues to use its independent streaming storage backend. Frozen v0 remains the speedup denominator.

## Assignment stages and modules

- Stage 0 arithmetic: `src/stage0.c`.
- Offline market engine and baseline: `src/market_baseline.c`, `include/market.h`.
- Reader/SMA/representation/storage comparisons: `src/market_variant.c`; isolated v10 SoA implementation: `src/market_soa.c`.
- Replay deadlines, release lag, latency summaries, and LED hooks: `src/replay_engine.c`, `src/replay_support.c`, `include/replay_support.h`.
- Rival P&L and reaction sweep: `scripts/rival.py`, launched by `scripts/project.py rival`.
- Concurrent baseline/fast replay race: `scripts/project.py race`.
- AArch64 kernel stretch: asm-kernel test runs native assembly on Linux AArch64 and the portable fallback elsewhere. The five-repeat Pi comparison remains to be collected on the Pi. NEON is optional.

## Common commands

```sh
python3 scripts/project.py doctor
python3 scripts/project.py build --profile release
python3 scripts/project.py build-soa
python3 scripts/project.py test --suite quick
python3 scripts/project.py verify --all-versions
python3 scripts/project.py benchmark --dataset stress_full --repeats 5
python3 scripts/project.py compare-python --dataset stress_full --repeats 5 --platform-label pi-c-python-numpy-final --taskset-cpus 3
python3 scripts/project.py benchmark --dataset stress_full --repeats 5 --versions v0_baseline_strtok_running_double_heap_O0,v1_baseline_strtok_running_double_heap_O2 --platform-label pi-final-performance --taskset-cpus 3 --final-controlled
python3 scripts/project.py benchmark --dataset stress_full --repeats 5 --versions v0_baseline_strtok_running_double_heap_O0,v9_manual_running_double_exact_malloc_trade_array_O2,v10_manual_running_double_soa_O2 --platform-label pi-soa-aos-final --taskset-cpus 3 --final-controlled
python3 scripts/project.py profile --dataset stress_full --versions v0_baseline_strtok_running_double_heap_O0,v1_baseline_strtok_running_double_heap_O2 --taskset-cpus 3 --platform-label pi-final-performance
python3 scripts/project.py assembly --version v1_baseline_strtok_running_double_heap_O2 --platform-label pi-final-performance
python3 scripts/project.py assembly --version replay-engine --platform-label pi-final-performance
python3 scripts/project.py asm-kernel --mode test
python3 scripts/project.py asm-kernel --mode benchmark --repeats 5 --platform-label pi-aarch64-kernel-final --taskset-cpus 3
```

Each command's `--help` output is authoritative. On the current Windows host, benchmark outputs route to `results/laptop`; only native Linux AArch64 runs route to `results/pi` and can produce Pi evidence.
