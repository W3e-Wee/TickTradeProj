# Design record

## Problem and data

The programs consume timestamped trade CSV rows and calculate VWAP, one-second OHLC bar counts, and short/long moving-average crossover signals. All input data remains offline. The C profiler measured 8,928,944 records in both large supplied CSVs; filenames are not treated as row-count guarantees.

## Architecture decisions

- Portable APIs live under `src/common/`; operating-system code is isolated in `platform.c` and the exact provided support files remain under `src/provided/`.
- Streaming paths allocate before starting the timed loop. The bounded static and exact-sized heap paths load trades before strategy processing.
- v0-v4 use naive double SMA calculations, v5 uses caller-backed compensated rolling sums, and v6-v8 use fixed-point VWAP/SMA storage. All versions use the same exact scaled-integer crossover classification so the signal sequence remains stable despite floating summation order.
- Stage A uses the supplied helper reader; Stage A+ compares that sscanf reference parser with a locale-independent decimal parser. The parser acceptance run compares every development-file row.
- OS-specific calls remain in `src/common/platform.c`. Timing and memory APIs return a documented unavailable value on unsupported systems.

## Numerical policy

The C profiler measured eight decimal places in supplied price and quantity fields. The selected scales are `PRICE_SCALE=100000000` and `QTY_SCALE=100000000`. It measured a maximum price of 79,500, maximum quantity of 18.29992, and 8,928,944 rows in each large input. With those scales, the conservative worst-case scaled product is approximately `1.45484364e22` and the conservative full-file numerator bound is approximately `1.29902174e29`; both exceed signed 64-bit capacity. Products therefore accumulate in the portable two-limb `FpAccumulator`.

Every C translation unit uses `-ffp-contract=off`. The build does not enable `-ffast-math`, `-Ofast`, or `-funsafe-math-optimizations`. Double VWAP output is checked with relative tolerance `1e-6`; trades, bars, signal counts, and signal hash must match exactly.

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

## Measurement policy

`make profile-data`, `make bench`, `make memory`, replay targets, and the rival sweep write or print measurements derived by the programs. Results depend on the host and must be rerun before use in a report. Linux performance counters and AArch64 NEON acceleration are not verified on the Windows x86 host used for this implementation.
