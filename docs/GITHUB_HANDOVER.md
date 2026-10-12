# GitHub Handover

These steps assume the team has created a GitHub repository. Replace bracketed values with the actual URL, folder, branch description, and paths.

## Clone

```sh
git clone <REPOSITORY_URL>
cd <REPOSITORY_FOLDER>
```

`<REPOSITORY_URL>` must be replaced with the team's real GitHub clone URL; `<REPOSITORY_FOLDER>` is the folder created by Git. No repository is created or pushed by this guide.

## Branch workflow

```sh
git switch main
git pull
git switch -c feature/<short-description>
```

Use a short topic branch, for example `docs/windows-setup` or `fix/build-metadata`. If the shared default branch is not `main`, use the branch named by the team.

## Commit workflow

```sh
git status
git diff
git add <specific files>
git commit -m "Meaningful message"
git push -u origin feature/<short-description>
```

Review `git status` before and after staging. Prefer explicit paths such as `git add README.md docs/GETTING_STARTED.md`; avoid `git add .` unless you have inspected every changed/untracked path carefully. Large local datasets and result evidence may appear in status by design. Do not stage them unless the team/course has approved the exact files for repository distribution.

## Before opening a pull request

- [ ] Code builds on the supported local platform.
- [ ] Relevant tests pass; skips and dataset gaps are reported honestly.
- [ ] `readiness` and applicable `verify` checks are reviewed.
- [ ] Documentation matches actual commands and platform behavior.
- [ ] No secrets, passwords, private IPs, personal home paths, or credentials are committed.
- [ ] No build artifacts, Python caches, accidental datasets, or temporary files are committed.
- [ ] No existing result evidence was overwritten or deleted.
- [ ] Only intended files changed; the full `git diff` has been inspected.
- [ ] Screenshots/evidence links point to the intended run and include raw/manifest context.
- [ ] Commit messages and branch name describe the work clearly.

## Review checklist

Review the change for correctness, test coverage, documentation accuracy, evidence impact, Windows/Pi compatibility, and fresh build metadata. Confirm no behavior or IDs in v0-v10 were changed without explicit scope, no result files were accidentally deleted, no secrets or private network details were added, no large files were staged, and no Pi or benchmark measurements were fabricated. Verify that raw evidence remains available for reported summaries.

## Common problems

- **Git identity missing:** configure a display name and the email approved for the group in local Git config, then retry the commit.
- **Wrong branch:** run `git status --short --branch`; switch to your topic branch before editing/committing. Do not force-push shared history.
- **Merge conflict:** run `git status`, resolve each conflict in the affected text file, remove conflict markers, inspect the whole diff, rerun relevant checks, then stage the resolved paths and complete the merge/rebase as Git requests. Do not resolve generated evidence by overwriting it blindly.
- **Compiler missing:** open MSYS2 UCRT64 and install its UCRT64 GCC/Make packages; run `doctor` in that same shell.
- **Wrong shell:** PowerShell uses `python`; MSYS2/Linux uses `python3`. Use commands marked for the correct shell in Getting Started.
- **Dataset missing:** put exact course files at root, `data/`, or `datasets/`; run `doctor`; check names contain `test20`, `dev_1M`, or `stress_full` as appropriate.
- **Stale build metadata:** rebuild selected targets with `build-all` or the relevant build command. Do not benchmark stale binaries.
- **Windows CRLF issue:** respect `.gitattributes` and `.editorconfig`; avoid mass-converting binary/evidence files. Restore text files through the editor's explicit line-ending control if needed.
- **Python module not found:** activate/install into the interpreter used by `python3`; the core CLI uses only the standard library, while test modules should run through the project driver's test command.
- **NumPy unavailable:** install `requirements-python.txt` in the same interpreter. C and plain Python workflows do not require NumPy.
- **Pi LED permission:** verify the Pi LED backend and device permissions; use elevated privileges only when required for the hardware operation. Never infer hardware output from a laptop.
- **SSH hostname issue:** confirm hostname/network with the team and verify SSH is enabled. Keep private host/IP details out of commits.

## Repository hygiene

`.gitignore` excludes generated build directories, Python caches, editor/OS temporary files, local secrets, and temporary archives. It intentionally does not blanket-ignore CSV datasets or `results/`, so required fixtures/manifests/evidence are not hidden. Check file sizes and ownership before adding any data or results. Stage only approved, specific paths.
