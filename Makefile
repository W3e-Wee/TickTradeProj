ifeq ($(origin CC),default)
CC = gcc
endif
OPT ?= -O2
GROUP ?= 01
DATA ?= data
FILE ?= $(DATA)/stress_full.csv
MEMBER ?= member
SUDO ?=
REPEAT ?= 5
MAX_TRADES ?= 10000000
FASTEST ?= v8
CORE ?=
SPEED ?= 1000
DURATION ?= 600
PIN_CORE ?= 1
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
ifeq ($(MSYSTEM),)
ifeq ($(OS),Windows_NT)
RESULT_MKDIR = if not exist "$@" mkdir "$@"
else
RESULT_MKDIR = mkdir -p "$@"
endif
else
RESULT_MKDIR = mkdir -p "$@"
endif
CPPFLAGS := -Isrc/common -Isrc/provided -Isrc/stage0 -Isrc/stageA_plus -Isrc/tools -Isrc/ladder -Isrc/stageC
CFLAGS := -std=c11 $(OPT) -ffp-contract=off -Wall -Wextra -Wpedantic $(STRICT_FLAGS) -MMD -MP
LDFLAGS :=
ifeq ($(OS),Windows_NT)
LDFLAGS += -lpsapi -lwinmm
GNUPLOT_PATH := $(shell where gnuplot 2>NUL)
else
GNUPLOT_PATH := $(shell command -v gnuplot 2>/dev/null)
endif

.DEFAULT_GOAL := all
.PHONY: all help clean distclean test test-unit test-test20 test-consistency verify-golden test-replay stage0 stageA stageA_plus ladder stageC tools stretch run-stage0 run-stageA plot-signals plot-rival parser_compare profile-data bench bench-pi perf memory replay-realtime replay-100x replay-1000x race-v0 race-fast rival asm plots strict stage0-check parser-compare-test parser-dev-test

all: $(BUILD)/stageA_stream$(EXE) $(BUILD)/platform_check$(EXE) $(BUILD)/parser_compare$(EXE) $(BUILD)/data_profile$(EXE)

ENGINE_COMMON := src/ladder/engine.c src/common/cli.c src/common/platform.c src/common/result.c src/common/fixed_point.c src/common/indicators.c src/common/signal_rule.c src/common/memusage.c src/stageA_plus/reader.c src/stageA_plus/fast_parser.c src/provided/trade_io.c
ENGINE_VERSIONS := v0 v1 v2 v3 v4 v5 v6 v7 v8
ENGINE_BINS := $(foreach version,$(ENGINE_VERSIONS),$(BUILD)/$(version)$(EXE))
REPLAY_COMMON := src/stageC/replay_cli.c src/stageC/replay_engine.c src/common/replay.c src/common/replay_events.c src/common/latency.c src/common/rival.c src/common/signal_display.c $(ENGINE_COMMON)
REPLAY_BINS := $(BUILD)/replay_v0$(EXE) $(BUILD)/replay_fast$(EXE)
all: $(ENGINE_BINS) $(REPLAY_BINS)
ifeq ($(OS),Windows_NT)
COMPILER_TRIPLE := $(shell $(CC) -dumpmachine 2>NUL)
else
COMPILER_TRIPLE := $(shell $(CC) -dumpmachine 2>/dev/null)
endif
ARCH := $(firstword $(subst -, ,$(COMPILER_TRIPLE)))
ASM_FILE := results/asm/$(FASTEST)_$(ARCH).s

-include $(wildcard $(BUILD)/*.d)

$(BUILD):
	$(MKDIR_CMD)

RESULT_DIRS := results/logs results/perf results/plots results/bench results/replay results/rival results/asm
$(RESULT_DIRS):
	$(RESULT_MKDIR)

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
	@echo "stretch: build optional C stretch programs"
	@echo "run-stage0: run Stage 0 programs"
	@echo "run-stageA: run Stage A on course files"
	@echo "plot-signals: export and plot Stage A signals"
	@echo "plot-rival: generate and plot the rival reaction sweep"
	@echo "test: run C unit and integration tests"
	@echo "test-unit: run C unit tests"
	@echo "test-test20: compare engines against test20 expected output"
	@echo "test-replay: check schedule, output, and clean replay exit"
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
	@echo "plots: generate all available plots"
	@echo "strict: rebuild with warnings treated as errors"
	@echo "clean: remove build outputs"
	@echo "distclean: remove build and generated results"
	@echo "stage0-check: run provided Stage 0 helper check"

test: test-unit test-consistency test-test20 parser-compare-test parser-dev-test test-replay
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
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_replay.c src/common/replay.c src/common/platform.c $(LDFLAGS) -o $(BUILD)/test_replay$(EXE)
	$(BUILD)/test_replay$(EXE)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_replay_events.c src/common/replay_events.c -o $(BUILD)/test_replay_events$(EXE)
	$(BUILD)/test_replay_events$(EXE)
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
parser-compare-test: $(BUILD)/parser_compare$(EXE) | results/bench
	$(BUILD)/parser_compare$(EXE) --file tests/fixtures/trades.csv --repeat 5
parser-dev-test: $(BUILD)/parser_compare$(EXE) | results/bench
	$(BUILD)/parser_compare$(EXE) --file data/dev_1M.csv --repeat 5
test-replay: $(REPLAY_BINS) | results/replay
	$(BUILD)/replay_v0$(EXE) --file tests/fixtures/replay.csv --short 10 --long 60 --speed 1000 --member test --no-display
	$(BUILD)/replay_fast$(EXE) --file tests/fixtures/replay.csv --short 10 --long 60 --speed 1000 --member test --no-display
	$(BUILD)/replay_fast$(EXE) --file tests/fixtures/replay.csv --short 10 --long 60 --speed 1 --duration-sec 4 --member pace --no-display --test-pacing
	$(BUILD)/replay_v0$(EXE) --file tests/fixtures/replay.csv --short 10 --long 60 --speed 1 --duration-sec 4 --member stop --no-display --stop-after-ms 1000 --test-stop-success
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_replay_output.c -o $(BUILD)/test_replay_output$(EXE)
	$(BUILD)/test_replay_output$(EXE) results/replay/$(PLATFORM)_test_v0_1000x.csv results/replay/$(PLATFORM)_test_v8_1000x.csv results/replay/$(PLATFORM)_pace_v8_1x.csv results/replay/$(PLATFORM)_stop_v0_1x.csv results/replay/summary_$(PLATFORM)_test.csv results/replay/summary_$(PLATFORM)_pace.csv results/replay/summary_$(PLATFORM)_stop.csv
stageA: all
stageA_plus: $(BUILD)/parser_compare$(EXE)
parser_compare: $(BUILD)/parser_compare$(EXE)
stageC: all $(BUILD)/rival_sweep$(EXE)
stretch: $(BUILD)/v9_soa$(EXE) $(BUILD)/neon_sum$(EXE)
	$(BUILD)/neon_sum$(EXE)
	$(BUILD)/v9_soa$(EXE) --file data/test20.csv --short 3 --long 5
tools: $(BUILD)/data_profile$(EXE)
run-stage0: stage0 | results/bench
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
plot-signals: $(BUILD)/stageA_stream$(EXE) | results/plots
	$(BUILD)/stageA_stream$(EXE) --file $(DATA)/dev_1M.csv --short $(SHORT) --long $(LONG) --export-signals results/plots/signals.dat --slice-len 2000
ifeq ($(strip $(GNUPLOT_PATH)),)
	@echo "SKIPPED: gnuplot not found"
else
	$(GNUPLOT_PATH) scripts/plot_signals.gp
endif
plots: plot-signals plot-rival
profile-data: $(BUILD)/data_profile$(EXE) | results/logs
	$(BUILD)/data_profile$(EXE) --output results/logs/data_profile.txt $(DATA)/dev_1M.csv $(DATA)/stress_full.csv
bench: ladder
	$(BUILD)/v0$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG)
	$(BUILD)/v4$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG)
	$(BUILD)/v5$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG)
	$(BUILD)/v6$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG)
	$(BUILD)/v8$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG)
memory: $(BUILD)/v8$(EXE)
	$(BUILD)/v8$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG)
bench-pi: ladder
ifeq ($(OS),Windows_NT)
	@echo "NOT VERIFIED: Raspberry Pi benchmarking is unavailable on Windows."
else ifneq ($(shell uname -s),Linux)
	@echo "NOT VERIFIED: Raspberry Pi benchmarking requires Linux and taskset."
else
	$(BUILD)/platform_check$(EXE)
	taskset -c $(PIN_CORE) $(MAKE) bench
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
	$(PERF_PATH) stat -d $(BUILD)/v8 --file $(FILE) --short $(SHORT) --long $(LONG)
endif
endif
replay-realtime: $(REPLAY_BINS) | results/replay
ifeq ($(CORE),)
	$(SUDO) $(BUILD)/replay_v0$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --speed 1 --duration-sec $(DURATION) --member $(MEMBER) --out-dir results/replay
	$(SUDO) $(BUILD)/replay_fast$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --speed 1 --duration-sec $(DURATION) --member $(MEMBER) --out-dir results/replay
else ifeq ($(CORE),v0)
	$(SUDO) $(BUILD)/replay_v0$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --speed 1 --duration-sec $(DURATION) --member $(MEMBER) --out-dir results/replay
else ifeq ($(CORE),fast)
	$(SUDO) $(BUILD)/replay_fast$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --speed 1 --duration-sec $(DURATION) --member $(MEMBER) --out-dir results/replay
else
$(error CORE must be empty, v0, or fast)
endif
replay-100x: $(REPLAY_BINS) | results/replay
ifeq ($(CORE),)
	$(SUDO) $(BUILD)/replay_v0$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --speed 100 --member $(MEMBER) --out-dir results/replay
	$(SUDO) $(BUILD)/replay_fast$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --speed 100 --member $(MEMBER) --out-dir results/replay
else ifeq ($(CORE),v0)
	$(SUDO) $(BUILD)/replay_v0$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --speed 100 --member $(MEMBER) --out-dir results/replay
else ifeq ($(CORE),fast)
	$(SUDO) $(BUILD)/replay_fast$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --speed 100 --member $(MEMBER) --out-dir results/replay
else
$(error CORE must be empty, v0, or fast)
endif
replay-1000x: $(REPLAY_BINS) | results/replay
ifeq ($(CORE),)
	$(SUDO) $(BUILD)/replay_v0$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --speed 1000 --member $(MEMBER) --out-dir results/replay
	$(SUDO) $(BUILD)/replay_fast$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --speed 1000 --member $(MEMBER) --out-dir results/replay
else ifeq ($(CORE),v0)
	$(SUDO) $(BUILD)/replay_v0$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --speed 1000 --member $(MEMBER) --out-dir results/replay
else ifeq ($(CORE),fast)
	$(SUDO) $(BUILD)/replay_fast$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --speed 1000 --member $(MEMBER) --out-dir results/replay
else
$(error CORE must be empty, v0, or fast)
endif
race-v0: $(BUILD)/replay_v0$(EXE) | results/replay
	$(SUDO) $(BUILD)/replay_v0$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --speed $(SPEED) --member $(MEMBER) --out-dir results/replay
race-fast: $(BUILD)/replay_fast$(EXE) | results/replay
	$(SUDO) $(BUILD)/replay_fast$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --speed $(SPEED) --member $(MEMBER) --out-dir results/replay
$(BUILD)/replay_v0$(EXE): src/stageC/replay_v0.c $(REPLAY_COMMON) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DREPLAY_FAST_LABEL=\"$(FASTEST)\" $(filter %.c,$^) $(LDFLAGS) -lm -o $@
$(BUILD)/replay_fast$(EXE): src/stageC/replay_fast.c $(REPLAY_COMMON) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DREPLAY_FAST_LABEL=\"$(FASTEST)\" $(filter %.c,$^) $(LDFLAGS) -lm -o $@
$(BUILD)/rival_sweep$(EXE): src/stageC/rival_sweep.c src/stageA_plus/fast_parser.c src/common/rival.c src/common/platform.c src/common/fixed_point.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) $(LDFLAGS) -lm -o $@
$(BUILD)/v9_soa$(EXE): src/stretch/soa/v9_soa.c src/stageA_plus/fast_parser.c src/common/platform.c src/common/result.c src/common/signal_rule.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) $(LDFLAGS) -lm -o $@
$(BUILD)/neon_sum$(EXE): src/stretch/neon/neon_sum.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(NEON_CFLAGS) $< -o $@
rival: $(BUILD)/rival_sweep$(EXE) | results/rival
	$(BUILD)/rival_sweep$(EXE) --file $(FILE) --output results/rival/sweep.csv --member $(MEMBER)
plot-rival: rival | results/plots
ifeq ($(strip $(GNUPLOT_PATH)),)
	@echo "SKIPPED: gnuplot not found"
else
	$(GNUPLOT_PATH) scripts/plot_money_lost.gp
endif
asm: | $(BUILD) results/asm
	$(CC) $(CPPFLAGS) $(CFLAGS) -S src/ladder/engine.c -o $(ASM_FILE)
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
	powershell -NoProfile -Command "Get-ChildItem -LiteralPath results -Recurse -File | Where-Object { $$_.Name -ne '.gitkeep' } | Remove-Item -Force"
else
	find results -type f ! -name .gitkeep -delete
endif
else
	find results -type f ! -name .gitkeep -delete
endif
