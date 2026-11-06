ifeq ($(origin CC),default)
CC = gcc
endif
OPT ?= -O2
GROUP ?= 01
DATA ?= data
INPUT ?= $(DATA)/dev_1M.csv
MEMBER ?= member
PI ?= 0
SUDO ?=
REPEAT ?= 5
MAX_TRADES ?= 10000000
FASTEST ?= v8
PYTHON ?= python
CORE ?= 1
V ?= 0
include config/groups.mk
include config/platform_flags.mk
SHORT ?= $(GROUP_$(GROUP)_SHORT)
LONG ?= $(GROUP_$(GROUP)_LONG)
PLATFORM := windows
EXE := .exe
ifeq ($(OS),)
HOST_UNAME := $(shell uname -s 2>/dev/null)
ifeq ($(HOST_UNAME),Darwin)
PLATFORM := macos
else ifeq ($(HOST_UNAME),Linux)
PLATFORM := linux
else
PLATFORM := posix
endif
EXE :=
endif
BUILD := build/$(PLATFORM)
ifeq ($(OS),Windows_NT)
ifneq ($(MSYSTEM),)
SHELL := sh
else
SHELL := cmd.exe
endif
endif
ifneq ($(MSYSTEM),)
MKDIR_CMD := mkdir -p "$(BUILD)"
RMDIR_CMD := rm -rf build
else ifeq ($(OS),Windows_NT)
MKDIR_CMD := if not exist "$(BUILD)" mkdir "$(BUILD)"
RMDIR_CMD := if exist build rmdir /s /q build
else
MKDIR_CMD := mkdir -p "$(BUILD)"
RMDIR_CMD := rm -rf build
endif
CPPFLAGS := -Isrc/common -Isrc/provided -Isrc/stage0 -Isrc/stageA_plus -Isrc/tools -Isrc/ladder
CFLAGS := -std=c11 $(OPT) -ffp-contract=off -Wall -Wextra -Wpedantic $(STRICT_FLAGS) -MMD -MP
LDFLAGS :=
ifeq ($(OS),Windows_NT)
LDFLAGS += -lpsapi
GNUPLOT_PATH := $(shell where gnuplot 2>NUL)
else
GNUPLOT_PATH := $(shell command -v gnuplot 2>/dev/null)
endif

.DEFAULT_GOAL := all
.PHONY: all help clean distclean test test-unit test-test20 test-consistency verify-golden stage0 stageA stageA_plus ladder stageC tools stretch run-stage0 run-stageA plot-signals plot-rival parser_compare profile-data bench bench-pi perf memory replay-realtime replay-100x replay-1000x race-v0 race-fast rival asm stretch-python plots strict stage0-check parser-compare-test parser-dev-test

all: $(BUILD)/stageA_stream$(EXE) $(BUILD)/platform_check$(EXE) $(BUILD)/parser_compare$(EXE) $(BUILD)/data_profile$(EXE)

ENGINE_COMMON := src/ladder/engine.c src/common/cli.c src/common/platform.c src/common/result.c src/common/fixed_point.c src/common/indicators.c src/common/signal_rule.c src/common/memusage.c src/stageA_plus/reader.c src/stageA_plus/fast_parser.c src/provided/trade_io.c
ENGINE_VERSIONS := v0 v1 v2 v3 v4 v5 v6 v7 v8
ENGINE_BINS := $(foreach version,$(ENGINE_VERSIONS),$(BUILD)/$(version)$(EXE))
all: $(ENGINE_BINS)

-include $(wildcard $(BUILD)/*.d)

$(BUILD):
	$(MKDIR_CMD)

STAGE0_LEVELS := O0 O1 O2 O3
STAGE0_BINARIES := $(foreach level,$(STAGE0_LEVELS),$(BUILD)/stage0_sum_loop_$(level)_plain$(EXE) $(BUILD)/stage0_sum_loop_$(level)_guarded$(EXE) $(BUILD)/stage0_sum_recursion_$(level)_plain$(EXE) $(BUILD)/stage0_sum_recursion_$(level)_guarded$(EXE))
GCC_NO_SIBLING := $(if $(findstring gcc,$(notdir $(firstword $(CC)))),-fno-optimize-sibling-calls,)

define STAGE0_RULES
$(BUILD)/stage0_sum_loop_$(1)_plain$(EXE): src/stage0/stage0_sum_loop.c src/stage0/stage0_math.c src/common/cli.c | $(BUILD)
	$$(CC) $$(CPPFLAGS) -std=c11 -$(1) -ffp-contract=off -Wall -Wextra -Wpedantic $$(STRICT_FLAGS) -MMD -MP -DOPT_LABEL=$(1) -DGUARD_OPTIMISER=0 $$(filter %.c,$$^) -o $$@
$(BUILD)/stage0_sum_loop_$(1)_guarded$(EXE): src/stage0/stage0_sum_loop.c src/stage0/stage0_math.c src/common/cli.c | $(BUILD)
	$$(CC) $$(CPPFLAGS) -std=c11 -$(1) -ffp-contract=off -Wall -Wextra -Wpedantic $$(STRICT_FLAGS) -MMD -MP -DOPT_LABEL=$(1) -DGUARD_OPTIMISER=1 $$(filter %.c,$$^) -o $$@
$(BUILD)/stage0_sum_recursion_$(1)_plain$(EXE): src/stage0/stage0_sum_recursion.c src/stage0/stage0_math.c src/common/cli.c | $(BUILD)
	$$(CC) $$(CPPFLAGS) -std=c11 -$(1) -ffp-contract=off -Wall -Wextra -Wpedantic $$(STRICT_FLAGS) -MMD -MP -DGUARD_OPTIMISER=0 $$(filter %.c,$$^) -o $$@
$(BUILD)/stage0_sum_recursion_$(1)_guarded$(EXE): src/stage0/stage0_sum_recursion.c src/stage0/stage0_math.c src/common/cli.c | $(BUILD)
	$$(CC) $$(CPPFLAGS) -std=c11 -$(1) -ffp-contract=off -Wall -Wextra -Wpedantic $$(STRICT_FLAGS) -MMD -MP $$(GCC_NO_SIBLING) -DGUARD_OPTIMISER=1 $$(filter %.c,$$^) -o $$@
endef
$(foreach level,$(STAGE0_LEVELS),$(eval $(call STAGE0_RULES,$(level))))

stage0: $(STAGE0_BINARIES)
all: $(STAGE0_BINARIES)

$(BUILD)/stageA_stream$(EXE): src/stageA/stageA_stream.c src/common/cli.c src/common/platform.c src/common/result.c src/common/fixed_point.c src/provided/trade_io.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DDEFAULT_SHORT_WINDOW=$(SHORT) -DDEFAULT_LONG_WINDOW=$(LONG) $(filter %.c,$^) $(LDFLAGS) -lm -o $@

$(BUILD)/platform_check$(EXE): src/tools/platform_check.c src/common/platform.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) $(LDFLAGS) -o $@

$(BUILD)/data_profile$(EXE): src/tools/data_profile.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

$(BUILD)/parser_compare$(EXE): src/stageA_plus/parser_compare.c src/stageA_plus/reader.c src/stageA_plus/fast_parser.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) -o $@

help:
	@echo "all: build portable C programs"
	@echo "stage0: build Stage 0 programs"
	@echo "stageA: build Stage A program"
	@echo "stageA_plus: build Stage A+ parser tools"
	@echo "ladder: build engine ladder"
	@echo "stageC: build replay and rival programs"
	@echo "tools: build data analysis tools"
	@echo "stretch: build optional stretch programs"
	@echo "run-stage0: run Stage 0 programs"
	@echo "run-stageA: run Stage A on course files"
	@echo "plot-signals: export and plot Stage A signals"
	@echo "plot-rival: generate and plot the rival reaction sweep"
	@echo "test: run C unit and integration tests"
	@echo "test-unit: run C unit tests"
	@echo "test-test20: compare engines against test20 expected output"
	@echo "verify-golden: check test20 expected output and Stage A"
	@echo "test-consistency: compare engine outputs across datasets"
	@echo "profile-data: profile development and stress files"
	@echo "bench: benchmark the engine ladder"
	@echo "bench-pi: benchmark on a pinned Raspberry Pi core"
	@echo "perf: collect Linux performance counters"
	@echo "memory: report peak engine memory"
	@echo "replay-realtime: replay at real time"
	@echo "replay-100x: replay at 100x"
	@echo "replay-1000x: replay at 1000x"
	@echo "race-v0: run baseline replay for a race"
	@echo "race-fast: run fastest replay for a race"
	@echo "rival: simulate rival fills and plot sweep"
	@echo "asm: generate an assembly listing"
	@echo "stretch-python: run optional C and Python comparison"
	@echo "plots: generate all available plots"
	@echo "strict: rebuild with warnings treated as errors"
	@echo "clean: remove build outputs"
	@echo "distclean: remove build and generated results"
	@echo "stage0-check: run provided Stage 0 helper check"

test: test-unit test-consistency test-test20 parser-compare-test parser-dev-test
test-unit: $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_compat.c -o $(BUILD)/test_compat$(EXE)
	$(BUILD)/test_compat$(EXE)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_exit_codes.c -o $(BUILD)/test_exit_codes$(EXE)
	$(BUILD)/test_exit_codes$(EXE)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_trade.c -o $(BUILD)/test_trade$(EXE)
	$(BUILD)/test_trade$(EXE)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_cli.c src/common/cli.c -o $(BUILD)/test_cli$(EXE)
	$(BUILD)/test_cli$(EXE)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_stage0_math.c src/stage0/stage0_math.c -o $(BUILD)/test_stage0_math$(EXE)
	$(BUILD)/test_stage0_math$(EXE)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_indicators.c src/common/indicators.c src/common/signal_rule.c -o $(BUILD)/test_indicators$(EXE)
	$(BUILD)/test_indicators$(EXE)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_parser.c src/stageA_plus/reader.c src/stageA_plus/fast_parser.c -o $(BUILD)/test_parser$(EXE)
	$(BUILD)/test_parser$(EXE)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_fixed_point.c src/common/fixed_point.c -o $(BUILD)/test_fixed_point$(EXE)
	$(BUILD)/test_fixed_point$(EXE)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_result.c src/common/result.c -o $(BUILD)/test_result$(EXE)
	$(BUILD)/test_result$(EXE)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_bench.c src/common/bench.c -o $(BUILD)/test_bench$(EXE)
	$(BUILD)/test_bench$(EXE)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_latency.c src/common/latency.c -o $(BUILD)/test_latency$(EXE)
	$(BUILD)/test_latency$(EXE)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_replay.c src/common/replay.c -o $(BUILD)/test_replay$(EXE)
	$(BUILD)/test_replay$(EXE)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_rival.c src/common/rival.c -lm -o $(BUILD)/test_rival$(EXE)
	$(BUILD)/test_rival$(EXE)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_signal_display.c src/common/signal_display.c src/common/platform.c -o $(BUILD)/test_signal_display$(EXE) $(LDFLAGS)
	$(BUILD)/test_signal_display$(EXE)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_memusage.c src/common/memusage.c src/common/platform.c -o $(BUILD)/test_memusage$(EXE) $(LDFLAGS)
	$(BUILD)/test_memusage$(EXE)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_platform.c src/common/platform.c -o $(BUILD)/test_platform$(EXE) $(LDFLAGS)
	$(BUILD)/test_platform$(EXE)
$(BUILD)/v0$(EXE): src/ladder/v0_baseline.c $(ENGINE_COMMON) | $(BUILD)
	$(CC) $(CPPFLAGS) $(filter-out $(OPT),$(CFLAGS)) -O0 -DENGINE_VERSION=v0 $(filter %.c,$^) $(LDFLAGS) -lm -o $@
$(BUILD)/v1$(EXE): src/ladder/v0_baseline.c $(ENGINE_COMMON) | $(BUILD)
	$(CC) $(CPPFLAGS) $(filter-out $(OPT),$(CFLAGS)) -O2 -DENGINE_VERSION=v1 $(filter %.c,$^) $(LDFLAGS) -lm -o $@
$(BUILD)/v2$(EXE): src/ladder/v0_baseline.c $(ENGINE_COMMON) | $(BUILD)
	$(CC) $(CPPFLAGS) $(filter-out $(OPT),$(CFLAGS)) -O3 -DENGINE_VERSION=v2 $(filter %.c,$^) $(LDFLAGS) -lm -o $@
$(BUILD)/v3$(EXE): src/ladder/v0_baseline.c $(ENGINE_COMMON) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(PI_CFLAGS) -DENGINE_VERSION=v3 $(filter %.c,$^) $(LDFLAGS) -lm -o $@
$(BUILD)/v4$(EXE): src/ladder/v4_fast_parser.c $(ENGINE_COMMON) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) $(LDFLAGS) -lm -o $@
$(BUILD)/v5$(EXE): src/ladder/v5_running_sum.c $(ENGINE_COMMON) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) $(LDFLAGS) -lm -o $@
$(BUILD)/v6$(EXE): src/ladder/v6_fixed_point.c $(ENGINE_COMMON) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) $(LDFLAGS) -lm -o $@
$(BUILD)/v7$(EXE): src/ladder/v7_in_memory.c $(ENGINE_COMMON) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DENGINE_STATIC_STORAGE -DMAX_TRADES=$(MAX_TRADES) $(filter %.c,$^) $(LDFLAGS) -lm -o $@
$(BUILD)/v8$(EXE): src/ladder/v8_malloc_exact.c $(ENGINE_COMMON) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) $(LDFLAGS) -lm -o $@

ladder: $(ENGINE_BINS)
test-consistency: $(BUILD)/test_consistency$(EXE)
	$(BUILD)/test_consistency$(EXE) --file data/dev_1M.csv --short $(SHORT) --long $(LONG)
	$(BUILD)/test_consistency$(EXE) --file data/stress_full.csv --short $(SHORT) --long $(LONG)
test-test20: $(BUILD)/test_consistency$(EXE)
	$(BUILD)/test_consistency$(EXE) --file data/test20.csv --expected data/test20_expected.txt --short 3 --long 5
verify-golden: test-test20
	$(BUILD)/stageA_stream$(EXE) --file data/test20.csv --short 3 --long 5
$(BUILD)/test_consistency$(EXE): tests/test_consistency.c $(ENGINE_COMMON) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DENGINE_STATIC_STORAGE -DMAX_TRADES=$(MAX_TRADES) $(filter %.c,$^) $(LDFLAGS) -lm -o $@
parser-compare-test: $(BUILD)/parser_compare$(EXE)
	$(BUILD)/parser_compare$(EXE) --file tests/fixtures/trades.csv --repeat 5
parser-dev-test: $(BUILD)/parser_compare$(EXE)
	$(BUILD)/parser_compare$(EXE) --file data/dev_1M.csv --repeat 5
stageA: all
stageA_plus: $(BUILD)/parser_compare$(EXE)
parser_compare: $(BUILD)/parser_compare$(EXE)
stageC: all $(BUILD)/replay_cli$(EXE) $(BUILD)/rival_sweep$(EXE)
stretch: $(BUILD)/v9_soa$(EXE) $(BUILD)/neon_sum$(EXE)
	$(BUILD)/neon_sum$(EXE)
	$(BUILD)/v9_soa$(EXE) --file data/test20.csv --short 3 --long 5
tools: $(BUILD)/data_profile$(EXE)
run-stage0: stage0
	$(BUILD)/stage0_sum_loop_O0_plain$(EXE) --n 1000000 --repeat 5
	$(BUILD)/stage0_sum_loop_O0_guarded$(EXE) --n 1000000 --repeat 5
	$(BUILD)/stage0_sum_recursion_O0_plain$(EXE) --n 100 --reps 10 --repeat 5
	$(BUILD)/stage0_sum_recursion_O0_guarded$(EXE) --n 100 --reps 10 --repeat 5
	$(BUILD)/stage0_sum_loop_O1_plain$(EXE) --n 1000000 --repeat 5
	$(BUILD)/stage0_sum_loop_O1_guarded$(EXE) --n 1000000 --repeat 5
	$(BUILD)/stage0_sum_recursion_O1_plain$(EXE) --n 100 --reps 10 --repeat 5
	$(BUILD)/stage0_sum_recursion_O1_guarded$(EXE) --n 100 --reps 10 --repeat 5
	$(BUILD)/stage0_sum_loop_O2_plain$(EXE) --n 1000000 --repeat 5
	$(BUILD)/stage0_sum_loop_O2_guarded$(EXE) --n 1000000 --repeat 5
	$(BUILD)/stage0_sum_recursion_O2_plain$(EXE) --n 100 --reps 10 --repeat 5
	$(BUILD)/stage0_sum_recursion_O2_guarded$(EXE) --n 100 --reps 10 --repeat 5
	$(BUILD)/stage0_sum_loop_O3_plain$(EXE) --n 1000000 --repeat 5
	$(BUILD)/stage0_sum_loop_O3_guarded$(EXE) --n 1000000 --repeat 5
	$(BUILD)/stage0_sum_recursion_O3_plain$(EXE) --n 100 --reps 10 --repeat 5
	$(BUILD)/stage0_sum_recursion_O3_guarded$(EXE) --n 100 --reps 10 --repeat 5
run-stageA: $(BUILD)/stageA_stream$(EXE)
	$(BUILD)/stageA_stream$(EXE) --file $(DATA)/test20.csv --short 3 --long 5
	$(BUILD)/stageA_stream$(EXE) --file $(DATA)/dev_1M.csv --short $(SHORT) --long $(LONG)
plot-signals: $(BUILD)/stageA_stream$(EXE)
	$(BUILD)/stageA_stream$(EXE) --file $(DATA)/dev_1M.csv --short $(SHORT) --long $(LONG) --export-signals results/plots/signals.dat --slice-len 2000
ifeq ($(strip $(GNUPLOT_PATH)),)
	@echo "SKIPPED: gnuplot not found"
else
	$(GNUPLOT_PATH) scripts/plot_signals.gp
endif
plots: plot-signals plot-rival
profile-data: $(BUILD)/data_profile$(EXE)
	$(BUILD)/data_profile$(EXE) --output results/logs/data_profile.txt $(DATA)/dev_1M.csv $(DATA)/stress_full.csv
bench: ladder
	$(BUILD)/v0$(EXE) --file $(INPUT) --short $(SHORT) --long $(LONG)
	$(BUILD)/v4$(EXE) --file $(INPUT) --short $(SHORT) --long $(LONG)
	$(BUILD)/v5$(EXE) --file $(INPUT) --short $(SHORT) --long $(LONG)
	$(BUILD)/v6$(EXE) --file $(INPUT) --short $(SHORT) --long $(LONG)
	$(BUILD)/v8$(EXE) --file $(INPUT) --short $(SHORT) --long $(LONG)
memory: $(BUILD)/v8$(EXE)
	$(BUILD)/v8$(EXE) --file $(INPUT) --short $(SHORT) --long $(LONG)
bench-pi: ladder
ifeq ($(OS),Windows_NT)
	@echo "NOT VERIFIED: Raspberry Pi benchmarking is unavailable on Windows."
else ifneq ($(shell uname -s),Linux)
	@echo "NOT VERIFIED: Raspberry Pi benchmarking requires Linux and taskset."
else
	$(BUILD)/platform_check$(EXE)
	taskset -c $(CORE) $(MAKE) bench
endif
ifeq ($(OS),Windows_NT)
perf:
	@echo "NOT VERIFIED: Linux perf counters are unavailable on Windows."
else
PERF_PATH := $(shell command -v perf 2>/dev/null)
ifeq ($(strip $(PERF_PATH)),)
perf:
	@echo "NOT VERIFIED: Linux perf is not installed."
else
perf: ladder
	$(PERF_PATH) stat -d $(BUILD)/v8 --file $(INPUT) --short $(SHORT) --long $(LONG)
endif
endif
replay-realtime: $(BUILD)/replay_cli$(EXE)
	$(BUILD)/replay_cli$(EXE) --file $(INPUT) --speed 1
replay-100x: $(BUILD)/replay_cli$(EXE)
	$(BUILD)/replay_cli$(EXE) --file $(INPUT) --speed 100
replay-1000x: $(BUILD)/replay_cli$(EXE)
	$(BUILD)/replay_cli$(EXE) --file $(INPUT) --speed 1000
race-v0: $(BUILD)/v0$(EXE)
	$(BUILD)/v0$(EXE) --file $(INPUT) --short $(SHORT) --long $(LONG)
race-fast: $(BUILD)/v8$(EXE)
	$(BUILD)/v8$(EXE) --file $(INPUT) --short $(SHORT) --long $(LONG)
$(BUILD)/replay_cli$(EXE): src/stageC/replay_cli.c src/stageA_plus/fast_parser.c src/common/latency.c src/common/replay.c src/common/platform.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) $(LDFLAGS) -o $@
$(BUILD)/rival_sweep$(EXE): src/stageC/rival_sweep.c src/stageA_plus/fast_parser.c src/common/rival.c src/common/platform.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) $(LDFLAGS) -lm -o $@
$(BUILD)/v9_soa$(EXE): src/stretch/soa/v9_soa.c src/stageA_plus/fast_parser.c src/common/platform.c src/common/result.c src/common/signal_rule.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) $(LDFLAGS) -lm -o $@
$(BUILD)/neon_sum$(EXE): src/stretch/neon/neon_sum.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(NEON_CFLAGS) $< -o $@
rival: $(BUILD)/rival_sweep$(EXE)
	$(BUILD)/rival_sweep$(EXE) --file $(INPUT) --output results/rival/sweep.csv
plot-rival: rival
ifeq ($(strip $(GNUPLOT_PATH)),)
	@echo "SKIPPED: gnuplot not found"
else
	$(GNUPLOT_PATH) scripts/plot_money_lost.gp
endif
asm: | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -S src/ladder/engine.c -o results/asm/engine.s
stretch-python: $(BUILD)/v8$(EXE)
	$(PYTHON) src/stretch/python/compare.py --file tests/fixtures/trades.csv --c-engine $(BUILD)/v8$(EXE) --short 3 --long 5
strict:
	@$(MAKE) clean
	@$(MAKE) STRICT_FLAGS=-Werror all test
$(BUILD)/stage0_check$(EXE): src/provided/test_helpers.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@
stage0-check: $(BUILD)/stage0_check$(EXE)
	$(BUILD)/stage0_check$(EXE)
clean:
	$(RMDIR_CMD)
distclean: clean
ifeq ($(MSYSTEM),)
ifeq ($(OS),Windows_NT)
	del /q results\logs\data_profile.txt results\bench\parser.csv results\bench\stage0_sum.csv results\bench\stage0_recursion.csv results\plots\signals.dat results\plots\signals.png results\rival\sweep.csv results\asm\engine.s 2>NUL
else
	rm -f results/logs/data_profile.txt results/bench/parser.csv results/bench/stage0_sum.csv results/bench/stage0_recursion.csv results/plots/signals.dat results/plots/signals.png results/rival/sweep.csv results/asm/engine.s
endif
else
	rm -f results/logs/data_profile.txt results/bench/parser.csv results/bench/stage0_sum.csv results/bench/stage0_recursion.csv results/plots/signals.dat results/plots/signals.png results/rival/sweep.csv results/asm/engine.s
endif
