# Project progress

## Current removal and regression session

- Date: 2026-11-09; host: Windows 64-bit with MSYS2 UCRT64, GCC and GNU Make.
- Before editing, captured `make help`, dry-run output for all retained phony targets, and an executable baseline matrix in the system temporary directory. Baseline invocations completed with exit status zero; optional tools printed their existing skip messages.
- Removed the saved-reference targets, their reference files and writer path, and the warning-wrapper target. Updated the command documentation and editor task list.
- The same 39 retained Make targets completed before and after with exit status zero. Optional prerequisite skips were unchanged. Output fingerprints, CSV headers, created/updated path manifests, and the 39 dry-run recipes matched; the help output differs only by the removed entries.
- A clean build and the full `make test` suite passed after the changes. `make distclean` preserved all seven results placeholders.

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
