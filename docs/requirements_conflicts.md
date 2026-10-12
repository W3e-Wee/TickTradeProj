# Requirements conflict log

The authoritative assignment PDF is present at `provided/CSC1103-1104_Mini_Project_2026-27_Tick-to-Trade_final.pdf`. The supplied helper sources `provided/trade_io.c`, `provided/trade_io.h`, and `provided/timing.h` are also present. The PDF is the authoritative source for coursework behavior.

Notable findings:

- Local filenames include copy suffixes; the driver selects the supplied CSV without renaming it.
- Group settings are recorded in `group_info - Copy (6).txt`: test20 uses short=3/long=5; dev and stress use short=30/long=60.
- `test20_expected - Copy (6).txt` provides official per-trade VWAP/SMA/warm-up/signal values and every OHLC bar; the verifier compares all of them.

The version registry must describe actual parser, SMA, representation, storage, and compiler settings. `scripts/project.py verify --all-versions` fails if any required version is missing or unimplemented.
