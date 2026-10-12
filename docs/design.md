# Design notes

## High-level architecture

- `src/stage0.c` implements the Stage 0 integer-sum experiment and a guarded recursive check.
- `src/market_baseline.c` implements a streaming CSV parser and computes cumulative VWAP, SMA windows, crossover signals, and one-second OHLC aggregation.
- `scripts/project.py` coordinates builds, tests, dataset verification, and summary reporting without depending on hard-coded personal paths.
- `python/market_python.py` provides a lightweight Python reference implementation for parity checks.

## Design choices

- The market engine reads one trade row at a time and updates cumulative values incrementally.
- `short_window` and `long_window` defaults follow the assignment for `test20.csv` and can be overridden at runtime.
- The parser is intentionally compact and explicit, which makes its behavior easier to review and debug.
- The recursive Stage 0 check is intentionally bounded to prevent misleading stack overflow behavior.
