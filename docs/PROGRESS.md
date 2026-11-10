# Project progress

## Signal plot repair

- Audited the existing Stage A export and gnuplot recipe before editing. The old chart had no signal markers, used the default terminal size, and did not capture or validate gnuplot diagnostics.
- Added the interpolated SMA crossing export, exact-signal self-check, first qualifying complete-slice selection, and a shorter-file path for fixtures. Added C checks for fixture/development export geometry and PNG/summary validation.
- Rewrote the fixed-path gnuplot script with terminal fallback, labeled axes, data-derived y-range, distinct SMA styling, crossing markers, and the five-line summary. `plot-signals` now captures stderr and runs the plot validator.
- Linux x86_64 checks run: clean build, full test suite, Stage A fixture and development export checks, gnuplot rendering, PNG dimensions/signature/size, summary counts and range, and visual inspection. All requested plot checks pass; Windows, macOS and Raspberry Pi runs remain unverified.

## Plot portability extension

- Initial audit found the rival plot failing on the headered CSV, the ladder chart sending binary PNG data to the terminal, gnuplot `-e` path arguments, no shared style or plot registry, and text-mode ladder CSV output.
- Added `scripts/common.gp`, registry-driven fixed-path scripts, `plot-money-lost`, runtime Make input/tool checks, captured gnuplot logs, C validators and script-policy checks. Ladder plotting normalizes the existing platform/member CSV into an LF-only fixed input using the latest row per rung; the original benchmark CSV schema and path remain unchanged.
- Plot checker and script scanner pass. `make clean && make && make test` passes on Linux x86_64, and `make plots` generates and validates all three PNGs. All plotted data files have LF line endings; gnuplot logs are empty. The no-gnuplot path skips rendering and passes data validation. macOS, Windows and Raspberry Pi remain unverified for this extension.

## Current removal and regression session

- Date: 2026-11-09; host: Windows 64-bit with MSYS2 UCRT64, GCC and GNU Make.
- Before editing, captured `make help`, dry-run output for all retained phony targets, and an executable baseline matrix in the system temporary directory. Baseline invocations completed with exit status zero; optional tools printed their existing skip messages.
- Removed the saved-reference targets, their reference files and writer path, and the warning-wrapper target. Updated the command documentation and editor task list.
- The same 39 retained Make targets completed before and after with exit status zero. Optional prerequisite skips were unchanged. Output fingerprints, CSV headers, created/updated path manifests, and the 39 dry-run recipes matched; the help output differs only by the removed entries.
- A clean build and the full `make test` suite passed after the changes. `make distclean` preserved all seven results placeholders.

## Group 05 configuration follow-up

- Before edits, `config/groups.mk` had 21 group entries and Make defaulted to 10/60. `data/group_info.txt` already specifies Group 05 at 30/60 and test20 at 3/5; the course data file was left unchanged.
- Baseline `make clean && make && make test` passed using the old 10/60 default. At 30/60, v0, Stage A and v8 independently produced the same dev_1M counts and signal hash. At 10/60, v0 and Stage A agreed on a different signal hash.
- Group 05 configuration, C and Python CLI defaults, rival sweep windows, and documentation have been updated. The clean build and full test suite pass at 30/60; the course test20 oracle remains at 3/5. Default Stage A and explicit 10/60 override runs pass, and v0, Stage A and v8 agree on the dev_1M result at 30/60.
- Fixture-backed ladder, memory, rival, replay, race and Python stretch runs pass with the configured windows. `make help` lists the retained targets; dry runs were captured for all 39 targets. The command table covers 39/39 targets, editor tasks parse and map to existing targets, and all relative documentation links resolve.
- Optional prerequisites absent on this Windows host: gnuplot, the Make-selected `python3` NumPy module, Linux `perf`, and Raspberry Pi/Linux support. Their targets report the corresponding prerequisite skips. Linux, macOS and Pi remain unverified.
- The Group 05 follow-up is complete after the final clean build/test gate and placeholder check.

## Verified project state before this cleanup

- The earlier Windows session passed the clean build, full test suite, course expected-output comparison, Stage 0/A/A+ checks, ladder consistency on development and stress files, Stage C replay/race/rival checks, and host assembly generation.
- Pure Python passed its fixture comparison. NumPy and gnuplot were unavailable; the related optional paths reported their prerequisite skips. Linux, macOS and Raspberry Pi/AArch64 remain unverified.
- The NEON stretch uses compiler intrinsics with a scalar fallback; a hand-written assembly kernel remains a separate stretch enhancement.

## Cleanup milestones

| Milestone | Status | Notes |
| --- | --- | --- |
| Baseline capture | DONE | Saved outside the repository: target help, per-target dry runs, per-target exit status and output, changed build/results paths, CSV headers, deterministic result fields. |
| Remove saved reference files and their writer path | DONE | The C consistency harness remains for regular course-oracle and cross-engine checks. |
| Remove build-warning wrapper | DONE | Normal compiler flags and remaining target behavior are preserved. |
| Documentation and editor task update | DONE | Updated command count, source inventory, output map, runbook and cross-machine comparison guidance. |
| Post-change regression | DONE | All 39 retained targets passed; output fingerprints, CSV headers, created/updated path manifests and dry-run recipes matched the baseline. |

## Required final checks

| Check | Status |
| --- | --- |
| Retained Make target count and help coverage | PASS — 39 targets documented |
| Python inventory remains exactly two files | PASS |
| No saved-reference or removed-target references outside the migration record | PASS |
| All relative documentation links resolve | PASS |
| Full clean build and `make test` pass | PASS |
| Seven results placeholders survive `make distclean` | PASS |
