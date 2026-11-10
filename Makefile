ifeq ($(origin CC),default)
CC = gcc
endif
OPT ?= -O2
GROUP ?= 05
DATA ?= data
FILE ?= $(DATA)/stress_full.csv
MEMBER ?= member
SUDO ?=
REPEAT ?= 5
MAX_TRADES ?= 10000000
PI ?= 0
PI_CFLAGS ?=
NEON_CFLAGS ?=
PROVIDED_DIR ?= provided
FASTEST ?= v8
CORE ?=
SPEED ?= 1000
DURATION ?= 600
PIN_CORE ?= 1
PYFILE ?= $(DATA)/stress_full.csv
PYLIMIT ?=
PYREPEAT ?= 3
V ?= 0
include config/groups.mk
include config/platform_flags.mk
ifneq ($(GROUP),05)
ifeq ($(strip $(SHORT)),)
$(error Only GROUP=05 (short 30, long 60) is configured. Pass SHORT=<n> LONG=<n> to override.)
endif
ifeq ($(strip $(LONG)),)
$(error Only GROUP=05 (short 30, long 60) is configured. Pass SHORT=<n> LONG=<n> to override.)
endif
endif
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
ifeq ($(PI),1)
ifneq ($(PLATFORM),linux)
$(error PI=1 requires a Linux/AArch64 toolchain)
endif
PI_CFLAGS += -mcpu=cortex-a76
NEON_CFLAGS += -march=armv8-a+simd
endif
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
CPPFLAGS := -I$(PROVIDED_DIR) -Isrc/common -Isrc/stage0 -Isrc/stageA_plus -Isrc/tools -Isrc/ladder -Isrc/stageC -Isrc/stretch/soa
ifeq ($(PLATFORM),linux)
CPPFLAGS += -D_GNU_SOURCE
else ifeq ($(PLATFORM),macos)
CPPFLAGS += -D_POSIX_C_SOURCE=200809L
endif
CFLAGS := -std=c11 $(OPT) -ffp-contract=off -Wall -Wextra -Wpedantic  -MMD -MP
LDFLAGS :=
ifeq ($(OS),Windows_NT)
PYTHON ?= $(if $(shell where python3 2>NUL),python3,$(if $(shell where python 2>NUL),python,))
PYTHON_PATH := $(shell where $(PYTHON) 2>NUL)
LDFLAGS += -lpsapi -lwinmm
GNUPLOT_PATH := $(shell where gnuplot 2>NUL)
else
PYTHON ?= $(shell command -v python3 2>/dev/null || command -v python 2>/dev/null)
PYTHON_PATH := $(shell command -v $(PYTHON) 2>/dev/null)
GNUPLOT_PATH := $(shell command -v gnuplot 2>/dev/null)
endif
ifeq ($(V),1)
$(info TickTrade: CC=$(CC) PLATFORM=$(PLATFORM) CPPFLAGS=$(CPPFLAGS) CFLAGS=$(CFLAGS) LDFLAGS=$(LDFLAGS) PROVIDED_DIR=$(PROVIDED_DIR))
endif

.DEFAULT_GOAL := all
.PHONY: all help clean distclean test test-unit test-test20 test-consistency test-replay stage0 stageA stageA_plus ladder stageC tools stretch stretch-python run-stage0 run-stageA plot-signals plot-rival plot-ladder parser_compare profile-data bench bench-pi perf memory replay-realtime replay-100x replay-1000x race-v0 race-fast rival asm plots stage0-check parser-compare-test parser-dev-test

all: $(BUILD)/stageA_stream$(EXE) $(BUILD)/platform_check$(EXE) $(BUILD)/parser_compare$(EXE) $(BUILD)/data_profile$(EXE)

ENGINE_COMMON := src/ladder/engine.c src/common/cli.c src/common/platform.c src/common/result.c src/common/fixed_point.c src/common/indicators.c src/common/signal_rule.c src/common/memusage.c src/stageA_plus/reader.c src/stageA_plus/fast_parser.c $(PROVIDED_DIR)/trade_io.c
ENGINE_VERSIONS := v0 v1 v2 v3 v4 v5 v6 v7 v8
ENGINE_BINS := $(foreach version,$(ENGINE_VERSIONS),$(BUILD)/$(version)$(EXE))
REPLAY_COMMON := src/stageC/replay_cli.c src/stageC/replay_engine.c src/common/replay.c src/common/replay_events.c src/common/latency.c src/common/rival.c src/common/signal_display.c $(ENGINE_COMMON)
REPLAY_BINS := $(BUILD)/replay_v0$(EXE) $(BUILD)/replay_fast$(EXE)
all: $(ENGINE_BINS) $(REPLAY_BINS) $(BUILD)/rival_sweep$(EXE) $(BUILD)/v9_soa$(EXE) $(BUILD)/neon_sum$(EXE)
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
	$$(CC) $$(CPPFLAGS) -std=c11 -$(1) -ffp-contract=off -Wall -Wextra -Wpedantic  -MMD -MP -DOPT_LABEL=$(1) -DGUARD_OPTIMISER=0 $$(filter %.c,$$^) -o $$@
$(BUILD)/stage0_sum_loop_$(1)_guarded$(EXE): src/stage0/stage0_sum_loop.c src/stage0/stage0_math.c src/common/cli.c | $(BUILD)
	$$(CC) $$(CPPFLAGS) -std=c11 -$(1) -ffp-contract=off -Wall -Wextra -Wpedantic  -MMD -MP -DOPT_LABEL=$(1) -DGUARD_OPTIMISER=1 $$(filter %.c,$$^) -o $$@
$(BUILD)/stage0_sum_recursion_$(1)_plain$(EXE): src/stage0/stage0_sum_recursion.c src/stage0/stage0_math.c src/common/cli.c | $(BUILD)
	$$(CC) $$(CPPFLAGS) -std=c11 -$(1) -ffp-contract=off -Wall -Wextra -Wpedantic  -MMD -MP -DGUARD_OPTIMISER=0 $$(filter %.c,$$^) -o $$@
$(BUILD)/stage0_sum_recursion_$(1)_guarded$(EXE): src/stage0/stage0_sum_recursion.c src/stage0/stage0_math.c src/common/cli.c | $(BUILD)
	$$(CC) $$(CPPFLAGS) -std=c11 -$(1) -ffp-contract=off -Wall -Wextra -Wpedantic  -MMD -MP $$(GCC_NO_SIBLING) -DGUARD_OPTIMISER=1 $$(filter %.c,$$^) -o $$@
endef
$(foreach level,$(STAGE0_LEVELS),$(eval $(call STAGE0_RULES,$(level))))

stage0: $(STAGE0_BINARIES)
all: $(STAGE0_BINARIES)

$(BUILD)/stageA_stream$(EXE): src/stageA/stageA_stream.c src/common/cli.c src/common/platform.c src/common/result.c src/common/fixed_point.c $(PROVIDED_DIR)/trade_io.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DDEFAULT_SHORT_WINDOW=$(SHORT) -DDEFAULT_LONG_WINDOW=$(LONG) $(filter %.c,$^) $(LDFLAGS) -lm -o $@

$(BUILD)/platform_check$(EXE): src/tools/platform_check.c src/common/platform.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) $(LDFLAGS) -o $@

$(BUILD)/data_profile$(EXE): src/tools/data_profile.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

$(BUILD)/parser_compare$(EXE): src/stageA_plus/parser_compare.c src/stageA_plus/reader.c src/stageA_plus/fast_parser.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) -o $@

help:
	@echo "help: list targets and descriptions"
	@echo "all: build portable C programs"
	@echo "stage0: build Stage 0 programs"
	@echo "stageA: build Stage A program"
	@echo "stageA_plus: build Stage A+ parser tools"
	@echo "parser_compare: build parser comparison program"
	@echo "ladder: build engine ladder"
	@echo "stageC: build replay and rival programs"
	@echo "tools: build data analysis tools"
	@echo "stretch: build optional C stretch programs"
	@echo "stretch-python: compare Stage A in C, pure Python, and NumPy"
	@echo "run-stage0: run Stage 0 programs"
	@echo "run-stageA: run Stage A on course files"
	@echo "plot-signals: export and plot Stage A signals"
	@echo "plot-rival: generate and plot the rival reaction sweep"
	@echo "plot-ladder: plot median ladder timings when gnuplot is installed"
	@echo "test: run C unit and integration tests"
	@echo "test-unit: run C unit tests"
	@echo "test-test20: compare engines against test20 expected output"
	@echo "test-replay: check schedule, output, and clean replay exit"
	@echo "test-consistency: compare engine outputs across datasets"
	@echo "parser-compare-test: compare reference and fast parsers on the fixture"
	@echo "parser-dev-test: compare reference and fast parsers on development data"
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
	$(CC) $(CPPFLAGS) $(filter-out $(OPT),$(CFLAGS)) -O0 -DENGINE_VERSION=v0 -DENGINE_BUILD_FLAGS=\"-O0\" $(filter %.c,$^) $(LDFLAGS) -lm -o $@
$(BUILD)/v1$(EXE): src/ladder/v0_baseline.c $(ENGINE_COMMON) | $(BUILD)
	$(CC) $(CPPFLAGS) $(filter-out $(OPT),$(CFLAGS)) -O2 -DENGINE_VERSION=v1 -DENGINE_BUILD_FLAGS=\"-O2\" $(filter %.c,$^) $(LDFLAGS) -lm -o $@
$(BUILD)/v2$(EXE): src/ladder/v0_baseline.c $(ENGINE_COMMON) | $(BUILD)
	$(CC) $(CPPFLAGS) $(filter-out $(OPT),$(CFLAGS)) -O3 -DENGINE_VERSION=v2 -DENGINE_BUILD_FLAGS=\"-O3\" $(filter %.c,$^) $(LDFLAGS) -lm -o $@
$(BUILD)/v3$(EXE): src/ladder/v0_baseline.c $(ENGINE_COMMON) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(PI_CFLAGS) -DENGINE_VERSION=v3 -DENGINE_BUILD_FLAGS=\"-O2+$(if $(filter 1,$(PI)),mcpu-cortex-a76,portable)\" $(filter %.c,$^) $(LDFLAGS) -lm -o $@
$(BUILD)/v4$(EXE): src/ladder/v4_fast_parser.c $(ENGINE_COMMON) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DENGINE_BUILD_FLAGS=\"$(OPT)\" $(filter %.c,$^) $(LDFLAGS) -lm -o $@
$(BUILD)/v5$(EXE): src/ladder/v5_running_sum.c $(ENGINE_COMMON) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DENGINE_BUILD_FLAGS=\"$(OPT)\" $(filter %.c,$^) $(LDFLAGS) -lm -o $@
$(BUILD)/v6$(EXE): src/ladder/v6_fixed_point.c $(ENGINE_COMMON) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DENGINE_BUILD_FLAGS=\"$(OPT)\" $(filter %.c,$^) $(LDFLAGS) -lm -o $@
$(BUILD)/v7$(EXE): src/ladder/v7_in_memory.c $(ENGINE_COMMON) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DENGINE_BUILD_FLAGS=\"$(OPT)\" -DENGINE_STATIC_STORAGE -DMAX_TRADES=$(MAX_TRADES) $(filter %.c,$^) $(LDFLAGS) -lm -o $@
$(BUILD)/v8$(EXE): src/ladder/v8_malloc_exact.c $(ENGINE_COMMON) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DENGINE_BUILD_FLAGS=\"$(OPT)\" $(filter %.c,$^) $(LDFLAGS) -lm -o $@

ladder: $(ENGINE_BINS)
HAS_TEST20 := $(wildcard $(DATA)/test20.csv)
HAS_DEV_DATA := $(wildcard $(DATA)/dev_1M.csv)
HAS_STRESS_DATA := $(wildcard $(DATA)/stress_full.csv)
ifeq ($(HAS_TEST20),)
test-test20: $(BUILD)/test_consistency$(EXE)
	@echo "NOTE: using fixture because $(DATA)/test20.csv is missing."
	$(BUILD)/test_consistency$(EXE) --file tests/fixtures/trades.csv --short 3 --long 5
else
test-test20: $(BUILD)/test_consistency$(EXE)
	$(BUILD)/test_consistency$(EXE) --file $(DATA)/test20.csv --expected $(DATA)/test20_expected.txt --short 3 --long 5
endif
ifeq ($(and $(HAS_DEV_DATA),$(HAS_STRESS_DATA)),)
test-consistency: $(BUILD)/test_consistency$(EXE)
	@echo "NOTE: using fixture because one or more course datasets are missing."
	$(BUILD)/test_consistency$(EXE) --file tests/fixtures/trades.csv --short 3 --long 5
else
test-consistency: $(BUILD)/test_consistency$(EXE)
	$(BUILD)/test_consistency$(EXE) --file $(DATA)/dev_1M.csv --short $(SHORT) --long $(LONG)
	$(BUILD)/test_consistency$(EXE) --file $(DATA)/stress_full.csv --short $(SHORT) --long $(LONG)
endif
$(BUILD)/test_consistency$(EXE): tests/test_consistency.c $(ENGINE_COMMON) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DTICKTRADE_SOA_NO_MAIN -DENGINE_STATIC_STORAGE -DMAX_TRADES=$(MAX_TRADES) $(filter-out src/stretch/soa/v9_soa.c,$(filter %.c,$^)) src/stretch/soa/v9_soa.c $(LDFLAGS) -lm -o $@
parser-compare-test: $(BUILD)/parser_compare$(EXE) | results/bench
	$(BUILD)/parser_compare$(EXE) --file tests/fixtures/trades.csv --repeat 5
ifeq ($(HAS_DEV_DATA),)
parser-dev-test: $(BUILD)/parser_compare$(EXE) | results/bench
	@echo "NOTE: using parser fixture because course dataset is missing."
	$(BUILD)/parser_compare$(EXE) --file tests/fixtures/trades.csv --repeat 5
else
parser-dev-test: $(BUILD)/parser_compare$(EXE) | results/bench
	$(BUILD)/parser_compare$(EXE) --file $(DATA)/dev_1M.csv --repeat 5
endif
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
	$(BUILD)/v9_soa$(EXE) --file $(if $(wildcard $(DATA)/test20.csv),$(DATA)/test20.csv,tests/fixtures/trades.csv) --short 3 --long 5

ifeq ($(strip $(PYTHON_PATH)),)
stretch-python:
	@echo "SKIPPED: python not found"
else ifeq ($(wildcard $(PYFILE)),)
stretch-python:
	@echo "SKIPPED: dataset not found: $(PYFILE)"
else
ifeq ($(strip $(PYLIMIT)),)
PYLIMIT_ARG :=
else
PYLIMIT_ARG := --limit $(PYLIMIT)
endif
stretch-python: $(BUILD)/$(FASTEST)$(EXE) | results/bench
	"$(BUILD)/$(FASTEST)$(EXE)" --file $(PYFILE) --short $(SHORT) --long $(LONG) > "$(BUILD)/python_c_reference.txt"
	$(PYTHON) -B src/stretch/python/stage_a.py --file $(PYFILE) --short $(SHORT) --long $(LONG) --repeat $(PYREPEAT) --compare-c $(BUILD)/python_c_reference.txt --csv results/bench/c_vs_python.csv --member $(MEMBER) --platform $(PLATFORM) $(PYLIMIT_ARG)
	$(PYTHON) -B src/stretch/python/stage_a_numpy.py --file $(PYFILE) --short $(SHORT) --long $(LONG) --repeat $(PYREPEAT) --compare-c $(BUILD)/python_c_reference.txt --csv results/bench/c_vs_python.csv --member $(MEMBER) --platform $(PLATFORM) $(PYLIMIT_ARG)
endif
tools: $(BUILD)/data_profile$(EXE)
run-stage0: stage0 | results/bench
	$(BUILD)/stage0_sum_loop_O0_plain$(EXE) --n 100000000 --repeat $(REPEAT)
	$(BUILD)/stage0_sum_loop_O0_guarded$(EXE) --n 100000000 --repeat $(REPEAT)
	$(BUILD)/stage0_sum_recursion_O0_plain$(EXE) --n 10000 --reps 1 --repeat $(REPEAT)
	$(BUILD)/stage0_sum_recursion_O0_guarded$(EXE) --n 10000 --reps 1 --repeat $(REPEAT)
	$(BUILD)/stage0_sum_loop_O1_plain$(EXE) --n 100000000 --repeat $(REPEAT)
	$(BUILD)/stage0_sum_loop_O1_guarded$(EXE) --n 100000000 --repeat $(REPEAT)
	$(BUILD)/stage0_sum_recursion_O1_plain$(EXE) --n 10000 --reps 1 --repeat $(REPEAT)
	$(BUILD)/stage0_sum_recursion_O1_guarded$(EXE) --n 10000 --reps 1 --repeat $(REPEAT)
	$(BUILD)/stage0_sum_loop_O2_plain$(EXE) --n 100000000 --repeat $(REPEAT)
	$(BUILD)/stage0_sum_loop_O2_guarded$(EXE) --n 100000000 --repeat $(REPEAT)
	$(BUILD)/stage0_sum_recursion_O2_plain$(EXE) --n 10000 --reps 1 --repeat $(REPEAT)
	$(BUILD)/stage0_sum_recursion_O2_guarded$(EXE) --n 10000 --reps 1 --repeat $(REPEAT)
	$(BUILD)/stage0_sum_loop_O3_plain$(EXE) --n 100000000 --repeat $(REPEAT)
	$(BUILD)/stage0_sum_loop_O3_guarded$(EXE) --n 100000000 --repeat $(REPEAT)
	$(BUILD)/stage0_sum_recursion_O3_plain$(EXE) --n 10000 --reps 1 --repeat $(REPEAT)
	$(BUILD)/stage0_sum_recursion_O3_guarded$(EXE) --n 10000 --reps 1 --repeat $(REPEAT)
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
plots: plot-signals plot-rival plot-ladder
profile-data: $(BUILD)/data_profile$(EXE) | results/logs
	$(BUILD)/data_profile$(EXE) --output results/logs/data_profile.txt $(DATA)/dev_1M.csv $(DATA)/stress_full.csv
LADDER_CSV := results/bench/ladder_$(PLATFORM)_$(MEMBER).csv
MEMORY_CSV := results/bench/memory_$(PLATFORM).csv
bench: ladder $(BUILD)/v9_soa$(EXE) | results/bench
	$(BUILD)/v0$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --repeat $(REPEAT) --csv "$(LADDER_CSV)" --member "$(MEMBER)"
	$(BUILD)/v1$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --repeat $(REPEAT) --csv "$(LADDER_CSV)" --member "$(MEMBER)"
	$(BUILD)/v2$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --repeat $(REPEAT) --csv "$(LADDER_CSV)" --member "$(MEMBER)"
	$(BUILD)/v3$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --repeat $(REPEAT) --csv "$(LADDER_CSV)" --member "$(MEMBER)"
	$(BUILD)/v4$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --repeat $(REPEAT) --csv "$(LADDER_CSV)" --member "$(MEMBER)"
	$(BUILD)/v5$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --repeat $(REPEAT) --csv "$(LADDER_CSV)" --member "$(MEMBER)"
	$(BUILD)/v6$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --repeat $(REPEAT) --csv "$(LADDER_CSV)" --member "$(MEMBER)"
	$(BUILD)/v7$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --repeat $(REPEAT) --csv "$(LADDER_CSV)" --member "$(MEMBER)"
	$(BUILD)/v8$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --repeat $(REPEAT) --csv "$(LADDER_CSV)" --member "$(MEMBER)"
	$(BUILD)/v9_soa$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --repeat $(REPEAT) --csv "$(LADDER_CSV)" --member "$(MEMBER)"
	$(MAKE) plot-ladder
plot-ladder: | results/plots
ifeq ($(strip $(GNUPLOT_PATH)),)
	@echo "SKIPPED: gnuplot not found"
else ifeq ($(wildcard $(LADDER_CSV)),)
	@echo "SKIPPED: ladder benchmark CSV not found"
else
	$(GNUPLOT_PATH) -e "input='$(LADDER_CSV)'; output='results/plots/ladder_$(PLATFORM)_$(MEMBER).png'" scripts/plot_ladder.gp
endif
memory: ladder $(BUILD)/v9_soa$(EXE) | results/bench
	$(BUILD)/v0$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --repeat 1 --csv "$(MEMORY_CSV)" --member "$(MEMBER)"
	$(BUILD)/v1$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --repeat 1 --csv "$(MEMORY_CSV)" --member "$(MEMBER)"
	$(BUILD)/v2$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --repeat 1 --csv "$(MEMORY_CSV)" --member "$(MEMBER)"
	$(BUILD)/v3$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --repeat 1 --csv "$(MEMORY_CSV)" --member "$(MEMBER)"
	$(BUILD)/v4$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --repeat 1 --csv "$(MEMORY_CSV)" --member "$(MEMBER)"
	$(BUILD)/v5$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --repeat 1 --csv "$(MEMORY_CSV)" --member "$(MEMBER)"
	$(BUILD)/v6$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --repeat 1 --csv "$(MEMORY_CSV)" --member "$(MEMBER)"
	$(BUILD)/v7$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --repeat 1 --csv "$(MEMORY_CSV)" --member "$(MEMBER)"
	$(BUILD)/v8$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --repeat 1 --csv "$(MEMORY_CSV)" --member "$(MEMBER)"
	$(BUILD)/v9_soa$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --repeat 1 --csv "$(MEMORY_CSV)" --member "$(MEMBER)"
ifeq ($(OS),Windows_NT)
bench-pi:
	@echo "SKIPPED: Raspberry Pi benchmarking requires Linux."
else ifneq ($(HOST_UNAME),Linux)
bench-pi:
	@echo "SKIPPED: Raspberry Pi benchmarking requires Linux."
else ifeq ($(shell command -v taskset 2>/dev/null),)
bench-pi:
	@echo "SKIPPED: taskset not found"
else
bench-pi: $(BUILD)/platform_check$(EXE)
	@if $(BUILD)/platform_check$(EXE) --require-pi; then $(MAKE) clean && taskset -c $(PIN_CORE) $(MAKE) PI=1 bench; else code=$$?; if test $$code -eq 77; then exit 0; else exit $$code; fi; fi
endif
ifeq ($(OS),Windows_NT)
perf:
	@echo "SKIPPED: Linux perf is unavailable on Windows."
else
ifeq ($(PLATFORM),linux)
PERF_PATH := $(shell command -v perf 2>/dev/null)
ifeq ($(strip $(PERF_PATH)),)
perf:
	@echo "SKIPPED: perf not found"
else
perf: ladder
	$(PERF_PATH) stat -d $(BUILD)/v8 --file $(FILE) --short $(SHORT) --long $(LONG)
endif
else
perf:
	@echo "SKIPPED: Linux perf is unavailable on this platform."
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
	$(CC) $(CPPFLAGS) $(CFLAGS) -DDEFAULT_SHORT_WINDOW=$(SHORT) -DDEFAULT_LONG_WINDOW=$(LONG) $(filter %.c,$^) $(LDFLAGS) -lm -o $@
$(BUILD)/v9_soa$(EXE): src/stretch/soa/v9_soa.c src/stageA_plus/fast_parser.c src/common/platform.c src/common/result.c src/common/fixed_point.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DENGINE_BUILD_FLAGS=\"$(OPT)-SoA\" $(filter %.c,$^) $(LDFLAGS) -lm -o $@
$(BUILD)/neon_scalar.o: src/stretch/neon/neon_scalar.c | $(BUILD)
	$(CC) $(CPPFLAGS) -std=c11 -O3 -ffp-contract=off -Wall -Wextra -Wpedantic -MMD -MP -c $< -o $@
$(BUILD)/neon_sum$(EXE): src/stretch/neon/neon_sum.c src/common/platform.c $(BUILD)/neon_scalar.o | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(NEON_CFLAGS) src/stretch/neon/neon_sum.c src/common/platform.c $(BUILD)/neon_scalar.o $(LDFLAGS) -lm -o $@
rival: $(BUILD)/rival_sweep$(EXE) | results/rival
	$(BUILD)/rival_sweep$(EXE) --file $(FILE) --short $(SHORT) --long $(LONG) --output results/rival/sweep.csv --member $(MEMBER)
plot-rival: rival | results/plots
ifeq ($(strip $(GNUPLOT_PATH)),)
	@echo "SKIPPED: gnuplot not found"
else
	$(GNUPLOT_PATH) scripts/plot_money_lost.gp
endif
asm: | $(BUILD) results/asm
	$(CC) $(CPPFLAGS) $(CFLAGS) -S src/ladder/engine.c -o $(ASM_FILE)
$(BUILD)/stage0_check$(EXE): $(PROVIDED_DIR)/test_helpers.c | $(BUILD)
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
