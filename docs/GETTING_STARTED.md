# Getting Started

Run commands from the repository root unless a section names another directory. The project driver discovers CSVs whose names contain `test20`, `dev_1M`, or `stress_full` in the root, `data/`, or `datasets/`. The checked-in official test input currently has the supplied filename `test20 - Copy (6).csv`; its oracle is `test20_expected - Copy (6).txt`. Obtain development/stress files from the course/team using their actual names. Do not rename data merely to make a guessed filename work.

The project uses test20 windows 3/5 and Group 05 dev/stress windows 30/60. These are defaults in the driver; replay and benchmark commands allow window overrides where applicable.

## Windows setup

### 1. Install Git, VS Code, MSYS2 UCRT64, and Python

In **PowerShell**, install Git, Visual Studio Code, MSYS2, and Python 3 with Windows Package Manager:

```powershell
winget install --id Git.Git --exact
winget install --id Microsoft.VisualStudioCode --exact
winget install --id MSYS2.MSYS2 --exact
winget install --id Python.Python.3.12 --exact
```

If a package ID is unavailable in your Windows image, use the vendor installer and select MSYS2's UCRT64 environment. Close and reopen terminals after installation so PATH changes take effect. Python 3.12 is a known supported baseline; newer Python 3 releases may also work, but use the version accepted by your course environment.

### 2. Install GCC, Make, and optional gnuplot

Open **MSYS2 UCRT64** from the Start menu (not MSYS, MinGW 32-bit, or an unrelated shell) and update/install packages:

```bash
pacman -Syu
```

If MSYS2 asks to close the terminal, close it, reopen **MSYS2 UCRT64**, then finish the update and install the toolchain:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make mingw-w64-ucrt-x86_64-python
```

Optional plotting support:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gnuplot
```

Alternatively install gnuplot with its Windows installer. The project creates SVG plots without gnuplot, so this is optional.

### 3. Verify the tools

In **MSYS2 UCRT64**:

```bash
git --version
gcc --version
make --version
python3 --version
```

Optional:

```bash
gnuplot --version
python3 -m pip --version
```

NumPy is optional; install it only for Python/NumPy comparison workflows:

```bash
python3 -m pip install -r requirements-python.txt
```

The Python command for setup/build/test should run in an environment where it can find GCC. Use **MSYS2 UCRT64** for all steps if a separate Windows Python cannot see the UCRT64 compiler. For the IDE, open the cloned folder in VS Code; a terminal configured as MSYS2 UCRT64 is the most reliable.

### 4. Clone the repository

In **PowerShell** or **MSYS2 UCRT64**, replace the URL and folder with the team's actual repository URL and desired local folder:

```powershell
git clone <REPOSITORY_URL>
cd <REPOSITORY_FOLDER>
```

The angle-bracket values are instructions to substitute, not literal shell commands. Open the resulting folder in VS Code if desired:

```powershell
code .
```

### 5. Place course datasets

Place the official test20 CSV and `test20_expected - Copy (6).txt` at the repository root. Place course-provided development and stress files at the root, under `data/`, or under `datasets/`. The actual names are found by substrings `test20`, `dev_1M`, and `stress_full`. For a direct filename containing spaces, quote it, for example:

```powershell
python scripts/project.py verify --dataset "test20"
```

The `--dataset` argument is a search substring and does not require typing the spaced filename. To access a file directly in the executable, pass its path in quotes. Keep large/private CSVs out of commits unless the team/course explicitly decides to include them; the repository intentionally does not ignore all CSVs, so inspect `git status` before staging.

Check dataset/tool discovery:

```powershell
python scripts/project.py doctor
```

### 6. Build, test, verify, and run a small workflow

In **MSYS2 UCRT64** from the repository root:

```bash
python3 scripts/project.py build-all
python3 scripts/project.py test --suite quick
python3 scripts/project.py verify --dataset test20 --all-versions
python3 scripts/project.py replay --version v1_baseline_strtok_running_double_heap_O2 --dataset test20 --speed 1000 --limit 20
```

Build writes generated executables and `.json` metadata under `build/`. Verification needs both test20 files. Replay needs replay targets from `build-all`, writes to a unique results run folder by default, and the `--limit 20` argument caps the input for a short demonstration.

### Common Windows issues

- **`gcc` or `make` missing:** check that the terminal is MSYS2 UCRT64 and run `pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make` there.
- **Wrong shell/path syntax:** Windows PowerShell uses `python`; MSYS2 generally uses `python3`. Do not paste Bash `for` loops into PowerShell or PowerShell commands into MSYS2.
- **Python finds no compiler:** run `python3 scripts/project.py doctor` in the same UCRT64 terminal that reports GCC.
- **Dataset missing:** check the spelling with `Get-ChildItem *.csv` in PowerShell or `ls *.csv` in MSYS2. The finder searches root, `data/`, and `datasets/` and matches the documented name fragments.
- **Filename with spaces:** quote the complete path, such as `"test20 - Copy (6).csv"` in PowerShell or `'test20 - Copy (6).csv'` in Bash.
- **CRLF or executable-script problems:** use the checked-in `.gitattributes`; rerun from the repository root. Native Windows execution does not need shell script executable bits.
- **NumPy import error:** NumPy is optional for C and plain-Python tasks; install requirements in the same Python environment used to run the driver.
- **Slow stress workflow:** start with test20. Stress benchmark timings can take a long time and are not needed for basic setup.

## Raspberry Pi 5 setup

### 1. Connect over SSH

From a laptop terminal, connect using the hostname or address assigned by your team/network:

```sh
ssh <pi-user>@<pi-hostname>
```

Replace both bracketed values with the Pi account and reachable hostname. If name resolution fails, check the hostname with the team and confirm that the Pi and laptop are on the expected network; do not put private addresses or credentials into repository documentation.

### 2. Update and install packages

On **Raspberry Pi OS 64-bit**:

```sh
sudo apt update
sudo apt full-upgrade
sudo apt install git build-essential python3 python3-pip python3-numpy gnuplot-bin linux-perf util-linux
```

`linux-perf` package naming can vary by Raspberry Pi OS/kernel release. If unavailable, search the configured apt repositories for the matching `linux-perf` package; perf is optional for builds and tests but required for hardware counter profiling. `taskset` is usually supplied by `util-linux`.

Verify:

```sh
uname -m
gcc --version
make --version
python3 --version
python3 -c "import numpy; print(numpy.__version__)"
gnuplot --version
perf --version
taskset --version
```

Expected architecture is `aarch64`. NumPy and gnuplot are optional for core build/test; `perf` and `taskset` are needed only for their respective measurement workflows.

### 3. Clone and place datasets

Clone using the actual team URL:

```sh
git clone <REPOSITORY_URL>
cd <REPOSITORY_FOLDER>
```

Place the course files at the repository root, `data/`, or `datasets/`. The driver recognizes filename fragments `test20`, `dev_1M`, and `stress_full`. Quote direct paths with spaces, for example:

```sh
python3 scripts/project.py verify --dataset test20
```

Dataset filenames, windows, and the policy for large files are described in [Dataset placement](#dataset-placement).

### 4. Build, test, and verify

```sh
python3 scripts/project.py doctor
python3 scripts/project.py build-all
python3 scripts/project.py test --suite quick
python3 scripts/project.py verify --dataset test20 --all-versions
python3 scripts/project.py readiness
```

The first full build can take a few minutes. A missing test20 file/oracle blocks official verification; missing dev/stress files affect the related gates and workflows. Read the status labels rather than treating a missing-data skip as a pass.

Expected results: `doctor` prints detected GCC/Python and discovered data; a successful build ends with built-target/metadata validation messages; the test command ends with `OK` (optional NumPy tests may be skipped); official verification reports matching rows/versions. Replay prints its new run directory and a final summary. Exact timings and result-directory timestamps vary by host and run.

### 5. Pi-only workflows and controlled runs

For performance evidence, record the Pi model, OS/kernel, compiler, governor, CPU pinning result, dataset hash, and run directory. The driver records many of these automatically, but inspect the resulting manifest. A five-repeat minimum and medians are expected for benchmark conclusions. Warm the target before repeats and keep terminal/file output outside timed sections.

Inspect the available CPU governor first:

```sh
for f in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do printf '%s=' "$f"; cat "$f"; done
```

If the hardware and OS expose the performance governor, temporarily request it with the system's supported governor tool. On installations with `cpupower`:

```sh
sudo cpupower frequency-set -g performance
```

Record the change and restore the previous governor after measurement. If `cpupower` is not installed or the governor is unavailable, report that condition; do not claim a controlled performance-governor run. Pin a controlled run to CPU 3 when supported:

```sh
python3 scripts/project.py benchmark --dataset stress_full --repeats 5 --platform-label pi-controlled --taskset-cpus 3 --final-controlled
```

`--final-controlled` requires native Pi Linux AArch64, five or more repeats, a performance governor on every CPU, and successfully applied taskset affinity. It records conditions; it does not change the governor. Without these conditions, label the run exploratory.

For real AArch64 assembly and PMU profiling, run `assembly`, `asm-kernel --mode benchmark`, and `profile` on the Pi. For physical LED output, connect supported hardware and use the platform's documented permissions. Some Pi LED backends require root; `sudo` only escalates that workflow. Return to the normal user session after testing. End a maintenance session cleanly with `sudo poweroff`; use `sudo reboot` only when a reboot is needed. Never remove power while the system is writing files.

### Common Pi issues

- **`uname -m` is not `aarch64`:** install/boot the 64-bit Raspberry Pi OS image before collecting AArch64 evidence.
- **`perf` permission denied or no counters:** check kernel perf settings and PMU availability; retain the raw error and report counters as unavailable. Missing counters are not zero.
- **`taskset` unavailable or rejected:** install/verify `util-linux`, use a valid CPU number, and record that pinning was not applied if the probe fails.
- **Governor path missing:** firmware/kernel may not expose cpufreq controls. Record it; do not claim performance mode.
- **LED permission denied:** check the LED driver and device permissions. Use `sudo` only for LED hardware access, then run ordinary workflows without it.
- **SSH host not found:** verify hostname, network, and SSH service with the team; do not guess or publish private network details.
- **Out of memory:** large datasets and array-based versions need substantial memory. Close other workloads, verify available memory, and run smaller correctness checks first.
- **Stale metadata:** rebuild the selected target; build metadata contains source hashes and must match current source.

## Dataset placement

Recognized data are located at the repository root or one level inside `data/` or `datasets/`. The current code searches for these filename fragments:

| Course data | Driver search fragment | Default short/long windows | Notes |
| --- | --- | --- | --- |
| Official small input | `test20` | 3 / 5 | In this checkout: `test20 - Copy (6).csv`; oracle: `test20_expected - Copy (6).txt` |
| Group 05 development input | `dev_1M` | 30 / 60 | Obtain the exact course/team file; current checkout filename is `dev_1M - Copy (6).csv` |
| Group 05 stress input | `stress_full` | 30 / 60 | Obtain the exact course/team file; current checkout filename is `stress_full.csv` |

To see what the driver found:

```sh
python3 scripts/project.py doctor
```

On PowerShell, list candidate root files with:

```powershell
Get-ChildItem *.csv
```

When invoking a program with a direct path containing spaces, quote the entire argument. The `--dataset` option itself takes a substring (`test20`, `dev_1M`, or `stress_full`) and the driver chooses a matching CSV. To pass the supplied test file directly after building, use:

```powershell
.\build\release\market_baseline.exe "test20 - Copy (6).csv" 3 5
```

```bash
./build/release/market_baseline.exe 'test20 - Copy (6).csv' 3 5
```

Keep large or private course datasets local by default; `.gitignore` deliberately does not hide CSVs because the team has not declared datasets excluded. Review `git status` and stage named source/doc paths only. If course rules require dataset submission, use the team's approved evidence/data packaging process.
