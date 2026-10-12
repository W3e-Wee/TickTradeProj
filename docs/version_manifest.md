# Version manifest

The active comparison matrix is built by `python scripts/project.py build --profile debug` and `python scripts/project.py build --profile release`. Each executable has an adjacent `.exe.json` sidecar containing its exact compiler command, compiler version, source hashes, and selected implementation. Variant builds use common flags `-std=c11 -Wall -Wextra -Wpedantic -Werror -DNDEBUG -Iinclude -Iprovided` plus the rung-specific flags below.

| Version ID | Executable | Build configuration and actual selection |
| --- | --- | --- |
| `v0_baseline_strtok_running_double_heap_O0` | `build/debug/market_baseline.exe` | `-std=c11 -Wall -Wextra -Wpedantic -g3 -O0 -Iinclude`; baseline `fgets/strtok/strtod`, running double SMA sums, double prices, whole-file dynamically allocated trade array. |
| `v1_baseline_strtok_running_double_heap_O2` | `build/release/market_baseline.exe` | `-std=c11 -Wall -Wextra -Wpedantic -O2 -DNDEBUG -Iinclude`; same baseline implementation at the actual release optimization level. |
| `v2_helper_sscanf_naive_double_stream_O2` | `build/variants/v2_helper_sscanf_naive_double_stream_O2.exe` | `-O2 -DREADER_KIND=0 -DSMA_KIND=0 -DFIXED_PRICE=0 -DSTORAGE_KIND=0`; supplied `trade_io.c` sscanf getter reader, naive window re-add, plus a running-double crossover shadow for exact signal agreement, double prices, streaming ring. |
| `v3_own_pointer_reader_naive_double_stream_O2` | `build/variants/v3_own_pointer_reader_naive_double_stream_O2.exe` | `-O2 -DREADER_KIND=1 -DSMA_KIND=0 -DFIXED_PRICE=0 -DSTORAGE_KIND=0`; own `read_trade()` pointer-output reader, naive SMA window re-add plus running-double crossover shadow, double prices, streaming ring. |
| `v4_manual_digit_parser_naive_double_stream_O2` | `build/variants/v4_manual_digit_parser_naive_double_stream_O2.exe` | `-O2 -DREADER_KIND=2 -DSMA_KIND=0 -DFIXED_PRICE=0 -DSTORAGE_KIND=0`; manual digit-walk parser, naive SMA window re-add plus running-double crossover shadow, double prices, streaming ring. |
| `v5_manual_digit_parser_running_double_stream_O2` | `build/variants/v5_manual_digit_parser_running_double_stream_O2.exe` | `-O2 -DREADER_KIND=2 -DSMA_KIND=1 -DFIXED_PRICE=0 -DSTORAGE_KIND=0`; manual parser, running double SMA, double prices, streaming ring. |
| `v6_manual_digit_parser_running_fixed_i64_1e8_stream_O2` | `build/variants/v6_manual_digit_parser_running_fixed_i64_1e8_stream_O2.exe` | `-O2 -DREADER_KIND=2 -DSMA_KIND=1 -DFIXED_PRICE=1 -DSTORAGE_KIND=0`; price/SMA ring at scale $10^8$ with checked int64 running sums and a double crossover shadow, double VWAP. |
| `v7_manual_digit_parser_running_double_stream_O3` | `build/variants/v7_manual_digit_parser_running_double_stream_O3.exe` | Same as v5 except `-O3`; isolated compiler optimization comparison. |
| `v8_manual_running_double_static_trade_array_O2` | `build/variants/v8_manual_running_double_static_trade_array_O2.exe` | Same manual parser, running double SMA, double prices, and O2 as v5; `-DSTORAGE_KIND=1 -DFIXED_TRADE_CAPACITY=12000000U` selects a global static `Trade[12000000]`. Runtime `--capacity` may further restrict acceptance. |
| `v9_manual_running_double_exact_malloc_trade_array_O2` | `build/variants/v9_manual_running_double_exact_malloc_trade_array_O2.exe` | Same parser/algorithm/representation/O2 as v5; `-DSTORAGE_KIND=2 -DFIXED_TRADE_CAPACITY=12000000U`; first pass counts valid rows, second allocates exactly `count * sizeof(Trade)` and loads. |
| `v10_manual_running_double_soa_O2` | `build/variants/v10_manual_running_double_soa_O2.exe` | Separate `src/market_soa.c`; same manual digit-walk parser, running double SMA, double price representation, O2 flags, and exact two-pass count/load boundary as v9. `-DSTORAGE_KIND=3` allocates four distinct exact-sized arrays: `int64_t timestamp_us[]`, `double price[]`, `double qty[]`, and `int is_buy[]`; no `Trade` array backs this version. |

All eleven offline versions are registered for the official test20 and development/stress correctness gates. v10's replay target is explicitly compatible with its manual-reader/running-double strategy, but the independent timestamp replay executable uses its streaming storage path; it does not claim offline SoA storage. The v9 versus v10 controlled pair uses matching parser, SMA, price representation, compiler optimization, dataset, and count/load/compute timing boundary. Cache-line locality is a hypothesis, and measured slower results remain in the report. Naive versions retain running-double comparator shadows for equality/crossover consistency; v6 likewise retains a double comparator shadow.

## Isolated assembly kernel (not a v11 market-engine version)

include/aarch64_dotprod.h defines the shared contract for dot_product_u64 and dot_product_ref: naturally aligned uint64_t arrays, size_t count, modulo-2^64 result, and zero for empty input. The assembly uses AAPCS64 argument and return registers and only caller-saved temporaries; it is a leaf and does not adjust the stack.

On Linux AArch64, asm-kernel links asm/aarch64_dotprod.S; elsewhere it links dot_product_u64 to the portable C reference. This kernel is not integrated into frozen v0-v10 or the whole market pipeline. Its benchmark compares C -O3 with the selected kernel using identical runtime-populated arrays, correctness checks before timing, warm-up, alternating order, at least five repeats, and checksums. No speedup is assumed.

## Python and NumPy references (not C version IDs)

These engines are independent Stage A implementations and do not add or modify v0-v10 BuildSpecs, C metadata, or frozen benchmark evidence.

| Engine | Source | Behavior |
|---|---|---|
| Plain Python | `python/market_python.py` | Streaming Binance CSV parser, file-order cumulative VWAP, running-double SMA, C-equivalent warm-up/crossover, buy flag interpretation, and contiguous one-second OHLC bars. JSON mode exposes row-level parity fields. |
| NumPy-assisted batch | `python/market_numpy.py` | Typed NumPy columns, cumulative VWAP and OHLC run reductions in NumPy; running SMA/crossover and row result construction remain Python-level. This is batch/offline processing. Install optional dependency with `requirements-python.txt`. |

`python scripts/project.py compare-python` uses the registered v5 C runner by default, correctness-gates official test20 and the selected dataset, and writes isolated raw/median/table/graph/manifest evidence. Its process-wall boundary includes each interpreter/executable launch. Python streaming parse and compute are interleaved, so those inner phases are intentionally blank; NumPy parse/load and compute are separately timed.
