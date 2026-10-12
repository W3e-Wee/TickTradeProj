# Raspberry Pi evidence handoff

The current Windows checkout has no inspectable Pi run artifacts under `results/pi/` beyond its README. Every Pi result remains **PI EVIDENCE REQUIRED / NOT LOCALLY VERIFIED** until the corresponding raw files and manifests are copied into the checkout and reviewed. Do not infer a completed run from a command example or a historical note.

The intended controlled target is a Raspberry Pi 5 with AArch64 and GCC 14.2.0, performance governor, and taskset CPU 3 where the workflow supports it. Confirm these values and the applied affinity from the Pi itself and each run manifest before using them as report facts.

## Final non-destructive readiness capture

From the repository root on the Pi, this runs the read-only audit and saves a uniquely timestamped transcript without replacing an earlier capture:

```sh
mkdir -p results/pi/readiness
python3 scripts/project.py readiness | tee "results/pi/readiness/readiness-final-$(date -u +%Y%m%dT%H%M%SZ).txt"
```

The readiness command itself does not build, benchmark, rewrite evidence, or require optional datasets. Review its source/layout return code and the Pi/manual items separately.

## Collection sequence

1. Record `uname -a`, `gcc --version`, Pi model, all visible CPU governors, and the actual `taskset` CPU list in the run manifests. Do not change the governor without recording the procedure and state.
2. Build and run `make all`, `make test`, and `make verify`; confirm test20 uses Group 05 short/long windows 3/5, and dev/stress use 30/60.
3. Run Stage 0 timing at O0/O1/O2/O3 and the permitted LED check. Save raw output with compiler flags and host details.
4. Run the full correctness-gated controlled benchmark with at least five repeats, then profile, storage comparison, replay at 1x/100x/1000x, and a real-time run of at least 600 seconds. Keep raw outputs and manifests.
5. Capture LED/terminal output, latency mean/worst and p50/p99 where available, rival reaction sweep, Version 0 versus selected-fast race, and the required demo video.
6. Generate assembly on the Pi, retain its manifest, then annotate that exact assembly file. Run the native AArch64 kernel correctness gate before its five-repeat benchmark.
7. Copy the original Pi evidence into the expected `results/pi/` categories without changing raw files. Re-run readiness and manually inspect each manifest and report claim.

Use the command examples in `docs/performance_methodology.md` and `docs/evidence_inventory.md`. They describe collection workflows, not evidence already present here.
