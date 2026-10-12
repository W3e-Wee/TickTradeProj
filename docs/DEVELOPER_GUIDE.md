# Developer Guide

## Repository map

| Path | Responsibility |
| --- | --- |
| `src/stage0.c` | Stage 0 arithmetic exercise and recursion guard |
| `src/market_baseline.c` | Frozen baseline reader and market calculations used by v0/v1 |
| `src/market_variant.c` | Registered compile-time v2-v9 reader/SMA/numeric/storage variants |
| `src/market_soa.c` | v10 exact-sized Struct-of-Arrays implementation |
| `src/replay_engine.c`, `src/replay_support.c` | Timestamp-paced replay and reusable scheduling/latency helpers |
| `include/market.h`, `include/replay_support.h` | Market and replay interfaces |
| `include/aarch64_dotprod.h` | Dot-product contract shared by C and assembly |
| `provided/` | Course-supplied trade I/O, timer, LED helpers, test helpers, and assignment PDF; treat supplied files as authoritative dependencies and edit only when course instructions allow it |
| `asm/` | Portable reference, isolated benchmark harness, and hand-written AArch64 kernel |
| `scripts/project.py` | Canonical CLI, build specifications, metadata validation, tests, workflows, and evidence management |
| `scripts/rival.py` | Offline rival simulation and reaction sweep support |
| `python/` | Streaming Python reference, batch NumPy engine, and comparison helper |
| `tests/` | Python unittest modules and replay-support C tests |
| `config/` | Platform flags and safe example configuration |
| `docs/` | Methodology, setup, handover, and course reporting notes |

## Build system

`Makefile` is a thin convenience layer. `scripts/project.py` owns the canonical version BuildSpecs: compiler command, flags, input files, source hashes, version ID, and implementation selection. Prefer the driver for reproducible builds:

```sh
python3 scripts/project.py build --profile debug
python3 scripts/project.py build --profile release
python3 scripts/project.py build-variants
python3 scripts/project.py build-soa
python3 scripts/project.py build-replay
python3 scripts/project.py build-all
```

Build outputs are grouped under `build/debug/`, `build/release/`, `build/variants/`, and `build/replay/`. Each executable has an adjacent `.exe.json` metadata sidecar. It records the exact command, compiler version, version ID/selection, and SHA-256 hashes for all declared source inputs. Do not copy an old binary and label it as a new build. A changed source with old metadata is stale and must not be benchmarked or treated as validated. Rebuild and let the driver check metadata.

`clean-build` only removes the generated `build/` tree. It does not clean results. Do not use generic cleanup commands against `results/`.

## Test system

The driver uses Python's standard `unittest` framework and may compile C helpers as part of tests. Tests are under `tests/`; fixtures, when added, should be small, deterministic, and intentionally checked in. Dataset-gated tests require official test20 and may require dev/stress data. Distinguish skips and missing-data failures from passes.

```sh
python3 scripts/project.py test --suite quick
python3 scripts/project.py test --suite full
python3 scripts/project.py replay-test
python3 scripts/project.py verify --dataset test20 --all-versions
```

Correctness gates come before performance claims. A fast result is irrelevant when parity or oracle checks fail. Keep tests deterministic and avoid tests that depend on live services, wall-clock scheduling, or hardware unless they are clearly marked platform-only.

## Results layout

Result evidence is separated by host under `results/laptop/` and `results/pi/`; replay runs may also be under `results/replays/`. Workflows allocate timestamped unique run directories. Preserve raw input/output, manifests, source/dataset hashes, and error records. Add a compact summary only when it can be traced to preserved raw data. Do not overwrite result folders or treat Windows output as Pi evidence. Do not commit large datasets or raw evidence by accident; review size and course requirements before staging.

## Python driver architecture

`scripts/project.py` defines registered `version_specs()` and `replay_specs()`, builds each target and sidecar, checks freshness, runs correctness gates, dispatches benchmarks/replay/profile/assembly, and creates manifests/reports. Keep parser help, dispatch, tests, and docs aligned when changing a command. Add subprocess arguments as argument lists rather than shell-concatenated strings. Paths and filenames with spaces must remain single arguments. Use temporary directories for tests; do not alter checked-in evidence as a side effect.

## Coding and documentation conventions

- **C:** C11, explicit interfaces in `include/`, warning-clean builds, checked bounds/arithmetic, clear ownership, and no behavior change to frozen versions unless the task explicitly calls for it.
- **Python:** standard-library type hints where useful, small functions with clear side effects, `pathlib` for paths, subprocess argument arrays, and explicit error reporting. NumPy stays optional outside its workflows.
- **Shell:** show the shell in docs; quote paths with spaces; prefer commands that work from repository root. Do not place secrets or private host details in examples.
- **Markdown:** describe actual CLI names and outputs, give runnable commands, identify platform/data prerequisites, and mark inferred/expected behavior separately from measured evidence.
- **Configuration:** keep LF text files. Do not include private credentials in checked-in config; `config/example.env` contains examples only.
- **Commit messages:** imperative, specific, concise (for example, `docs: clarify Pi setup`). Use branches such as `docs/getting-started` or `fix/metadata-validation`.

## Safely adding a version

Do not change the behavior or identity of existing v0-v10 IDs. For a genuinely requested new version, first document its hypothesis and exact dimension, then add a new BuildSpec and (where applicable) a matching replay spec without repurposing an old ID. Include every compilation input in metadata hashing, define correctness/parity gates, add targeted tests, update the version manifest and user/reproduction docs, and check all prior version evidence remains untouched. Benchmark only after correctness passes and use new timestamped output.

## Adding a test or documentation page

Add tests under `tests/` and keep small generated inputs in an intentional fixture directory. Never make tests rewrite evidence. Run the relevant test plus the quick suite; report dataset/hardware skips. For docs, inspect the CLI/config/source first, identify platform and data requirements, use exact quoted paths, avoid private machine details, and add a link from README when the page is a primary guide.

## Validation checklists

Local validation:

```sh
python3 scripts/project.py doctor
python3 scripts/project.py readiness
python3 scripts/project.py test --suite quick
python3 scripts/project.py verify --dataset test20 --all-versions
```

Pi-only validation must be run on Raspberry Pi 5/AArch64: record actual OS, compiler, governor, CPU affinity, and dataset identity; verify replay, LED hardware only if available, native assembly/kernel, perf permissions/counters, and controlled repeated benchmarks. Never fabricate Pi output from laptop results.

Before a pull request:

- [ ] Existing v0-v10 behavior and evidence are unchanged unless explicitly authorized.
- [ ] Build metadata is fresh for changed binaries.
- [ ] Correctness passes before any benchmark is interpreted.
- [ ] Tests and docs are updated and applicable local checks pass.
- [ ] Pi-only checks are identified as pending unless actually run on Pi.
- [ ] Raw evidence and existing result folders are preserved.
- [ ] No secrets, caches, binaries, datasets, unrelated edits, or temporary files are staged.
- [ ] Commit message, branch name, and pull request description are clear.
