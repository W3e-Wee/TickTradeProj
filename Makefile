PYTHON ?= python3

.PHONY: all debug release variants soa asm-kernel asm-kernel-test replay replay-test test verify clean clean-build stage0 market_baseline

# The Python driver owns each version BuildSpec, its compiler command, and its metadata.
all:
	$(PYTHON) scripts/project.py build-all

debug:
	$(PYTHON) scripts/project.py build --profile debug

release:
	$(PYTHON) scripts/project.py build --profile release

variants:
	$(PYTHON) scripts/project.py build-variants

soa:
	$(PYTHON) scripts/project.py build-soa

asm-kernel:
	$(PYTHON) scripts/project.py asm-kernel --mode test

asm-kernel-test: asm-kernel

replay:
	$(PYTHON) scripts/project.py build-replay

replay-test:
	$(PYTHON) scripts/project.py replay-test

test:
	$(PYTHON) scripts/project.py test --suite quick

verify: all
	$(PYTHON) scripts/project.py verify --all-versions

stage0:
	$(PYTHON) scripts/project.py build --profile debug

market_baseline:
	$(PYTHON) scripts/project.py build --profile release

clean-build:
	$(PYTHON) scripts/project.py clean-build

clean: clean-build
