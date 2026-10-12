# Code walkthrough

## `src/stage0.c`

This file implements the Stage 0 sum experiment. It computes the iterative total and checks a recursion path only when the requested N is within a safe recursion budget. This prevents a misleading stack-overflow crash while still meeting the requirement to report recursion limits honestly.

## `src/market_baseline.c`

This file parses the CSV trade stream, validates rows, updates cumulative VWAP, maintains the rolling short and long windows, emits BUY/SELL crossover signals, and flushes OHLC bars by second.

## `scripts/project.py`

The Python driver builds the project and runs quick verification commands. It avoids hard-coded lecturer paths and instead discovers files in the current workspace.

## Circular buffers and running sums

The Stage A logic updates running sums rather than re-scanning the full history on each trade. This keeps the computation simple and portable while preserving the specification.

## Replay scheduling and rival logic

`src/replay_engine.c` and `src/replay_support.c` implement timestamp-paced replay and latency reporting. The Python driver and `scripts/rival.py` implement the rival reaction-time simulation and coordinated race workflow. These are implementation claims, not proof of Pi execution: local Windows replay evidence covers x1000 only, and local Pi run artifacts were unavailable in this audit. See `docs/evidence_inventory.md` for the evidence boundary.
