# Results locations and formats

Output paths and headers below are taken from the Makefile and source. Generated paths are created by the listed program/target. Paths marked **PATH FROM CODE, NOT RUN HERE** are conditional or platform-specific and must be checked on that platform before collecting results.

## Generated results

| Path | Producer | Format/header | Status |
| --- | --- | --- | --- |
| `results/bench/stage0_sum.csv` | `run-stage0` / `stage0_sum_loop` | `opt_level,mode,n,repeats,best_ns,median_ns,worst_ns,ns_per_add,sum` | GENERATED HERE |
| `results/bench/stage0_recursion.csv` | `run-stage0` / `stage0_sum_recursion` | `opt_level,mode,n,reps,batches,iterative_median_ns,recursive_median_ns,ratio` | GENERATED HERE |
| `results/bench/parser.csv` | `parser_compare` | `file,rows,repeats,sscanf_median_ns,fast_median_ns,speedup` | GENERATED HERE |
| `results/logs/data_profile.txt` | `profile-data` | Human-readable data profile; no CSV header | GENERATED HERE |
| `results/bench/ladder_<platform>_<member>.csv` | `bench` | `platform,member,version,file,trades,repeats,median_total_ns,best_total_ns,worst_total_ns,median_process_ns,trades_per_sec,peak_mem_kb,compiler,flags` | GENERATED HERE |
| `results/bench/memory_<platform>.csv` | `memory` | Same ladder CSV header; one measured row per engine | GENERATED HERE |
| `results/bench/c_vs_python.csv` | `stretch-python` | `platform,member,implementation,file,trades,short,long,limit,repeats,median_total_ns,c_total_ns,times_slower_than_c,matches_c,python_version,numpy_version` | GENERATED HERE (fixture; NumPy row skipped) |
| `results/replay/<platform>_<member>_<core>_<speed>x.csv` | Replay program | `platform,member,core,speed,trades_replayed,market_seconds,wall_seconds,avg_latency_ns,worst_latency_ns,p50_latency_ns,p99_latency_ns,avg_processing_ns,max_lag_us,pct_behind,dropped_events,buys,sells,pnl_vs_rival_r1` | GENERATED HERE |
| `results/replay/summary_<platform>_<member>.csv` | Replay program | Per-run summary with the replay CSV schema above | GENERATED HERE |
| `results/rival/sweep.csv` | `rival` | `platform,member,reaction_us,pnl,money_lost,trades` | PATH FROM CODE, NOT RUN HERE |
| `results/plots/signals.dat` | `plot-signals` | Tab-separated signal trace; columns are declared by the Stage A exporter | PATH FROM CODE, NOT RUN HERE |
| `results/plots/signals.png` | gnuplot `plot_signals.gp` | PNG chart | PATH FROM CODE, NOT RUN HERE |
| `results/plots/money_lost.png` | gnuplot `plot_money_lost.gp` | PNG chart | PATH FROM CODE, NOT RUN HERE |
| `results/plots/ladder_<platform>_<member>.png` | gnuplot `plot_ladder.gp` | PNG chart | PATH FROM CODE, NOT RUN HERE |
| `results/asm/<version>_<architecture>.s` | `asm` | Compiler assembly listing | GENERATED HERE |
| `build/<platform>/python_c_reference.txt` | `stretch-python` | C `RESULT` record used for comparison | GENERATED HERE |
| `build/<platform>/*.exe` or `build/<platform>/*` | `all`, stage/ladder/replay/stretch/test builds | Compiler outputs: programs, test runners, object and dependency files | GENERATED HERE (Windows build) |
| `results/bench/ladder_<platform>_<member>.csv` (v9_soa row) | Struct-of-Arrays stretch rung | Same ladder CSV header above | GENERATED HERE |
| `results/replay/<platform>_<member>_<core>_<speed>x.csv` | Percentile-latency stretch | Replay schema above, including `p50_latency_ns` and `p99_latency_ns` | GENERATED HERE |
| `results/bench/c_vs_python.csv` | C-versus-Python stretch | Python comparison header above | GENERATED HERE (pure Python passed; NumPy unavailable) |
| Console output | NEON stretch | Console summary compares scalar and intrinsic kernel; no separate CSV | PATH FROM CODE, NOT RUN HERE |

Programs print a machine-readable `RESULT` line with `version`, `file`, trade/window counts, VWAP, bars, buys/sells, `signal_hash`, load/process/total nanoseconds, and peak memory. Console summaries are not appended to CSV unless the row is listed above.

## Result directories

The Makefile recreates `results/logs`, `results/perf`, `results/plots`, `results/bench`, `results/replay`, `results/rival`, and `results/asm`. The tracked `.gitkeep` placeholders live in each directory. `make clean` removes `build/`; `make distclean` removes generated result files but preserves placeholders.

`results/perf/` is reserved for platform perf captures; the current `perf` target reports counters to the terminal and does not write a file. Do not infer a generated file from the directory name.

Stretch output coverage: NEON emits its comparison in the terminal; SoA v9 uses the ladder CSV; percentile latency uses the replay CSV; and C-versus-Python uses `c_vs_python.csv`. Each CSV has one schema above; the SoA row shares the ladder schema.
