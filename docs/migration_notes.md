# Migration notes

Each bullet records a removed, moved, or rewritten file from the cleanup pass.

## Rewritten files

- Rewrote `.gitattributes` for the specified text and line-ending rules.
- Rewrote `.gitignore` for generated C builds, results, and local datasets.
- Rewrote `Makefile` to build C with `-ffp-contract=off`; legacy Python commands were removed.
- Rewrote `README.md` for the completed newcomer guide and supported commands.
- Rewrote `docs/design.md` with the implemented numerical policy and measured data limits.
- Replaced `config/platform_flags.mk` with portable compiler-flag defaults.
- Rewrote `docs/migration_notes.md` as this per-file cleanup record.
- Removed `.github/workflows/ci.yml` and `.github/`: CI not used

## Moved files

- Moved `test20 - Copy (6).csv` to `data/test20.csv` without changing its bytes.
- Moved `test20_expected - Copy (6).txt` to `data/test20_expected.txt` without changing its bytes.
- Moved `dev_1M - Copy (6).csv` to `data/dev_1M.csv` without changing its bytes.
- Moved `stress_full - Copy (6).csv` to `data/stress_full.csv` without changing its bytes.
- Moved `provided/group_info - Copy (6).txt` to `data/group_info.txt` without changing its bytes.
- Moved `provided/timing.h` to `src/provided/timing.h` without changing its bytes.
- Moved `provided/led.h` to `src/provided/led.h` without changing its bytes.
- Moved `provided/trade_io.h` to `src/provided/trade_io.h` without changing its bytes.
- Moved `provided/trade_io.c` to `src/provided/trade_io.c` without changing its bytes.
- Moved `provided/test_helpers.c` to `src/provided/test_helpers.c` without changing its bytes.
- Moved and rewrote `src/stage0.c` as `src/stage0/stage0_sum_loop.c` to implement the Stage 0 benchmark interface.

## Follow-up implementation

- Added the shared ladder core and v0-v8 front ends, plus a consistency harness that checks test20, development, and stress inputs.
- Added streaming parser comparison, exact scaled crossover checks, fixed-point overflow protection, Stage C replay and rival tools, and the v9 SoA and NEON stretch programs.
- Removed the optional interpreter comparison to keep the implementation C-only.
- Replaced unfinished-target messages in Make and documented measured and platform-dependent behavior.

## Deleted legacy files

- Deleted `.editorconfig`.
- Deleted `LICENSE_NOTICE.md`.
- Deleted `asm/aarch64_dotprod.S`.
- Deleted `asm/aarch64_dotprod_bench.c`.
- Deleted `asm/aarch64_dotprod_ref.c`.
- Deleted `include/aarch64_dotprod.h`.
- Deleted `include/market.h`.
- Deleted `include/replay_support.h`.
- Deleted `provided/CSC1103-1104_Mini_Project_2026-27_Tick-to-Trade_final.pdf`.
- Deleted `config/example.env`.
- Deleted `docs/DEVELOPER_GUIDE.md`.
- Deleted `docs/GETTING_STARTED.md`.
- Deleted `docs/GITHUB_HANDOVER.md`.
- Deleted `docs/REPRODUCIBILITY.md`.
- Deleted `docs/USER_GUIDE.md`.
- Deleted `docs/ai_assistance.md`.
- Deleted `docs/benchmark_methodology.md`.
- Deleted `docs/code_walkthrough.md`.
- Deleted `docs/codebase_map.md`.
- Deleted `docs/demo_script.md`.
- Deleted `docs/evidence_inventory.md`.
- Deleted `docs/git_readiness.md`.
- Deleted `docs/known_limitations.md`.
- Deleted `docs/laptop_report_draft.md`.
- Deleted `docs/member_benchmark_template.md`.
- Deleted `docs/member_contributions_template.md`.
- Deleted `docs/numeric_policy.md`.
- Deleted `docs/performance_methodology.md`.
- Deleted `docs/pi_handoff.md`.
- Deleted `docs/prediction_logbook_template.md`.
- Deleted `docs/pseudocode.md`.
- Deleted `docs/report_evidence_map.md`.
- Deleted `docs/requirements_conflicts.md`.
- Deleted `docs/requirements_matrix.md`.
- Deleted `docs/stage_status.md`.
- Deleted `docs/submission_checklist.md`.
- Deleted `docs/version_manifest.md`.
- Deleted `python/compare_numpy.py`.
- Deleted `python/market_numpy.py`.
- Deleted `python/market_python.py`.
- Deleted `scripts/project.py`.
- Deleted `scripts/rival.py`.
- Deleted `src/market_baseline.c`.
- Deleted `src/market_soa.c`.
- Deleted `src/market_variant.c`.
- Deleted `src/replay_engine.c`.
- Deleted `src/replay_support.c`.
- Deleted `tests/fixtures/invalid.csv`.
- Deleted `tests/test_asm_kernel.py`.
- Deleted `tests/test_market.py`.
- Deleted the legacy performance test.
- Deleted `tests/test_python_engines.py`.
- Deleted `tests/test_readiness.py`.
- Deleted `tests/test_replay.py`.
- Deleted `tests/test_replay_support.c`.
- Deleted `tests/test_rival.py`.
- Deleted `scripts/__pycache__/project.cpython-314.pyc`.
- Deleted `scripts/__pycache__/rival.cpython-314.pyc`.
- Deleted `tests/__pycache__/test_asm_kernel.cpython-314.pyc`.
- Deleted `tests/__pycache__/test_market.cpython-314.pyc`.
- Deleted the compiled legacy performance test bytecode.
- Deleted `tests/__pycache__/test_python_engines.cpython-314.pyc`.
- Deleted `tests/__pycache__/test_readiness.cpython-314.pyc`.
- Deleted `tests/__pycache__/test_replay.cpython-314.pyc`.
- Deleted `tests/__pycache__/test_rival.cpython-314.pyc`.
- Deleted `results/laptop/correctness_matrix.csv`.
- Deleted `results/laptop/assembly-runs/prior-inspection/20261105T095550Z/annotation_template.md`.
- Deleted `results/laptop/assembly-runs/prior-inspection/20261105T095550Z/assembly/v1_baseline_strtok_running_double_heap_O2.s`.
- Deleted `results/laptop/assembly-runs/prior-inspection/20261105T095550Z/assembly_manifest.json`.
- Deleted `results/laptop/assembly-runs/prior-inspection/20261105T095550Z/inspection_command.txt`.
- Deleted `results/laptop/assembly-runs/prior-inspection/20261105T100245Z/annotation_template.md`.
- Deleted `results/laptop/assembly-runs/prior-inspection/20261105T100245Z/assembly/v1_baseline_strtok_running_double_heap_O2.s`.
- Deleted `results/laptop/assembly-runs/prior-inspection/20261105T100245Z/assembly_manifest.json`.
- Deleted `results/laptop/assembly-runs/prior-inspection/20261105T100245Z/inspection_command.txt`.
- Deleted `results/laptop/assembly-runs/prior-inspection/20261106T014248Z/annotation_template.md`.
- Deleted `results/laptop/assembly-runs/prior-inspection/20261106T014248Z/assembly/v1_baseline_strtok_running_double_heap_O2.s`.
- Deleted `results/laptop/assembly-runs/prior-inspection/20261106T014248Z/assembly_manifest.json`.
- Deleted `results/laptop/assembly-runs/prior-inspection/20261106T014248Z/inspection_command.txt`.
- Deleted `results/laptop/assembly-runs/prior-inspection/20261106T015009Z/annotation_template.md`.
- Deleted `results/laptop/assembly-runs/prior-inspection/20261106T015009Z/assembly/v1_baseline_strtok_running_double_heap_O2.s`.
- Deleted `results/laptop/assembly-runs/prior-inspection/20261106T015009Z/assembly_manifest.json`.
- Deleted `results/laptop/assembly-runs/prior-inspection/20261106T015009Z/inspection_command.txt`.
- Deleted the now-empty legacy folders `asm/`, `include/`, `provided/`, `python/`, `results/laptop/`, and both Python cache folders.
