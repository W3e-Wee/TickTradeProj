# Design record

## Problem and data

The programs consume timestamped trade CSV rows and calculate VWAP, one-second OHLC bar counts, and short/long moving-average crossover signals. All input data remains offline. Row counts and decimal ranges are produced by `make profile-data` from the selected inputs; filenames are not treated as row-count guarantees.

## Architecture decisions

- Portable APIs live under `src/common/`; operating-system code is isolated in `platform.c` and the exact provided support files remain under `provided/`.
- Streaming paths allocate before starting the timed loop. The bounded static and exact-sized heap paths load trades before strategy processing.
- v0-v4 use naive double SMA calculations, v5 uses caller-backed compensated rolling sums, and v6-v8 use fixed-point VWAP/SMA storage. All versions use the same exact scaled-integer crossover classification so the signal sequence remains stable despite floating summation order.
- Stage A uses the supplied helper reader; Stage A+ compares that sscanf reference parser with a locale-independent decimal parser. The parser acceptance run compares every development-file row.
- OS-specific calls remain in `src/common/platform.c`. Timing and memory APIs return a documented unavailable value on unsupported systems.
- Stage C records events in caller-owned bounded storage and renders them only outside the engine step or while waiting for a later release.

## Numerical policy

The fixed scales are `PRICE_SCALE` and `QTY_SCALE` in `src/common/fixed_point.h`. For maximum observed unscaled price `P`, quantity `Q`, and row count `N` from `make profile-data`, the conservative single-product bound is `(P × PRICE_SCALE) × (Q × QTY_SCALE)` and the accumulated VWAP numerator bound is `N` times that product bound. Compare each result with `INT64_MAX`; values beyond that limit use checked wide accumulation in the portable two-limb `FpAccumulator`. The profiler writes the input-specific numeric values to `results/logs/data_profile.txt`.

Every C translation unit uses `-ffp-contract=off`. Linux builds receive `_GNU_SOURCE` from the Makefile; macOS builds receive `_POSIX_C_SOURCE=200809L`. Platform feature macros are not defined inside project source files. Windows builds receive neither macro. The build does not enable `-ffast-math`, `-Ofast`, or `-funsafe-math-optimizations`. Double VWAP output is checked with relative tolerance `1e-6`; trades, bars, signal counts, and signal hash must match exactly.

## Streaming pseudocode

```text
allocate price rings and exact scaled-price rings
for each trade:
    update floating or fixed VWAP totals
    update naive, compensated, or fixed SMA state for the selected rung
    update exact rolling scaled-price sums
    when both windows are full, compare short_sum * long_window to long_sum * short_window
    emit a signal only on a classified sign crossing after the first full-window comparison
    update the active one-second OHLC bar
flush the final bar
```

The first full-window difference establishes the previous direction and does not emit a signal. Equality is a neutral direction; a later move through zero can trigger the corresponding crossing.

## Stage algorithms

### Stage 0

```text
read N or use the documented default
for each repeat:
    turn on the green LED
    time the protected loop that sums 1 through N
    turn off the green LED
    check the result against the closed-form sum outside timing
    append the optimization and timing row
```

The recursion comparison times iterative and recursive triangular sums in batches. The benchmark rows are written after each timed batch.

### Stage A and Stage A+

```text
open the trade file with the supplied reader (Stage A) or compare both readers (Stage A+)
for each valid trade:
    update VWAP numerator and quantity totals
    update the current one-second OHLC bar
    update SMA windows and exact scaled rolling sums
    classify a BUY/SELL only when the exact sum comparison changes sign
flush the last bar and print one RESULT line
```

The parser comparison first checks each row's timestamp, price, quantity and side, then measures repeated full-file parses and appends the generated timing row.

### Stage B

```text
for each selected engine version:
    warm up once and discard that run
    execute the selected file for each requested repeat
    sort total and process durations
    append median, best, worst, throughput and peak-memory fields
```

Streaming versions include input reads in total time. In-memory versions report loading and processing intervals separately. The v7 static array is bounded by `MAX_TRADES`; v8 counts rows and allocates exact storage; v9 stores SoA columns.

### Stage C and rival simulation

```text
for each selected trade:
    calculate release = start + (timestamp - first_timestamp) / speed
    sleep for coarse gaps, then spin on the monotonic clock until release
    run the strategy step and record decision, latency and processing durations
    queue signals for later rendering and update the R=1 microsecond rival state
render at idle gaps or the periodic render deadline, draining at most the fixed batch limit
```

Replay latency is decision time minus scheduled release; processing time is the engine-step interval. The loop rejects a decision timestamp earlier than release, and lag is reported as a nonnegative value. The rival fills at the signal price only when processing time is strictly below the reaction threshold; otherwise it fills on the next trade. Pending fills at end of input are discarded, and open positions are not marked to market.

## Measurement policy

`make profile-data`, `make bench`, `make memory`, replay targets, and the rival sweep write or print measurements derived by the programs. Results depend on the host and must be rerun before use in a report. Linux performance counters and AArch64 NEON acceleration are not verified on the Windows x86 host used for this implementation.

## Replay design

For trade timestamp `ts`, the scheduled release is `start + (ts - first_ts) / speed`, with timestamps converted from microseconds to nanoseconds. The wait helper sleeps only while the gap exceeds the measured sleep granularity plus 500 microseconds, then checks the provided monotonic `now_ns()` clock until release; a trade is never passed to the engine early. Windows temporarily requests 1 ms timer resolution, and the Makefile links `-lwinmm` there.

Latency is decision time minus scheduled release time. Processing time covers only the engine step. Both measurements are stored in arrays allocated before replay starts. Current and maximum lag use nonnegative lateness, and the report includes the fraction of decisions after release. Signal events enter a fixed ring buffer in the replay loop; console output, flushes, LED calls and status updates happen during idle gaps or periodic render visits. This keeps slow terminal and LED operations out of the engine decision path.

## C versus Python

C is compiled to machine code, while ordinary Python executes bytecode through an interpreter. Python values are boxed objects with dynamic types, so each loop iteration carries extra interpreter and object-management work. A pure Python loop also has less control over memory layout and does not automatically vectorise its per-trade work. NumPy comes closer because its loops execute in compiled code over contiguous typed arrays. It still trails C when the processing pipeline needs extra passes and temporary arrays, text parsing, substantial memory traffic, and separate array operations rather than fused steps.

Use the generated rows in `results/bench/c_vs_python.csv` for reports; do not copy timings into this design record.
