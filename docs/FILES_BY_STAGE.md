# Files by stage

This map reflects the direct Makefile source lists and generated `-MMD -MP` dependency files on the Windows build. Shared-module use follows those include/build dependencies; each file under `src/`, `tests/`, `scripts/` and `config/` is indexed in the closing check.

## Stage 0

| Category | Paths | Role |
| --- | --- | --- |
| Stage sources | `src/stage0/stage0_math.c`, `src/stage0/stage0_math.h`, `src/stage0/stage0_sum_loop.c`, `src/stage0/stage0_sum_recursion.c` | Checked sums and optimization comparisons. |
| Shared modules | `src/common/cli.c`, `src/common/cli.h`, `src/common/compat.h`, `src/common/exit_codes.h` | Numeric argument checks, optimizer barriers and shared exits. |
| Provided files | `provided/timing.h`, `provided/led.h`, `provided/test_helpers.c` | Clock, LED and required helper check. |
| Scripts | — | Stage 0 has no plot script. |
| Tests and fixtures | `tests/test_stage0_math.c`, `tests/test_compat.c`, `tests/test_exit_codes.c`, `tests/check.h` | Sum and common helper checks. |
| Configuration | `config/platform_flags.mk` | Common compile flags. |
| Data inputs | — | Stage 0 takes numeric CLI inputs. |

## Stage A

| Category | Paths | Role |
| --- | --- | --- |
| Stage sources | `src/stageA/stageA_stream.c` | Streaming Stage A program and optional signal export. |
| Shared modules | `src/common/cli.c`, `src/common/cli.h`, `src/common/exit_codes.h`, `src/common/fixed_point.c`, `src/common/fixed_point.h`, `src/common/result.c`, `src/common/result.h`, `src/common/platform.c`, `src/common/platform.h` | CLI, exact crossover, result output, timing and memory services. |
| Provided files | `provided/trade_io.c`, `provided/trade_io.h`, `provided/timing.h` | Supplied stream reader and clock. |
| Scripts | `scripts/plot_signals.gp` | Signal chart template. |
| Tests and fixtures | `tests/test_trade.c`, `tests/test_result.c`, `tests/test_fixed_point.c`, `tests/fixtures/trades.csv`, `data/test20_expected.txt` | Input, result and course-oracle checks. |
| Configuration | `config/groups.mk`, `config/platform_flags.mk` | Window selection and platform flags. |
| Data inputs | `data/test20.csv`, `data/test20_expected.txt`, `data/dev_1M.csv` | Course oracle and development input. |

## Stage A+

| Category | Paths | Role |
| --- | --- | --- |
| Stage sources | `src/stageA_plus/reader.c`, `src/stageA_plus/reader.h`, `src/stageA_plus/fast_parser.c`, `src/stageA_plus/fast_parser.h`, `src/stageA_plus/parser_compare.c` | Reference and character-walking parser plus row/timing comparison. |
| Shared modules | `src/common/exit_codes.h` | Standard parser exit values. |
| Provided files | `provided/trade_io.c`, `provided/trade_io.h`, `provided/timing.h` | Reference reader and monotonic clock. |
| Scripts | — | Parser comparisons are tabular, not plotted. |
| Tests and fixtures | `tests/test_parser.c`, `tests/check.h`, `tests/fixtures/trades.csv` | Header, CRLF, decimal and boolean parser equivalence. |
| Configuration | `config/platform_flags.mk` | Shared compile flags. |
| Data inputs | `data/dev_1M.csv` | Full parser comparison input. |

## Stage B

| Category | Paths | Role |
| --- | --- | --- |
| Stage sources | `src/ladder/engine.c`, `src/ladder/engine.h`, `src/ladder/v0_baseline.c`, `src/ladder/v4_fast_parser.c`, `src/ladder/v5_running_sum.c`, `src/ladder/v6_fixed_point.c`, `src/ladder/v7_in_memory.c`, `src/ladder/v8_malloc_exact.c`, `src/stretch/soa/v9_soa.c`, `src/stretch/soa/v9_soa.h`, `src/tools/data_profile.c` | Ladder core, rung selection, SoA rung and data profiler. |
| Ladder compile flags | `v0`: `-O0`; `v1`: `-O2`; `v2`: `-O3`; `v3`: `$(OPT)` plus `-mcpu=cortex-a76` under `PI=1`; `v4`–`v9_soa`: `$(OPT)`, default `-O2`; `v7` additionally defines `ENGINE_STATIC_STORAGE` and `MAX_TRADES`. | Values are taken from the current Makefile recipes. |
| Shared modules | `src/common/bench.c`, `src/common/bench.h`, `src/common/cli.c`, `src/common/cli.h`, `src/common/compat.h`, `src/common/exit_codes.h`, `src/common/fixed_point.c`, `src/common/fixed_point.h`, `src/common/indicators.c`, `src/common/indicators.h`, `src/common/memusage.c`, `src/common/memusage.h`, `src/common/platform.c`, `src/common/platform.h`, `src/common/result.c`, `src/common/result.h`, `src/common/signal_rule.c`, `src/common/signal_rule.h`, `src/common/trade.h` | Engine input, arithmetic, indicators, timing, memory and output contracts. |
| Provided files | `provided/trade_io.c`, `provided/trade_io.h`, `provided/timing.h` | Baseline parser and clock. |
| Scripts | `scripts/plot_ladder.gp` | Ladder median-time chart. |
| Tests and fixtures | `tests/test_consistency.c`, `tests/test_bench.c`, `tests/test_fixed_point.c`, `tests/test_indicators.c`, `tests/test_memusage.c`, `tests/test_result.c`, `tests/test_trade.c`, `tests/check.h`, `tests/fixtures/trades.csv` | Rung equality, arithmetic, indicator and result checks. |
| Configuration | `config/groups.mk`, `config/platform_flags.mk` | Windows by group and architecture flags. |
| Data inputs | `data/test20.csv`, `data/test20_expected.txt`, `data/dev_1M.csv`, `data/stress_full.csv` | Course oracle, development and final comparison inputs. |

## Stage C

| Category | Paths | Role |
| --- | --- | --- |
| Stage sources | `src/stageC/replay_cli.c`, `src/stageC/replay_engine.c`, `src/stageC/replay_engine.h`, `src/stageC/replay_main.h`, `src/stageC/replay_v0.c`, `src/stageC/replay_fast.c` | Replay selection, scheduler loop and baseline/fast cores. |
| Shared modules | `src/common/cli.c`, `src/common/cli.h`, `src/common/exit_codes.h`, `src/common/fixed_point.c`, `src/common/fixed_point.h`, `src/common/indicators.c`, `src/common/indicators.h`, `src/common/latency.c`, `src/common/latency.h`, `src/common/memusage.c`, `src/common/memusage.h`, `src/common/platform.c`, `src/common/platform.h`, `src/common/replay.c`, `src/common/replay.h`, `src/common/replay_events.c`, `src/common/replay_events.h`, `src/common/result.c`, `src/common/result.h`, `src/common/rival.c`, `src/common/rival.h`, `src/common/signal_display.c`, `src/common/signal_display.h`, `src/common/signal_rule.c`, `src/common/signal_rule.h` | Scheduling, event buffering, deferred display, latency and result logic. |
| Provided files | `provided/led.h`, `provided/timing.h`, `provided/trade_io.c`, `provided/trade_io.h` | LED, clock and baseline reader. |
| Scripts | — | Replay results are recorded as CSV. |
| Tests and fixtures | `tests/test_replay.c`, `tests/test_replay_events.c`, `tests/test_replay_output.c`, `tests/test_latency.c`, `tests/test_platform.c`, `tests/test_signal_display.c`, `tests/fixtures/replay.csv`, `tests/check.h` | Schedule, event, output, percentile and platform checks. |
| Configuration | `config/groups.mk`, `config/platform_flags.mk` | Replay windows and compile flags. |
| Data inputs | `data/stress_full.csv`, `data/dev_1M.csv`, `tests/fixtures/replay.csv` | Full replay and bounded pacing fixture. |

## Rival bot demonstration

| Category | Paths | Role |
| --- | --- | --- |
| Stage sources | `src/stageC/rival_sweep.c` | Read signals and write reaction-time sweep rows. |
| Shared modules | `src/common/rival.c`, `src/common/rival.h`, `src/common/platform.c`, `src/common/platform.h`, `src/common/fixed_point.c`, `src/common/fixed_point.h`, `src/common/cli.c`, `src/common/result.c`, `src/common/latency.c`, `src/common/latency.h` | Fill rules, parser support, timing and result metadata. |
| Provided files | — | The sweep uses the Stage A+ parser. |
| Scripts | `scripts/plot_money_lost.gp` | Reaction-time P&L chart. |
| Tests and fixtures | `tests/test_rival.c`, `tests/check.h` | Fill boundary and P&L behavior. |
| Configuration | `config/groups.mk` | Default short/long windows. |
| Data inputs | `data/stress_full.csv`, `tests/fixtures/trades.csv` | Sweep input and small validation data. |

## Race demonstration

| Category | Paths | Role |
| --- | --- | --- |
| Stage sources | `src/stageC/replay_v0.c`, `src/stageC/replay_fast.c`, `src/stageC/replay_cli.c`, `src/stageC/replay_engine.c`, `src/stageC/replay_engine.h`, `src/stageC/replay_main.h` | Baseline and fastest replay processes. |
| Shared modules | `src/common/platform.c`, `src/common/platform.h`, `src/common/replay.c`, `src/common/replay.h`, `src/common/replay_events.c`, `src/common/replay_events.h`, `src/common/latency.c`, `src/common/latency.h`, `src/common/rival.c`, `src/common/rival.h`, `src/common/signal_display.c`, `src/common/signal_display.h`, `src/common/result.c`, `src/common/result.h` | Timing, replay state, event output and per-run summaries. |
| Provided files | `provided/led.h`, `provided/timing.h`, `provided/trade_io.c`, `provided/trade_io.h` | Platform helper services. |
| Scripts | — | Compare the separate replay CSVs or terminal summaries. |
| Tests and fixtures | `tests/test_replay.c`, `tests/test_replay_output.c`, `tests/fixtures/replay.csv`, `tests/check.h` | Replay timing and output validation. |
| Configuration | `config/groups.mk`, `config/platform_flags.mk` | Windows and compiler selection. |
| Data inputs | `data/stress_full.csv`, `tests/fixtures/replay.csv` | Shared race input and smoke fixture. |

## Stretch goals

### NEON kernel

| Category | Paths | Role |
| --- | --- | --- |
| Sources | `src/stretch/neon/neon_sum.c`, `src/stretch/neon/neon_scalar.c`, `src/common/platform.c`, `src/common/platform.h` | NEON intrinsics and portable scalar `-O3` reference. No hand-written `.S` file is present. |
| Tests | `make stretch` worked-value check | Exercises the kernel comparison on the host. |
| Configuration | `config/platform_flags.mk`, `PI_CFLAGS`, `NEON_CFLAGS` in `Makefile` | Optional AArch64 tuning flags. |

### Struct-of-Arrays

| Category | Paths | Role |
| --- | --- | --- |
| Sources | `src/stretch/soa/v9_soa.c`, `src/stretch/soa/v9_soa.h`, `src/stageA_plus/fast_parser.c`, `src/common/fixed_point.c`, `src/common/result.c`, `src/common/platform.c` | SoA reader, exact crossover and shared result output. |
| Tests | `tests/test_consistency.c`, `tests/fixtures/trades.csv` | v9 is checked against v0 by the consistency harness. |
| Data | `data/test20.csv`, `data/dev_1M.csv`, `data/stress_full.csv` | Course-oracle and full consistency inputs. |

### Percentile latency

| Category | Paths | Role |
| --- | --- | --- |
| Sources | `src/common/latency.c`, `src/common/latency.h`, `src/stageC/replay_cli.c` | Caller-owned sample buffer and nearest-rank p50/p99 replay reporting. |
| Tests | `tests/test_latency.c`, `tests/test_replay_output.c` | Percentile math and CSV schema. |
| Data | `tests/fixtures/replay.csv`, `data/stress_full.csv` | Replay fixture and full input. |

### C versus Python

| Category | Paths | Role |
| --- | --- | --- |
| Sources | `src/stretch/python/stage_a.py`, `src/stretch/python/stage_a_numpy.py` | Exactly two optional Python comparison files. |
| Shared C reference | `src/stageA/stageA_stream.c`, `src/common/fixed_point.c`, `src/common/result.c`, `provided/trade_io.c` | C Stage A reference invoked only by `stretch-python`. |
| Data | `tests/fixtures/trades.csv`, `data/test20.csv`, `data/dev_1M.csv`, `data/stress_full.csv` | Fixture, correctness and full comparison inputs. |

## Tests and verification

| Category | Paths | Role |
| --- | --- | --- |
| Test programs | `tests/check.h`, `tests/test_bench.c`, `tests/test_cli.c`, `tests/test_compat.c`, `tests/test_consistency.c`, `tests/test_exit_codes.c`, `tests/test_fixed_point.c`, `tests/test_indicators.c`, `tests/test_latency.c`, `tests/test_memusage.c`, `tests/test_parser.c`, `tests/test_platform.c`, `tests/test_replay.c`, `tests/test_replay_events.c`, `tests/test_replay_output.c`, `tests/test_result.c`, `tests/test_rival.c`, `tests/test_signal_display.c`, `tests/test_stage0_math.c`, `tests/test_trade.c` | Unit and integration test sources. |
| Fixtures | `tests/fixtures/replay.csv`, `tests/fixtures/trades.csv` | Bounded replay and market input. |
| Shared test modules | Common modules are enumerated under Stage B and Stage C above. | Each test links the modules listed in its Makefile recipe. |
| Configuration | `config/groups.mk`, `config/platform_flags.mk` | Test windows and platform build flags. |
| Data inputs | `data/test20.csv`, `data/test20_expected.txt`, `data/dev_1M.csv`, `data/stress_full.csv` | Course oracle and cross-version input. |

## General tools and configuration

| Category | Paths | Role |
| --- | --- | --- |
| Tools | `src/tools/data_profile.c`, `src/tools/platform_check.c` | Numeric profile and platform prerequisite inspection. |
| Shared modules | `src/common/bench.c`, `src/common/bench.h`, `src/common/cli.c`, `src/common/cli.h`, `src/common/compat.h`, `src/common/exit_codes.h`, `src/common/fixed_point.c`, `src/common/fixed_point.h`, `src/common/indicators.c`, `src/common/indicators.h`, `src/common/latency.c`, `src/common/latency.h`, `src/common/memusage.c`, `src/common/memusage.h`, `src/common/platform.c`, `src/common/platform.h`, `src/common/replay.c`, `src/common/replay.h`, `src/common/replay_events.c`, `src/common/replay_events.h`, `src/common/result.c`, `src/common/result.h`, `src/common/rival.c`, `src/common/rival.h`, `src/common/signal_display.c`, `src/common/signal_display.h`, `src/common/signal_rule.c`, `src/common/signal_rule.h`, `src/common/trade.h` | Shared headers and implementations used across the listed stages and tests. |
| Scripts | `scripts/plot_ladder.gp`, `scripts/plot_money_lost.gp`, `scripts/plot_signals.gp` | The project's gnuplot programs. |
| Configuration | `config/groups.mk`, `config/platform_flags.mk` | Group windows and compile switches. |

## File coverage check

| Tree | Files indexed | Check |
| --- | --- | --- |
| `src/` | 62 files, all listed in the stage and shared-module tables above. | 62 indexed; 0 missing in the filesystem inventory. |
| `tests/` | 22 test and fixture files. | 22 indexed; 0 missing in the filesystem inventory. |
| `scripts/` | `scripts/plot_ladder.gp`, `scripts/plot_money_lost.gp`, `scripts/plot_signals.gp`. | 3 indexed; 0 missing in the filesystem inventory. |
| `config/` | `config/groups.mk`, `config/platform_flags.mk`. | 2 indexed; 0 missing in the filesystem inventory. |

**Inventory check:** `rg --files src tests scripts config` returned 89 paths: 62 under `src/`, 22 under `tests/`, 3 under `scripts/`, and 2 under `config/`. Each is represented in the stage-category tables above. Rerun this command after adding or removing files.
