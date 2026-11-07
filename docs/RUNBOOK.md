# Laptop and Raspberry Pi runbook

Replay, race, rival and assembly targets run on Windows, macOS, Linux and Raspberry Pi. The LED is printed on laptops and operated by the provided helper on Pi hardware; assembly listings use the host architecture. The Windows Makefile adds the `-lwinmm` timer library.

The `results/` subfolders are present so a fresh clone can write outputs immediately. Make targets recreate any required output folder if it was removed.

## Replay quick start

```sh
make replay-realtime DURATION=20
make replay-1000x FILE=data/dev_1M.csv
```

Replay options include `--file`, `--short`, `--long`, `--speed`, `--duration-sec`, `--start-offset-sec`, `--pin`, `--no-display`, `--out-dir`, `--member`, and `--help`.

Replay, race, rival and assembly targets are laptop and Pi targets. Laptops print LED state through the provided helper, while a supported Pi uses its LEDs. The Windows Makefile links `-lwinmm` for replay timer resolution. Assembly output is named for the host architecture; the annotated CSC1104 listing must come from the AArch64 Pi build.

Before sharing or submitting, each member runs `make clean && make && make test && make verify-golden` on their own machine and runs `make strict` at least once per operating system.

The annotated assembly listing for CSC1104 must come from the AArch64 Raspberry Pi build. Laptop assembly output is useful for CSC1103 and is labelled with the host architecture.
