# Prerequisites

Verification status for this session: **Windows 64-bit with MSYS2 UCRT64**. The compiler, Make, Python and NumPy version checks were run here; gnuplot is absent. Linux, macOS and Raspberry Pi OS instructions are documented but **NOT VERIFIED** on those platforms.

Stretch-goal mapping: NEON uses `make stretch` and a scalar fallback; Struct-of-Arrays v9 is built and checked by `make stretch` and `make test-consistency`; replay latency p50/p99 is included in Stage C CSV; C-versus-Python is isolated to `make stretch-python` with optional NumPy.

The SoA rung is a C-only addition to the engine ladder. It needs no external package; its prerequisite is the same C11 compiler used for the main build. The percentile-latency output is also part of the C replay binary.

## Linux (Debian/Ubuntu)

**Required**

- Install: `sudo apt update && sudo apt install build-essential`.
- Verify: `gcc --version` and `make --version` print GCC/Clang and GNU Make version banners.
- Needed by: all C build, stage, test and benchmark targets.

**Recommended**

- Install plots, Git and task affinity support: `sudo apt install gnuplot git util-linux`.
- Install performance counters when the distribution provides the matching package: `sudo apt install linux-perf`.
- Install VS Code from Microsoft's Linux package repository, or use its published `.deb` installer; verification is `code --version`.
- Verify: `gnuplot --version`, `git --version`, `taskset --version`, `perf --version` and `code --version`.
- Needed by: `plot-signals`, `plot-rival`, `plot-ladder`, `plots` (gnuplot); `bench-pi` (taskset and Raspberry Pi hardware); `perf` (perf); VS Code tasks are optional shortcuts.

**Stretch-goal only**

- Install Python and NumPy: `sudo apt install python3 python3-numpy`.
- Verify: `python3 --version` and `python3 -c "import numpy; print(numpy.__version__)"`.
- Needed by: `stretch-python`; the pure implementation needs only Python, while the NumPy implementation skips cleanly if NumPy is absent.
- NEON uses compiler intrinsics. On Raspberry Pi OS, the native AArch64 GCC toolchain is supplied by `build-essential`; verify `gcc -dumpmachine` reports an AArch64 target.
- Platform status: **NOT VERIFIED (Linux unavailable)**.

## Windows (MSYS2 UCRT64)

**Required**

- In the MSYS2 UCRT64 terminal install: `pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make`.
- Verify: `gcc --version` and `mingw32-make --version` print version banners. `mingw32-make` also works from cmd or PowerShell when the UCRT64 `bin` directory is on `PATH`.
- MSVC is not supported.
- Needed by: all C build, stage, test and benchmark targets.

**Recommended**

- Install plots and Git: `pacman -S mingw-w64-ucrt-x86_64-gnuplot git`.
- Install VS Code with `winget install Microsoft.VisualStudioCode`; verify with `code --version`.
- Verify: `gnuplot --version`, `git --version`, and `code --version`.
- Needed by: `plot-signals`, `plot-rival`, `plot-ladder`, and `plots` require gnuplot. VS Code is optional for *Terminal > Run Task…*.

**Stretch-goal only**

- Install the optional interpreter and array package: `pacman -S mingw-w64-ucrt-x86_64-python mingw-w64-ucrt-x86_64-python-numpy`.
- Verify: `python --version` and `python -c "import numpy; print(numpy.__version__)"`.
- Needed by: `stretch-python`. NumPy is optional and the normal C build does not use Python.
- NEON is enabled by an AArch64 compiler using intrinsics; the portable C fallback works on Windows.
- Platform status: **VERIFIED (Windows 64-bit)**. Linux, macOS and Pi behavior remains NOT VERIFIED here.

## macOS

**Required**

- Install Command Line Tools: `xcode-select --install`.
- Install GNU Make: `brew install make`.
- Verify: `clang --version` and `gmake --version`. Homebrew installs GNU Make as `gmake`; use it or add Homebrew's `gnubin` directory to `PATH` before using the commands named `make`.
- Needed by: all C build, stage, test and benchmark targets.

**Recommended**

- Install plotting, Git and VS Code: `brew install gnuplot git` and `brew install --cask visual-studio-code`.
- Verify: `gnuplot --version`, `git --version` and `code --version`.
- Needed by: gnuplot is used by `plot-signals`, `plot-rival`, `plot-ladder` and `plots`; VS Code tasks are optional.

**Stretch-goal only**

- Install Python and NumPy: `brew install python numpy`.
- Verify: `python3 --version` and `python3 -c "import numpy; print(numpy.__version__)"`.
- Needed by: `stretch-python` only.
- NEON requires an AArch64 compiler for AArch64 code generation; the portable scalar implementation remains available.
- Platform status: **NOT VERIFIED (macOS unavailable)**.

## Raspberry Pi OS (Pi 5)

**Required**

- Install the native compiler and GNU Make: `sudo apt update && sudo apt install build-essential`.
- Verify: `gcc --version`, `make --version`, and `getconf LONG_BIT` (a 64-bit OS reports `64`).
- Needed by: all C build, stage, test and benchmark targets.

**Recommended**

- Install plotting, Git, CPU affinity and performance counters: `sudo apt install gnuplot git util-linux linux-perf`.
- Verify: `gnuplot --version`, `git --version`, `taskset --version`, and `perf --version`.
- Needed by: plot targets use gnuplot; `bench-pi` uses `taskset` and requires Pi hardware; `perf` uses Linux perf. Use a `performance` CPU governor and record the selected core for controlled measurements.
- Run LED targets with `SUDO=sudo` when the device permissions require elevated access.

**Stretch-goal only**

- Install Python and NumPy: `sudo apt install python3 python3-numpy`.
- Verify: `python3 --version` and `python3 -c "import numpy; print(numpy.__version__)"`.
- Needed by: `stretch-python` only.
- The AArch64 GCC toolchain compiles the NEON intrinsics in `src/stretch/neon/`; verify `gcc -dumpmachine` reports AArch64.
- Platform status: **NOT VERIFIED (Raspberry Pi unavailable)**.

## Feature-to-package matrix

| External command or service | Make targets | Missing-prerequisite output |
| --- | --- | --- |
| GCC or Clang (`CC`) | `all`, stage builds, tests, benchmarks, replay, plots, `asm`, `strict` | Build error; the compiler is required. |
| GNU Make (`make`, `gmake`, `mingw32-make`) | Every target; recursive in `strict` and `bench-pi` | Command unavailable; required. |
| `gnuplot` | `plot-signals`, `plot-rival`, `plot-ladder`, `plots`, the chart step of `bench` | `SKIPPED: gnuplot not found` |
| Python interpreter | `stretch-python` | `SKIPPED: python not found` |
| NumPy | NumPy row in `stretch-python` | `SKIPPED: numpy not installed` |
| Course CSV files | Data-dependent targets; test targets fall back to fixtures when absent | `stretch-python`: `SKIPPED: dataset not found: <path>`; `bench-pi`: `SKIPPED: Raspberry Pi hardware not detected` is separate from datasets. |
| `perf` | `perf` | Windows: `SKIPPED: Linux perf is unavailable on Windows.`; non-Linux: `SKIPPED: Linux perf is unavailable on this platform.`; Linux without perf: `SKIPPED: perf not found` |
| `taskset` and Pi hardware | `bench-pi` | Non-Linux: `SKIPPED: Raspberry Pi benchmarking requires Linux.`; missing taskset: `SKIPPED: taskset not found`; non-Pi Linux: `SKIPPED: Raspberry Pi hardware not detected` |
| `sudo` | Optional prefix supplied through `SUDO` for replay and race targets | No automatic skip; leave `SUDO` empty when elevated access is not needed. |
| `uname`, `where`, `command -v` | Make platform/tool discovery | Make's platform-specific discovery commands. |
| `mkdir`, `rm`, `rmdir`, `find`, PowerShell | Output-directory creation, `clean`, `distclean` | Required by the platform's Make recipes. |

The target-level `SKIPPED:` messages are reserved for unavailable external prerequisites or unsupported platforms. A malformed or unreadable required input remains an error.

## Runtime requirements

- Place the separately distributed `test20.csv`, `dev_1M.csv`, and `stress_full.csv` under `data/`; keep them unchanged. The oracle is `data/test20_expected.txt`.
- v7's static trade storage requires `MAX_TRADES × sizeof(Trade)` bytes, where `MAX_TRADES` is a Make variable and `Trade` is defined in `src/common/trade.h`. v8 uses the detected row count times `sizeof(Trade)` plus loader and indicator storage. SoA v9 allocates parallel timestamp, price and quantity columns.
- A 64-bit operating system is required for the intended coursework datasets and memory configuration.
- The real LED helper may need elevated permissions on Pi; `SUDO=sudo` supplies that prefix.
- No network access is needed at runtime. Install packages and obtain course datasets separately.

## Versions observed in this session

| Tool | Version or status |
| --- | --- |
| GCC | `16.1.0` (MSYS2 UCRT64) |
| GNU Make | `4.4.1` |
| gnuplot | Not installed; plot targets report their documented skip. |
| Python 3 | `3.14.5` via `python3`; `3.14.10` via `python` |
| NumPy | `2.5.4` under `python` |

## Not needed

The project does not require pandas, CMake, Docker, MSVC, or internet access at runtime. It uses no third-party C library.

## Official package references

- MSYS2: [UCRT64 toolchain](https://packages.msys2.org/groups/mingw-w64-ucrt-x86_64-toolchain), [Make](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-make), [gnuplot](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-gnuplot), [Python](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-python), [NumPy](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-python-numpy).
- Homebrew: [Make](https://formulae.brew.sh/formula/make), [gnuplot](https://formulae.brew.sh/formula/gnuplot), [NumPy](https://formulae.brew.sh/formula/numpy).
