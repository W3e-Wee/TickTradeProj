# Laptop results

Generated on the Windows 11 AMD64 host with GCC 16.2.0, Python 3.13.14, and the local `stress_full.csv` (8,928,944 trades; SHA-256 `863599f5858af2c21d153eeda8168dc298b8bc748d6c3a224efc59c3bca8d663`). Five process-level repeats were run per version after test20, dev, and stress correctness gates passed. The host governor is not exposed by the Linux governor interface; the raw records say `not_available_on_host`. No CPU affinity or system settings were changed.

- `correctness_matrix.csv`: test20 official oracle plus full dev/stress parity against frozen v1.
- `benchmark_raw.csv`: each repeat, exact compiler command, source/dataset identity, phase timings, throughput inputs, and memory metadata.
- `benchmark_medians.csv`: medians and throughput by version.
- `speedup_table.csv`, `speedup_ladder.dat`, `speedup_ladder.gnuplot`, `speedup_ladder.png`: speedups versus frozen v0.
- `controlled_comparisons.csv`: paired parser/compiler/SMA/representation/storage comparisons.
- `memory_comparison.csv`: logical reservations/payloads separated from OS process-memory measurements.

The Pi remains unmeasured here. Use the exact native build/test/verify/benchmark commands and optional `taskset` pinning in `docs/benchmark_methodology.md`; do not change the governor automatically.
