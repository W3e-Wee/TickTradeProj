# Performance methodology

## Controlled offline benchmark

The benchmark times the normal non-interactive C computation with `--bench-phases`. It does not enable per-trade traces, LED output, plotting, decision logging, or file export. Process wall time includes launch and the full executable path; the executable separately reports counting, loading, and compute phases where applicable. Compare like timing boundaries and retain the phase columns when storage variants have different load work.

Each selected version must pass the repository correctness gate before its timing rows are accepted. The gate covers the official test20 data plus development and stress datasets against frozen v1. Frozen v0 (`v0_baseline_strtok_running_double_heap_O0`) remains in every selected benchmark set and is the speedup denominator. At least five independent process repetitions reduce sensitivity to one-off scheduling and cache noise; the median is the reported central value, while every raw run remains available for inspection.

## C versus plain Python versus NumPy

`compare-python` selects a registered C reference (default v5), the one-pass plain-Python engine, and the separate batch NumPy-assisted engine. It correctness-gates test20 and the selected dataset before timing. One to four repeats are `SMOKE / NON-FINAL` and are written to a unique `python-comparison-smoke-runs` directory. Five or more repeats are `FINAL-COMPARISON ELIGIBLE` and use a unique `python-comparisons-runs` directory. A final-controlled or final-performance platform label is rejected below five repeats. The same CSV bytes and short/long windows are used, with no per-trade printing or plotting in timed computation. Outputs include `comparison_raw.csv`, `comparison_medians.csv`, `comparison.md`, `comparison.svg`, `comparison_manifest.json`, and `correctness_gate.json`.

Parity requires exact trade ordinals, buy/sell flags, warm-up readiness, signal ordinal/type, and OHLC bar seconds/counts. Floating fields printed by C at eight decimal places are compared with an absolute tolerance of `1e-7`; this tolerance is never applied to a crossover decision or signal identity.

All three process-wall samples include launch and compact summary output. The C reference also reports its native phase fields. NumPy reports CSV-to-array load/parse separately from its compute engine. The streaming Python implementation intentionally parses and computes one row at a time; a parse-only split would require buffering a large Python object graph and would change its memory behavior, so those phase cells remain blank. Throughput uses total process wall and is not interchangeable with compute-only phase rates. Python interpreter/object overhead commonly makes plain Python slower than native C. NumPy can reduce Python-loop overhead when work maps to bulk native array operations, but this engine keeps rolling SMA updates and crossover decisions in ordered Python code to preserve C signal semantics. The cache/locality and native bulk-work benefits are hypotheses until measured; NumPy is batch/offline processing and its timings do not represent timestamp-paced C replay latency. Results are reported even if NumPy is slower than either alternative.

On the Pi, install NumPy with `python3 -m pip install -r requirements-python.txt`. Run `python3 -m unittest tests.test_python_engines -v` for test20, development, and synthetic parity; the stress comparison performs its own stress correctness gate. For quick checks use, for example, `python3 scripts/project.py compare-python --dataset test20 --repeats 1 --platform-label pi-smoke-test20`. A laptop result is not Pi evidence.

A controlled Pi run should use the same dataset bytes, native build, compiler flags, window sizes, and quiet machine state. Pinning with `taskset` constrains scheduling to the chosen CPU; it does not reserve that core from the OS or other services. Record every visible CPU's governor. The workflow observes governor state and never changes it. It reports requested versus actually applied pinning. A final-controlled label is accepted only for native Linux AArch64, at least five repeats, performance governor on every CPU, and taskset successfully applied.

For Group 05, the short/long windows are 3/5 on test20 and 30/60 on dev/stress. The intended final Pi target is Pi 5/AArch64 with GCC 14.2.0, performance governor, and taskset CPU 3 where supported. Treat these as collection conditions to verify, not as established facts, until the run manifest records the hardware, compiler, governor state, and applied affinity. The current local `results/pi/` tree contains no run artifacts beyond README.md.

Host OS, architecture/model where exposed, compiler path/version, build metadata and exact flags, dataset path/size/SHA256/trade count, versions, timestamps, raw times, medians, throughput, memory metrics, and status are saved with each run. Logical allocation is the algorithm's reported reservation/payload; process peak memory is an OS measurement when available and is identified by metric.

## perf stat evidence

The raw `perf stat -x,` file is the primary capture: it preserves perf's comma-separated rows, event names (including suffixes such as `cycles:u`), unsupported-event markers, and appended process diagnostics. The profile command sets `LC_ALL=C` for a stable delimiter/number format. Raw files are never normalized in place.

`perf_summary.csv` and `perf_summary.md` are derived reports. The parser normalizes event suffixes (`cycles:u` to `cycles`, for example), accepts empty separator fields, parses integer counts, and computes IPC as instructions divided by cycles only when both counters are available and cycles are nonzero. Unsupported, not-counted, permission-denied, unreadable, and malformed values remain blank/null; they are never converted to zero. Their details appear in `counter_notes`. A summary can be rebuilt from an existing profile directory without running perf again:

```sh
python3 scripts/project.py refresh-perf-summary --profile-dir results/pi/profile-runs/pi-final-performance/<run-timestamp>
```

The profile workflow measures the normal computation-only executable path. A controlled offline benchmark result is a separate correctness-gated timing artifact with its own repetitions and metadata; perf counters explain hardware activity and do not replace that benchmark result.

## Hand-written dot-product kernel

The assembly stretch isolates one unsigned 64-bit dot product; it does not implement or replace the market engine. The C reference and assembly have the same pointer/count contract, alignment assumptions, and modulo-2^64 arithmetic. Test mode covers zero elements (including null pointers), one/fixed arrays, deterministic random arrays, repeated values, wraparound, and a boundary count. The benchmark performs these tests and a full-input checksum equality gate before timing.

Both implementations receive the same runtime-populated arrays and call count. Each of at least five repeats times only repeated kernel calls; allocation, initialization, output, and correctness checks are outside the interval. The harness alternates execution order and consumes a volatile checksum. raw.csv retains elapsed batches and checksums; median.csv reports median nanoseconds per call and throughput. The manifest records compiler, flags, source hashes, architecture, governor observations, requested/applied taskset, and timestamp. It records the portable fallback explicitly on non-AArch64 hosts.

Scalar assembly can be slower than C compiled with -O3: GCC may unroll or vectorize the reduction, schedule independent loads/multiplies, and tune for the selected microarchitecture. A measured loss is valid. NEON is optional; adding it would require a separate vector contract and comparison.

The portable test command is:

    python3 scripts/project.py asm-kernel --mode test

The native Pi comparison is:

    python3 scripts/project.py asm-kernel --mode benchmark --repeats 5 --platform-label pi-aarch64-kernel-final --taskset-cpus 3

## Replay timing and race evidence

Replay uses timestamp-paced inputs. `processing_duration_ns` is the decision work duration; `release_latency_ns` is decision completion relative to the scheduled release deadline and includes scheduling delay/lag. They are distinct from the offline benchmark's end-to-end process wall time.

The baseline-versus-fast race starts both processes from one shared monotonic epoch. The processes contend for CPU, cache, and I/O, and progress telemetry may affect later trades. Race evidence describes a concurrent workload and must not be presented as an isolated benchmark result. Keep race summaries separate from controlled benchmark and perf profile reports.

## Assembly annotation

Compiler-generated `.s` files and their manifests are retained unchanged. `annotate-assembly` reads the existing assembly, verifies its SHA256 against the adjacent `assembly_manifest.json`, and writes a reproducible `annotation.md` beside the manifest. The report cites actual source line numbers and instruction text, identifies function labels, and groups likely loads, stores, arithmetic, branches, calls, stack setup, returns, and floating-point work. The categories are heuristic and explicitly marked as automatically generated; ambiguous instructions receive a review-needed marker. Use the embedded checklist to review timing, scheduling, signal decisions, latency capture, loops, and the effects of `-O2` against the actual source and listings.

`-O2` changes instruction layout because the compiler can inline or specialize helpers, keep values in registers, reorder independent work, remove redundant branches, and choose different loop shapes. These transformations can change instruction count, register pressure, memory traffic, and branch behavior without changing the source-level algorithm. Inspect the actual generated listing; source appearance alone does not establish the resulting instruction sequence.

For example, on the Pi:

```sh
python3 scripts/project.py annotate-assembly \
  --assembly results/pi/assembly-runs/pi-final-performance/<run-timestamp>/assembly/replay_engine.s \
  --version replay-engine
```

## Pi-only outputs

Pi benchmark, perf counter, replay, LED, and compiler-generated AArch64 assembly evidence belongs under `results/pi/` and must be collected on the Pi. In this checkout, Pi evidence is PI EVIDENCE REQUIRED / NOT LOCALLY VERIFIED. Each new workflow allocates a unique timestamped directory. A host `.s` file is labeled with its host architecture and is not a substitute for Pi output. NumPy batch/offline timing is not replay latency, no engine is assumed to win, and scalar hand-written assembly may validly lose to GCC -O3.
