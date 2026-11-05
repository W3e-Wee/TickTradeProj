#!/usr/bin/env python3
"""Cross-platform project driver for the tick-to-trade project."""

from __future__ import annotations

import argparse
import csv
import contextlib
import hashlib
import io
import json
import math
import os
import platform
import queue
import re
import shutil
import subprocess
import sys
import threading
import tempfile
import time
import html
from pathlib import Path
from statistics import median
from decimal import Decimal, InvalidOperation

from rival import (REACTION_SWEEP_US, Signal, compact_simulate, compact_sweep,
                   decimal_text, read_prices, read_replay_signals, simulate,
                   stream_replay_pair, sweep as reaction_sweep, write_compact_decisions,
                   write_sweep_svg)

PROJECT_SOURCE_ROOT = Path(__file__).resolve().parents[1]
# ROOT is the active project/data root. Tests may redirect it to a temporary
# workspace; source configuration must still come from this checked-out tree.
ROOT = PROJECT_SOURCE_ROOT
BUILD_ROOT = ROOT / 'build'
RESULT_ROOT = ROOT / 'results' / 'laptop'
FROZEN_V0 = 'v0_baseline_strtok_running_double_heap_O0'
CORRECTNESS_MATRIX = RESULT_ROOT / 'correctness_matrix.csv'
DATASET_DIRS = [ROOT, ROOT / 'data', ROOT / 'datasets']
DEFAULT_WINDOWS_GNUPLOT = Path(r'C:\Program Files\gnuplot\bin\gnuplot.exe')
DEFAULT_WINDOWS_MAKE = Path(r'C:\msys64\ucrt64\bin\make.exe')


def platform_build_flags():
    """Compiler definitions needed for APIs exposed by the current platform."""
    if not sys.platform.startswith('linux'):
        return []
    config_path = PROJECT_SOURCE_ROOT / 'config' / 'platform_flags.mk'
    for line in config_path.read_text(encoding='utf-8').splitlines():
        match = re.match(r'^\s*POSIX_API_CFLAGS\s*:=\s*(.*?)\s*$', line)
        if match:
            return match.group(1).split()
    raise RuntimeError(f'Missing POSIX_API_CFLAGS in {config_path}')


def profile_build_flags(profile: str) -> list[str]:
    common = ['-std=c11', '-Wall', '-Wextra', '-Wpedantic']
    if profile == 'debug':
        return common + ['-g3', '-O0', '-Iinclude', '-Iprovided'] + platform_build_flags()
    if profile == 'release':
        return common + ['-O2', '-DNDEBUG', '-Iinclude', '-Iprovided'] + platform_build_flags()
    raise ValueError('profile must be debug or release')


def version_specs():
    baseline_selection = (
        'reader=market_baseline fgets/strtok/strtod; SMA=running double sums; '
        'price=double; storage=whole-file dynamically allocated Trade array'
    )
    baseline_debug_flags = profile_build_flags('debug')
    baseline_release_flags = profile_build_flags('release')
    variant_common = ['-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-DNDEBUG', '-Iinclude', '-Iprovided'] + platform_build_flags()
    specs = [
        {
            'id': 'v0_baseline_strtok_running_double_heap_O0',
            'exe': BUILD_ROOT / 'debug' / 'market_baseline.exe',
            'metadata': BUILD_ROOT / 'debug' / 'market_baseline.exe.json',
            'source': 'src/market_baseline.c',
            'profile': 'debug',
            'flags': baseline_debug_flags,
            'compile_sources': ['src/market_baseline.c'],
            'input_files': ['src/market_baseline.c', 'include/market.h', 'provided/timing.h'],
            'selection': baseline_selection,
            'implemented': True,
        },
        {
            'id': 'v1_baseline_strtok_running_double_heap_O2',
            'exe': BUILD_ROOT / 'release' / 'market_baseline.exe',
            'metadata': BUILD_ROOT / 'release' / 'market_baseline.exe.json',
            'source': 'src/market_baseline.c',
            'profile': 'release',
            'flags': baseline_release_flags,
            'compile_sources': ['src/market_baseline.c'],
            'input_files': ['src/market_baseline.c', 'include/market.h', 'provided/timing.h'],
            'selection': baseline_selection,
            'implemented': True,
        },
        {
            'id': 'v3_own_pointer_reader_naive_double_stream_O2',
            'exe': BUILD_ROOT / 'variants' / 'v3_own_pointer_reader_naive_double_stream_O2.exe',
            'metadata': BUILD_ROOT / 'variants' / 'v3_own_pointer_reader_naive_double_stream_O2.exe.json',
            'source': 'src/market_variant.c',
            'flags': variant_common + ['-O2', '-DREADER_KIND=1', '-DSMA_KIND=0', '-DFIXED_PRICE=0', '-DSTORAGE_KIND=0'],
            'selection': 'reader=own fgets read_trade() with pointer outputs and strtod; SMA=naive window re-add plus running-double crossover shadow; price=double; storage=stream plus circular double ring',
            'implemented': True,
        },
        {
            'id': 'v2_helper_sscanf_naive_double_stream_O2',
            'exe': BUILD_ROOT / 'variants' / 'v2_helper_sscanf_naive_double_stream_O2.exe',
            'metadata': BUILD_ROOT / 'variants' / 'v2_helper_sscanf_naive_double_stream_O2.exe.json',
            'source': 'src/market_variant.c',
            'compile_sources': ['src/market_variant.c', 'provided/trade_io.c'],
            'flags': variant_common + ['-O2', '-DREADER_KIND=0', '-DSMA_KIND=0', '-DFIXED_PRICE=0', '-DSTORAGE_KIND=0'],
            'selection': 'reader=supplied trade_io sscanf getter API; SMA=naive window re-add plus running-double crossover shadow; price=double; storage=stream plus circular double ring',
            'implemented': True,
        },
        {
            'id': 'v4_manual_digit_parser_naive_double_stream_O2',
            'exe': BUILD_ROOT / 'variants' / 'v4_manual_digit_parser_naive_double_stream_O2.exe',
            'metadata': BUILD_ROOT / 'variants' / 'v4_manual_digit_parser_naive_double_stream_O2.exe.json',
            'source': 'src/market_variant.c',
            'flags': variant_common + ['-O2', '-DREADER_KIND=2', '-DSMA_KIND=0', '-DFIXED_PRICE=0', '-DSTORAGE_KIND=0'],
            'selection': 'reader=manual digit-walk CSV parser; SMA=naive window re-add plus running-double crossover shadow; price=double; storage=stream plus circular double ring',
            'implemented': True,
        },
        {
            'id': 'v7_manual_digit_parser_running_double_stream_O3',
            'exe': BUILD_ROOT / 'variants' / 'v7_manual_digit_parser_running_double_stream_O3.exe',
            'metadata': BUILD_ROOT / 'variants' / 'v7_manual_digit_parser_running_double_stream_O3.exe.json',
            'source': 'src/market_variant.c',
            'flags': variant_common + ['-O3', '-DREADER_KIND=2', '-DSMA_KIND=1', '-DFIXED_PRICE=0', '-DSTORAGE_KIND=0', '-DFIXED_TRADE_CAPACITY=12000000U'],
            'selection': 'reader=manual digit-walk CSV parser; SMA=running double sums; price=double; storage=stream plus circular double ring',
            'implemented': True,
        },
        {
            'id': 'v5_manual_digit_parser_running_double_stream_O2',
            'exe': BUILD_ROOT / 'variants' / 'v5_manual_digit_parser_running_double_stream_O2.exe',
            'metadata': BUILD_ROOT / 'variants' / 'v5_manual_digit_parser_running_double_stream_O2.exe.json',
            'source': 'src/market_variant.c',
            'flags': variant_common + ['-O2', '-DREADER_KIND=2', '-DSMA_KIND=1', '-DFIXED_PRICE=0', '-DSTORAGE_KIND=0', '-DFIXED_TRADE_CAPACITY=12000000U'],
            'selection': 'reader=manual digit-walk CSV parser; SMA=running double sums; price=double; storage=stream plus circular double ring',
            'implemented': True,
        },
        {
            'id': 'v8_manual_running_double_static_trade_array_O2',
            'exe': BUILD_ROOT / 'variants' / 'v8_manual_running_double_static_trade_array_O2.exe',
            'metadata': BUILD_ROOT / 'variants' / 'v8_manual_running_double_static_trade_array_O2.exe.json',
            'source': 'src/market_variant.c',
            'flags': variant_common + ['-O2', '-DREADER_KIND=2', '-DSMA_KIND=1', '-DFIXED_PRICE=0', '-DSTORAGE_KIND=1', '-DFIXED_TRADE_CAPACITY=12000000U'],
            'selection': 'reader=manual digit-walk CSV parser; SMA=running double sums; price=double; storage=global static Trade[12000000] with runtime capacity limit',
            'implemented': True,
        },
        {
            'id': 'v9_manual_running_double_exact_malloc_trade_array_O2',
            'exe': BUILD_ROOT / 'variants' / 'v9_manual_running_double_exact_malloc_trade_array_O2.exe',
            'metadata': BUILD_ROOT / 'variants' / 'v9_manual_running_double_exact_malloc_trade_array_O2.exe.json',
            'source': 'src/market_variant.c',
            'flags': variant_common + ['-O2', '-DREADER_KIND=2', '-DSMA_KIND=1', '-DFIXED_PRICE=0', '-DSTORAGE_KIND=2', '-DFIXED_TRADE_CAPACITY=12000000U'],
            'selection': 'reader=manual digit-walk CSV parser; SMA=running double sums; price=double; storage=two-pass valid-count then exact malloc Trade[] AoS',
            'implemented': True,
        },
        {
            'id': 'v6_manual_digit_parser_running_fixed_i64_1e8_stream_O2',
            'exe': BUILD_ROOT / 'variants' / 'v6_manual_digit_parser_running_fixed_i64_1e8_stream_O2.exe',
            'metadata': BUILD_ROOT / 'variants' / 'v6_manual_digit_parser_running_fixed_i64_1e8_stream_O2.exe.json',
            'source': 'src/market_variant.c',
            'flags': variant_common + ['-O2', '-DREADER_KIND=2', '-DSMA_KIND=1', '-DFIXED_PRICE=1', '-DSTORAGE_KIND=0'],
            'selection': 'reader=manual digit-walk CSV parser; SMA=running int64 sum over price x 1e8 with running-double crossover shadow; price=checked signed int64 fixed-point for SMA/OHLC, double VWAP accumulators; storage=stream plus circular int64 ring',
            'implemented': True,
        },
        {
            'id': 'v10_manual_running_double_soa_O2',
            'exe': BUILD_ROOT / 'variants' / 'v10_manual_running_double_soa_O2.exe',
            'metadata': BUILD_ROOT / 'variants' / 'v10_manual_running_double_soa_O2.exe.json',
            'source': 'src/market_soa.c',
            'flags': variant_common + ['-O2', '-DREADER_KIND=2', '-DSMA_KIND=1', '-DFIXED_PRICE=0', '-DSTORAGE_KIND=3', '-DFIXED_TRADE_CAPACITY=12000000U'],
            'selection': 'reader=manual digit-walk CSV parser; SMA=running double sums; price=double; storage=exact two-pass malloc Struct-of-Arrays with separate int64 timestamp, double price, double quantity, and int buy/sell arrays',
            'implemented': True,
            'replay_compatible': True,
            'replay_storage_note': 'replay-compatible; replay uses its registered streaming reader/storage backend, and the offline SoA arrays are not used by the replay executable',
        },
    ]
    for spec in specs[2:]:
        spec.setdefault('profile', 'release')
        spec.setdefault('compile_sources', [spec['source']])
        dependencies = ['include/market.h', 'provided/timing.h', 'provided/trade_io.h']
        spec.setdefault('input_files', list(dict.fromkeys(spec['compile_sources'] + dependencies)))
    return specs


def replay_specs():
    variant_common = ['-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-Iinclude', '-Iprovided']
    source_inputs = ['src/replay_engine.c', 'src/replay_support.c', 'include/replay_support.h',
                     'include/market.h', 'provided/timing.h', 'provided/led.h']
    settings = {
        'v0_baseline_strtok_running_double_heap_O0': ('-O0', '-g3', 3, 1, 0, 3, []),
        'v1_baseline_strtok_running_double_heap_O2': ('-O2', '-DNDEBUG', 3, 1, 0, 3, []),
        'v2_helper_sscanf_naive_double_stream_O2': ('-O2', None, 0, 0, 0, 0, ['provided/trade_io.c']),
        'v3_own_pointer_reader_naive_double_stream_O2': ('-O2', None, 1, 0, 0, 0, []),
        'v4_manual_digit_parser_naive_double_stream_O2': ('-O2', None, 2, 0, 0, 0, []),
        'v5_manual_digit_parser_running_double_stream_O2': ('-O2', None, 2, 1, 0, 0, []),
        'v6_manual_digit_parser_running_fixed_i64_1e8_stream_O2': ('-O2', None, 2, 1, 1, 0, []),
        'v7_manual_digit_parser_running_double_stream_O3': ('-O3', None, 2, 1, 0, 0, []),
        'v8_manual_running_double_static_trade_array_O2': ('-O2', None, 2, 1, 0, 1, []),
        'v9_manual_running_double_exact_malloc_trade_array_O2': ('-O2', None, 2, 1, 0, 2, []),
        # Replay accepts the v10 version ID and same parser/SMA/numeric rules.
        # The timestamp-paced replay keeps its independent streaming backend;
        # this is intentionally explicit and does not claim replay SoA storage.
        'v10_manual_running_double_soa_O2': ('-O2', None, 2, 1, 0, 0, []),
    }
    specs = []
    for version in version_specs():
        optimization, extra, reader, sma, fixed, storage, extras = settings[version['id']]
        flags = list(variant_common) + platform_build_flags()
        if version['id'] != 'v0_baseline_strtok_running_double_heap_O0' and extra != '-DNDEBUG':
            flags.append('-DNDEBUG')
        if extra is not None:
            flags.append(extra)
        flags.extend([f'-DREADER_KIND={reader}', f'-DSMA_KIND={sma}',
                      f'-DFIXED_PRICE={fixed}', f'-DSTORAGE_KIND={storage}'])
        if storage in (1, 2, 3):
            flags.append('-DFIXED_TRADE_CAPACITY=12000000U')
        compile_sources = ['src/replay_engine.c', 'src/replay_support.c'] + extras
        input_files = source_inputs + compile_sources
        replay_selection = version['selection'] + '; replay policy defaults to primary decisions; legacy-hybrid-shadow is explicit'
        if version.get('replay_storage_note'):
            replay_selection = version['replay_storage_note'] + '; running-double strategy; primary replay decisions'
        specs.append({
            'id': version['id'],
            'source': 'src/replay_engine.c',
            'exe': BUILD_ROOT / 'replay' / f"{version['id']}.exe",
            'metadata': BUILD_ROOT / 'replay' / f"{version['id']}.exe.json",
            'flags': flags,
            'compile_sources': compile_sources,
            'input_files': list(dict.fromkeys(input_files)),
            'profile': 'release',
            'selection': replay_selection,
        })
    return specs


def build_replay_matrix(compiler=None) -> int:
    compiler = compiler or find_executable(['gcc', 'gcc.exe']) or 'gcc'
    replay_dir = BUILD_ROOT / 'replay'
    replay_dir.mkdir(parents=True, exist_ok=True)
    specs = replay_specs()
    def clean_replay_outputs():
        for item in specs:
            for path in (item['exe'], item['metadata']):
                try:
                    path.unlink()
                except FileNotFoundError:
                    pass

    def validate_replay_outputs():
        errors = []
        for item in specs:
            exe, metadata_path = item['exe'], item['metadata']
            if not exe.is_file() or not os.access(exe, os.X_OK):
                errors.append(f"missing or non-executable target: {exe}")
                continue
            if not metadata_path.is_file():
                errors.append(f"missing build metadata: {metadata_path}")
                continue
            try:
                metadata = json.loads(metadata_path.read_text(encoding='utf-8'))
                if not build_metadata_matches(item, metadata):
                    errors.append(f"build metadata is stale or mismatched: {metadata_path}")
            except (OSError, ValueError, KeyError) as exc:
                errors.append(f"cannot validate build metadata {metadata_path}: {exc}")
        return errors

    for spec in specs:
        cmd = [compiler] + spec['flags'] + spec['compile_sources'] + ['-o', str(spec['exe'])]
        print('+', ' '.join(cmd))
        if subprocess.run(cmd, cwd=str(ROOT), check=False).returncode != 0:
            print(f"Replay build failed for {spec['id']}")
            clean_replay_outputs()
            return 1
        try:
            write_build_metadata(spec['exe'], cmd,
                                 [ROOT / name for name in spec['input_files']], spec['selection'],
                                 spec['id'], spec.get('profile', 'release'))
        except OSError as exc:
            print(f"Replay metadata write failed for {spec['id']}: {exc}")
            clean_replay_outputs()
            return 1
    errors = validate_replay_outputs()
    if errors:
        for error in errors:
            print(f'Replay build validation failed: {error}')
        clean_replay_outputs()
        return 1
    print(f'Validated all {len(specs)} replay executables and metadata sidecars.')
    return 0


def write_build_metadata(executable: Path, command, sources, selection: str,
                         version_id: str | None = None, profile: str | None = None) -> None:
    source_paths = [sources] if isinstance(sources, Path) else list(sources)
    primary_source = source_paths[0]
    compiler_result = subprocess.run(command[:1] + ['--version'], capture_output=True, text=True, check=False)
    metadata = {
        'executable': str(executable.resolve()),
        'command': [str(part) for part in command],
        'compiler_version': compiler_result.stdout.splitlines()[0] if compiler_result.stdout else 'unknown',
        'source': str(primary_source.resolve()),
        'source_sha256': hashlib.sha256(primary_source.read_bytes()).hexdigest(),
        'source_inputs': {
            str(source.resolve()): hashlib.sha256(source.read_bytes()).hexdigest()
            for source in source_paths
        },
        'selection': selection,
        'version_id': version_id,
        'profile': profile,
    }
    executable.with_suffix(executable.suffix + '.json').write_text(
        json.dumps(metadata, indent=2) + '\n', encoding='utf-8'
    )


def build_metadata_matches(spec, metadata, root: Path | None = None) -> bool:
    """Validate command, output, registration, and hashes of actual build inputs."""
    root = root or ROOT
    source_path = (root / spec['source']).resolve()
    input_files = spec.get('input_files', spec.get('compile_sources', [spec['source']]))
    compile_sources = spec.get('compile_sources', [spec['source']])
    expected_inputs = {
        str((root / name).resolve()): hashlib.sha256((root / name).read_bytes()).hexdigest()
        for name in input_files
    }
    expected_command = spec['flags'] + compile_sources + ['-o', str(spec['exe'])]
    recorded_command = metadata.get('command', [])
    return (
        metadata.get('source_inputs') == expected_inputs and
        metadata.get('source_sha256') == expected_inputs.get(str(source_path)) and
        metadata.get('selection') == spec['selection'] and
        recorded_command[1:] == expected_command and
        Path(metadata.get('executable', '')).resolve() == spec['exe'].resolve() and
        Path(metadata.get('source', '')).resolve() == source_path and
        metadata.get('version_id') == spec['id'] and
        metadata.get('profile') == spec.get('profile', 'release')
    )


def find_executable(names):
    for name in names:
        path = shutil.which(name)
        if path:
            return path
    for candidate in names:
        if candidate.strip().lower().endswith('.exe'):
            p = Path(candidate)
            if p.exists():
                return str(p)
    return None


def locate_gnuplot():
    found = find_executable(['gnuplot', 'gnuplot.exe', 'wgnuplot.exe'])
    if found:
        return found
    if DEFAULT_WINDOWS_GNUPLOT.exists():
        return str(DEFAULT_WINDOWS_GNUPLOT)
    return None


def locate_make():
    found = find_executable(['make', 'mingw32-make', 'gmake'])
    if found:
        return found
    if DEFAULT_WINDOWS_MAKE.exists():
        return str(DEFAULT_WINDOWS_MAKE)
    return None


def cpu_governor():
    governor_path = Path('/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor')
    try:
        return governor_path.read_text(encoding='utf-8').strip()
    except OSError:
        return 'not_available_on_host'


def cpu_governors():
    """Return every visible CPU governor without changing system policy."""
    root = Path('/sys/devices/system/cpu')
    result = {}
    for path in sorted(root.glob('cpu[0-9]*/cpufreq/scaling_governor')):
        try:
            result[path.parts[-3]] = path.read_text(encoding='utf-8').strip()
        except OSError:
            result[path.parts[-3]] = 'unavailable'
    return result or {'host': cpu_governor()}


def on_pi() -> bool:
    return sys.platform.startswith('linux') and platform.machine().lower() in {'aarch64', 'arm64'}


def unique_evidence_dir(kind: str, platform_label: str) -> Path:
    """Create a fresh evidence directory under the host-appropriate result tree."""
    root = ROOT / 'results' / ('pi' if on_pi() else 'laptop')
    slug = re.sub(r'[^a-zA-Z0-9_.-]+', '-', platform_label.strip()).strip('-.').lower() or 'host'
    parent = root / f'{kind}-runs' / slug
    parent.mkdir(parents=True, exist_ok=True)
    stamp = time.strftime('%Y%m%dT%H%M%SZ', time.gmtime())
    for suffix in range(1000):
        path = parent / (stamp if suffix == 0 else f'{stamp}-{suffix}')
        try:
            path.mkdir()
            return path
        except FileExistsError:
            continue
    raise RuntimeError(f'Unable to allocate unique evidence directory under {parent}')


def taskset_prefix(cpu_list: str | None):
    """Build a taskset prefix when requested and available; report actual use separately."""
    if not cpu_list:
        return [], False
    taskset = shutil.which('taskset')
    if not taskset:
        return [], False
    try:
        probe = subprocess.run([taskset, '-c', cpu_list, sys.executable, '-c', 'pass'],
                               capture_output=True, check=False)
    except OSError:
        return [], False
    return ([taskset, '-c', cpu_list], True) if probe.returncode == 0 else ([], False)


def pi_model_name():
    """Read the firmware model string when exposed by Linux device tree."""
    for path in (Path('/sys/firmware/devicetree/base/model'), Path('/proc/device-tree/model')):
        try:
            return path.read_bytes().decode('utf-8', errors='replace').strip('\x00\n ')
        except OSError:
            continue
    return None


def detect_datasets():
    matches = []
    for base in DATASET_DIRS:
        if base.exists():
            for p in sorted(base.iterdir()):
                if p.is_file() and ('test20' in p.name or 'stress' in p.name or 'dev_1M' in p.name):
                    matches.append(str(p))
    return matches


def choose_dataset(name: str):
    search = name.lower().strip()
    if not search:
        return None
    candidates = [p for p in ROOT.rglob('*') if p.is_file() and p.suffix.lower() == '.csv' and search in p.name.lower()]
    if candidates:
        return sorted(candidates)[0].resolve()
    for base in DATASET_DIRS:
        if base.exists():
            for p in sorted(base.iterdir()):
                if p.is_file() and search in p.name.lower():
                    return p.resolve()
    return None


def doctor() -> int:
    print('Tick-to-Trade project doctor')
    print('OS:', platform.platform())
    print('Machine:', platform.machine())
    print('Python:', sys.version.replace('\n', ' '))
    gcc = find_executable(['gcc', 'gcc.exe', 'x86_64-w64-mingw32-gcc', 'aarch64-linux-gnu-gcc'])
    print('GCC:', gcc or 'MISSING')
    print('Make:', locate_make() or 'MISSING')
    print('GNUplot:', locate_gnuplot() or 'MISSING')
    try:
        import numpy as np  # type: ignore
        print('NumPy:', np.__version__)
    except Exception:
        print('NumPy: MISSING')
    print('Datasets:', ', '.join(detect_datasets()) if detect_datasets() else 'NONE FOUND')
    print('Group settings: test20 short=3 long=5; dev/stress short=30 long=60')
    print('Optional tools: git, clang, aarch64-linux-gnu-as')
    missing = []
    if gcc is None:
        missing.append('gcc')
    if not detect_datasets():
        missing.append('official CSV dataset files')
    if missing:
        print('Missing prerequisites:', ', '.join(missing))
    return 0


def build_profile(profile: str, compiler: str | None = None) -> int:
    """Build Stage 0 and the baseline BuildSpec matching one profile."""
    prof = profile.lower()
    if prof not in {'debug', 'release'}:
        raise ValueError('profile must be debug or release')
    BUILD_ROOT.mkdir(parents=True, exist_ok=True)
    out_dir = BUILD_ROOT / prof
    out_dir.mkdir(parents=True, exist_ok=True)
    compiler = compiler or find_executable(['gcc', 'gcc.exe']) or 'gcc'
    flags = profile_build_flags(prof)
    stage0_cmd = [compiler] + flags + ['src/stage0.c', '-o', str(out_dir / 'stage0.exe')]
    for cmd, label in ((stage0_cmd, 'stage0.exe'),):
        print('+', ' '.join(cmd))
        if subprocess.run(cmd, cwd=str(ROOT), check=False).returncode != 0:
            print(f'Build failed for {label}')
            return 1
    spec = next(item for item in version_specs() if item.get('profile') == prof)
    return compile_version_spec(spec, compiler)


def compile_version_spec(spec, compiler: str | None = None) -> int:
    """Build one registered version and write metadata from its declared inputs."""
    compiler = compiler or find_executable(['gcc', 'gcc.exe']) or 'gcc'
    spec['exe'].parent.mkdir(parents=True, exist_ok=True)
    command = [compiler] + spec['flags'] + spec.get('compile_sources', [spec['source']]) + ['-o', str(spec['exe'])]
    print('+', ' '.join(command))
    result = subprocess.run(command, cwd=str(ROOT), check=False)
    if result.returncode != 0:
        print(f"Build failed for {spec['id']}")
        return result.returncode
    write_build_metadata(spec['exe'], command,
                         [ROOT / name for name in spec.get('input_files', spec.get('compile_sources', [spec['source']]))],
                         spec['selection'], spec['id'], spec.get('profile', 'release'))
    return 0


def build_normal_variants(compiler: str | None = None) -> int:
    compiler = compiler or find_executable(['gcc', 'gcc.exe']) or 'gcc'
    for spec in version_specs()[2:]:
        status = compile_version_spec(spec, compiler)
        if status != 0:
            return status
    return 0


def build_matrix(compiler: str | None = None) -> int:
    status = build_normal_variants(compiler)
    if status != 0:
        return status
    return build_replay_matrix(compiler)


def build_replay_support_test(compiler: str | None = None) -> int:
    """Compile the standalone C unit-test executable for replay support."""
    compiler = compiler or find_executable(['gcc', 'gcc.exe']) or 'gcc'
    test_build = BUILD_ROOT / 'tests'
    test_build.mkdir(parents=True, exist_ok=True)
    test_exe = test_build / ('replay_support_tests.exe' if os.name == 'nt' else 'replay_support_tests')
    command = [compiler, '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror'] + platform_build_flags() + [
        '-Iinclude', '-Iprovided', 'src/replay_support.c', 'tests/test_replay_support.c', '-o', str(test_exe)]
    print('+', ' '.join(command))
    return subprocess.run(command, cwd=str(ROOT), check=False).returncode


def build(profile: str = 'debug') -> int:
    compiler = find_executable(['gcc', 'gcc.exe']) or 'gcc'
    status = build_profile(profile, compiler)
    if status != 0:
        return status
    return build_matrix(compiler)


def build_all() -> int:
    """Build both baseline profiles and each comparison/replay matrix once."""
    compiler = find_executable(['gcc', 'gcc.exe']) or 'gcc'
    for profile in ('debug', 'release'):
        status = build_profile(profile, compiler)
        if status != 0:
            return status
    status = build_matrix(compiler)
    if status != 0:
        return status
    return build_replay_support_test(compiler)


def clean_build() -> int:
    """Remove only generated build outputs; datasets and results are outside this root."""
    if BUILD_ROOT.exists():
        shutil.rmtree(BUILD_ROOT)
    print(f'Removed generated build directory: {BUILD_ROOT}')
    return 0


def run_unittest_suite() -> int:
    tests_dir = ROOT / 'tests'
    if not tests_dir.exists():
        print('Missing tests directory')
        return 1
    compiler = find_executable(['gcc', 'gcc.exe'])
    if compiler is None:
        print('GCC is required for the replay-support C tests.')
        return 1
    if build_all() != 0:
        print('Tests not run: complete project build failed.')
        return 1
    test_exe = BUILD_ROOT / 'tests' / ('replay_support_tests.exe' if os.name == 'nt' else 'replay_support_tests')
    if not test_exe.is_file():
        print('Missing replay-support test executable after build-all.')
        return 1
    support_result = subprocess.run([str(test_exe)], cwd=str(ROOT), check=False)
    if support_result.returncode != 0:
        return support_result.returncode
    test_command = [sys.executable, '-m', 'unittest', 'discover', '-s', 'tests', '-p', 'test_*.py', '-v']
    return subprocess.run(test_command, cwd=str(ROOT), check=False).returncode


def read_test20_oracle():
    expected_path = ROOT / 'test20_expected - Copy (6).txt'
    if not expected_path.exists():
        raise FileNotFoundError(f'Missing official expected answer file: {expected_path}')
    expected_rows = []
    expected_bars = []
    section = None
    for line in expected_path.read_text(encoding='utf-8').splitlines():
        fields = line.split()
        if fields[:1] == ['trade'] and 'SMA_3' in fields:
            section = 'rows'
            continue
        if fields[:1] == ['second'] and 'trades' in fields:
            section = 'bars'
            continue
        if not fields or not fields[0].isdigit():
            continue
        if section == 'rows' and len(fields) in {7, 8}:
            expected_rows.append(fields[:7] + [fields[7] if len(fields) == 8 else '-'])
        elif section == 'bars' and len(fields) == 6:
            expected_bars.append(fields)
    if len(expected_rows) != 20 or not expected_bars:
        raise ValueError('Could not parse all official test20 rows and OHLC bars.')
    return expected_rows, expected_bars


def parse_trace(output: str):
    rows = []
    bars = []
    for line in output.splitlines():
        if line.startswith('TRACE_ROW|'):
            rows.append(line.split('|')[1:])
        elif line.startswith('TRACE_BAR|'):
            bars.append(line.split('|')[1:])
    return rows, bars


def close_enough(actual: str, expected: str) -> bool:
    from decimal import Decimal, InvalidOperation
    try:
        return abs(Decimal(actual) - Decimal(expected)) <= Decimal('0.00000001')
    except InvalidOperation:
        return False


def compare_test20_trace(output: str, expected_rows, expected_bars):
    actual_rows, actual_bars = parse_trace(output)
    errors = []
    if len(actual_rows) != len(expected_rows):
        errors.append(f'row count expected={len(expected_rows)} actual={len(actual_rows)}')
    if len(actual_bars) != len(expected_bars):
        errors.append(f'OHLC bar count expected={len(expected_bars)} actual={len(actual_bars)}')
    numeric_row_fields = {1, 2, 4, 5, 6}
    for index, (actual, expected) in enumerate(zip(actual_rows, expected_rows), start=1):
        normalized = [actual[0], actual[1], actual[2], actual[3], actual[4], actual[5], actual[6], actual[7]] if len(actual) == 8 else []
        if not normalized:
            errors.append(f'row {index}: malformed trace {actual!r}')
            continue
        for field_index, (got, want) in enumerate(zip(normalized, expected)):
            if field_index in numeric_row_fields and got != '-' and want != '-':
                if not close_enough(got, want):
                    errors.append(f'row {index} field {field_index}: expected={want} actual={got}')
            elif got != want:
                errors.append(f'row {index} field {field_index}: expected={want} actual={got}')
    for index, (actual, expected) in enumerate(zip(actual_bars, expected_bars), start=1):
        if len(actual) != 6:
            errors.append(f'bar {index}: malformed trace {actual!r}')
            continue
        for field_index, (got, want) in enumerate(zip(actual, expected)):
            if field_index in {1, 2, 3, 4}:
                if not close_enough(got, want):
                    errors.append(f'bar {index} field {field_index}: expected={want} actual={got}')
            elif got != want:
                errors.append(f'bar {index} field {field_index}: expected={want} actual={got}')
    return errors


def verify_version(spec, dataset: Path, expected_rows=None, expected_bars=None,
                   short_window: int = 3, long_window: int = 5) -> bool:
    executable = spec['exe'].resolve()
    print(f"VERSION_ID: {spec['id']}")
    print(f'EXECUTABLE: {executable}')
    print('SELECTION:', spec['selection'])
    metadata_path = spec['metadata']
    if not executable.exists() or not metadata_path.exists():
        print('BUILD_METADATA: MISSING; build the requested profiles first')
        print('STATUS: UNIMPLEMENTED' if not spec['implemented'] else 'STATUS: FAIL')
        return False
    metadata = json.loads(metadata_path.read_text(encoding='utf-8'))
    print('BUILD_METADATA:', json.dumps(metadata, sort_keys=True))
    if Path(metadata.get('executable', '')).resolve() != executable:
        print('STATUS: FAIL (metadata executable path mismatch)')
        return False
    if not build_metadata_matches(spec, metadata):
        print('STATUS: FAIL (build metadata is stale or does not match the registered configuration)')
        return False
    if not spec['implemented']:
        print('STATUS: UNIMPLEMENTED')
        return False
    command = [str(executable), str(dataset.resolve()), str(short_window), str(long_window), '--trace']
    result = subprocess.run(command, cwd=str(ROOT), capture_output=True, text=True, check=False)
    if result.returncode != 0:
        print(result.stderr.strip())
        print(f'STATUS: FAIL (exit={result.returncode})')
        return False
    if expected_rows is not None:
        errors = compare_test20_trace(result.stdout, expected_rows, expected_bars)
        if errors:
            for error in errors[:20]:
                print('MISMATCH:', error)
            print(f'STATUS: FAIL ({len(errors)} mismatch(es))')
            return False
    print('STATUS: PASS')
    return True


def output_summary(output: str):
    summary = {}
    signals = []
    bars = []
    for line in output.splitlines():
        if line.startswith('SIGNAL|'):
            signals.append(line.split('|')[1:])
        elif line.startswith('TRACE_BAR|'):
            bars.append(line.split('|')[1:])
        elif '=' in line:
            key, value = line.split('=', 1)
            summary[key] = value
    return summary, signals, bars


def compare_dataset_outputs(reference_output: str, candidate_output: str):
    ref_summary, ref_signals, ref_bars = output_summary(reference_output)
    got_summary, got_signals, got_bars = output_summary(candidate_output)
    errors = []
    for key in ('trades', 'buy_signals', 'sell_signals', 'ohlc_bars'):
        if ref_summary.get(key) != got_summary.get(key):
            errors.append(f'{key}: reference={ref_summary.get(key)} actual={got_summary.get(key)}')
    if 'final_vwap' in ref_summary and 'final_vwap' in got_summary:
        if not close_enough(ref_summary['final_vwap'], got_summary['final_vwap']):
            errors.append(f"final_vwap: reference={ref_summary['final_vwap']} actual={got_summary['final_vwap']}")
    if len(ref_signals) != len(got_signals):
        errors.append(f'signal count: reference={len(ref_signals)} actual={len(got_signals)}')
    for index, (expected, actual) in enumerate(zip(ref_signals, got_signals), start=1):
        if len(expected) != 4 or len(actual) != 4:
            errors.append(f'signal {index}: malformed row')
            continue
        if expected[:2] != actual[:2]:
            errors.append(f'signal {index}: reference={expected[:2]} actual={actual[:2]}')
        for sma_index in (2, 3):
            if not close_enough(expected[sma_index], actual[sma_index]):
                errors.append(f'signal {index} SMA: reference={expected[sma_index]} actual={actual[sma_index]}')
    if len(ref_bars) != len(got_bars):
        errors.append(f'OHLC count: reference={len(ref_bars)} actual={len(got_bars)}')
    for index, (expected, actual) in enumerate(zip(ref_bars, got_bars), start=1):
        if len(expected) != 6 or len(actual) != 6:
            errors.append(f'OHLC bar {index}: malformed row')
            continue
        if expected[0] != actual[0] or expected[5] != actual[5]:
            errors.append(f'OHLC bar {index} key/count: reference={expected[0], expected[5]} actual={actual[0], actual[5]}')
        for field_index in (1, 2, 3, 4):
            if not close_enough(expected[field_index], actual[field_index]):
                errors.append(f'OHLC bar {index} field {field_index}: reference={expected[field_index]} actual={actual[field_index]}')
    return errors


def _python_numpy_parity_gate(dataset: Path, short_window: int, long_window: int,
                              c_spec, *, trace: bool = False):
    """Compare the two Python implementations to a registered C reference."""
    python_root = str(ROOT / 'python')
    if python_root not in sys.path:
        sys.path.insert(0, python_root)
    try:
        import market_python
        import market_numpy
    except ImportError as exc:
        raise RuntimeError(f'Python engine import failed: {exc}') from exc
    exe = c_spec['exe'].resolve()
    args = [str(exe), str(dataset.resolve()), str(short_window), str(long_window)]
    command = args + (['--trace'] if trace else ['--signals-only', '--bars-only'])
    c_result = subprocess.run(command, cwd=str(ROOT), capture_output=True, text=True, check=False)
    if c_result.returncode:
        raise RuntimeError(f"C reference {c_spec['id']} failed: {c_result.stderr.strip()}")
    baseline_spec = next(spec for spec in version_specs()
                         if spec['id'] == 'v1_baseline_strtok_running_double_heap_O2')
    baseline_args = [str(baseline_spec['exe'].resolve()), str(dataset.resolve()),
                     str(short_window), str(long_window)]
    baseline_command = baseline_args + (['--trace'] if trace else ['--signals-only', '--bars-only'])
    baseline_result = subprocess.run(baseline_command, cwd=str(ROOT), capture_output=True,
                                     text=True, check=False)
    if baseline_result.returncode:
        raise RuntimeError(f"Frozen C correctness reference failed: {baseline_result.stderr.strip()}")
    c_differences = compare_dataset_outputs(baseline_result.stdout, c_result.stdout)
    if c_differences:
        raise RuntimeError('Selected C version differs from frozen v1: ' + '; '.join(c_differences[:5]))
    if trace:
        expected_rows, expected_bars = read_test20_oracle()
        oracle_errors = compare_test20_trace(c_result.stdout, expected_rows, expected_bars)
        if oracle_errors:
            raise RuntimeError('Selected C version differs from official test20: ' +
                               '; '.join(oracle_errors[:5]))
    def parity_close(a, b):
        if a is None or b is None:
            return a is None and b is None
        try:
            return math.isfinite(float(a)) and math.isfinite(float(b)) and abs(float(a) - float(b)) <= 1e-7
        except (TypeError, ValueError, OverflowError):
            return False
    py = market_python.compute_summary(dataset, short_window, long_window, include_rows=trace)
    npy = market_numpy.compute_summary(dataset, short_window, long_window, include_rows=trace)
    errors = []
    if (py['trade_count'], py['buy_signals'], py['sell_signals']) != (
            npy['trade_count'], npy['buy_signals'], npy['sell_signals']):
        errors.append('Python/NumPy trade or signal counts differ')
    if [(s['trade'], s['type']) for s in py['signals']] != [
            (s['trade'], s['type']) for s in npy['signals']]:
        errors.append('Python/NumPy signal ordinals or types differ')
    if len(py['ohlc']) != len(npy['ohlc']):
        errors.append('Python/NumPy OHLC bar count differs')
    elif py['ohlc'] != npy['ohlc']:
        # Numeric float differences are allowed only for displayed calculations;
        # keys, file-order bars, and trade counts are always exact.
        for a, b in zip(py['ohlc'], npy['ohlc']):
            if (a['second'], a['count']) != (b['second'], b['count']) or any(
                    not parity_close(a[k], b[k]) for k in ('open', 'high', 'low', 'close')):
                errors.append(f"Python/NumPy OHLC mismatch at second {a['second']}")
                break
    if not parity_close(py['final_vwap'], npy['final_vwap']):
        errors.append('Python/NumPy final VWAP differs')
    c_summary, c_signals, c_bars = output_summary(c_result.stdout)
    c_rows = [line.split('|') for line in c_result.stdout.splitlines()
              if line.startswith('TRACE_ROW|')]
    if trace and not c_signals:
        c_signals = [[row[1], row[8]] for row in c_rows if row[8] in {'BUY', 'SELL'}]
    if (str(py['trade_count']) != c_summary.get('trades') or
            str(py['buy_signals']) != c_summary.get('buy_signals') or
            str(py['sell_signals']) != c_summary.get('sell_signals')):
        errors.append('C/Python trade or signal counts differ')
    py_signals = [(str(s['trade']), s['type']) for s in py['signals']]
    if py_signals != [(s[0], s[1]) for s in c_signals]:
        errors.append('C/Python signal ordinals or types differ')
    if len(c_bars) != len(py['ohlc']):
        errors.append('C/Python OHLC bar count differs')
    else:
        for cbar, bar in zip(c_bars, py['ohlc']):
            if (int(cbar[0]), int(cbar[5])) != (bar['second'], bar['count']) or any(
                    not parity_close(cbar[i], bar[k]) for i, k in
                    ((1, 'open'), (2, 'high'), (3, 'low'), (4, 'close'))):
                errors.append(f"C/Python OHLC mismatch at second {bar['second']}")
                break
    if not parity_close(c_summary.get('final_vwap'), py['final_vwap']):
        errors.append('C/Python final VWAP differs')
    if trace:
        if len(c_rows) != len(py['rows']) or len(c_rows) != len(npy['rows']):
            errors.append('C/Python/NumPy row trace length differs')
        else:
            for c_row, a, b in zip(c_rows, py['rows'], npy['rows']):
                if int(c_row[1]) != a['trade'] or int(c_row[4]) != int(a['is_buy']) or \
                        a['is_buy'] != b['is_buy'] or a['short_ready'] != b['short_ready'] or \
                        a['long_ready'] != b['long_ready'] or a['signal'] != b['signal']:
                    errors.append(f"exact row parity mismatch at trade {a['trade']}")
                    break
                for ci, key in ((2, 'price'), (3, 'qty'), (5, 'vwap'),
                                (6, 'short_sma'), (7, 'long_sma')):
                    cval = None if c_row[ci] == '-' else c_row[ci]
                    if not parity_close(cval, a[key]) or not parity_close(a[key], b[key]):
                        errors.append(f"numeric row parity mismatch at trade {a['trade']} field {key}")
                        break
                if errors:
                    break
    return {'dataset': dataset.name, 'windows': [short_window, long_window],
            'c_version': c_spec['id'], 'trade_count': py['trade_count'],
            'buy_signals': py['buy_signals'], 'sell_signals': py['sell_signals'],
            'ohlc_bars': len(py['ohlc']), 'status': 'PASS' if not errors else 'FAIL',
            'errors': errors}


def python_comparison_profile(repeats: int, platform_label: str | None = None):
    """Return classification and isolated output kind for a repeat count."""
    if repeats < 1:
        raise ValueError('compare-python requires at least one repeat')
    smoke = repeats < 5
    if smoke and platform_label and re.search(
            r'final[-_ ]?(controlled|performance)|controlled[-_ ]?final',
            platform_label, re.IGNORECASE):
        raise ValueError('A final-controlled platform label requires at least five repeats')
    return (smoke, 'SMOKE / NON-FINAL' if smoke else 'FINAL-COMPARISON ELIGIBLE',
            'python-comparison-smoke' if smoke else 'python-comparisons')


def _comparison_svg(path: Path, medians, comparison_class: str):
    names = ['C', 'Python', 'NumPy']
    values = [medians[name]['median_wall_ns'] / 1e9 for name in names]
    colors = ['#526d82', '#d18b47', '#4c9b70']
    width, height, left, top, plotw, ploth = 800, 380, 100, 55, 640, 255
    maximum = max(values) or 1.0
    bars = []
    for i, (name, value, color) in enumerate(zip(names, values, colors)):
        y = top + i * (ploth / 3)
        barw = value / maximum * plotw
        bars.append(f'<text x="{left-12}" y="{y+25:.1f}" text-anchor="end">{name}</text>'
                    f'<rect x="{left}" y="{y+5:.1f}" width="{barw:.2f}" height="28" fill="{color}"/>'
                    f'<text x="{left+barw+8:.2f}" y="{y+25:.1f}">{value:.3f} s</text>')
    path.write_text('<svg xmlns="http://www.w3.org/2000/svg" width="800" height="380">'
                    '<style>text{font:14px sans-serif}</style>'
                    f'<text x="100" y="28" font-size="18">{html.escape(comparison_class)} — median process time</text>'
                    + ''.join(bars) + '</svg>\n', encoding='utf-8')


def compare_python_numpy(dataset_name: str, repeats: int, platform_label: str | None,
                         c_version: str, taskset_cpus: str | None,
                         short_window: int | None = None, long_window: int | None = None) -> int:
    try:
        smoke, comparison_class, evidence_kind = python_comparison_profile(repeats, platform_label)
    except ValueError as exc:
        print(exc)
        return 2
    dataset = choose_dataset(dataset_name)
    if not dataset:
        print(f'Dataset not found: {dataset_name}')
        return 2
    default_short, default_long = (3, 5) if 'test20' in dataset.name.lower() else (30, 60)
    short_window = default_short if short_window is None else short_window
    long_window = default_long if long_window is None else long_window
    if short_window <= 0 or long_window <= 0 or short_window > long_window:
        print('Windows must be positive and short must not exceed long')
        return 2
    specs = {spec['id']: spec for spec in version_specs()}
    if c_version not in specs:
        print(f'Unknown C reference version: {c_version}')
        return 2
    c_spec = specs[c_version]
    exe, metadata_path = c_spec['exe'].resolve(), c_spec['metadata']
    if not exe.exists() or not metadata_path.exists():
        print(f'C reference or metadata missing for {c_version}; build that existing registered target first')
        return 2
    if not build_metadata_matches(c_spec, json.loads(metadata_path.read_text(encoding='utf-8'))):
        print(f'C reference metadata is stale for {c_version}; comparison refused')
        return 2
    try:
        import numpy as np
        numpy_version = np.__version__
    except ImportError as exc:
        print(f'NumPy is required for this comparison: {exc}')
        return 2
    label = platform_label or f'{platform.system().lower()}-{platform.machine().lower()}-python-numpy'
    out = unique_evidence_dir(evidence_kind, label)
    gates = []
    for gate_path, sw, lw in [(choose_dataset('test20'), 3, 5), (dataset, short_window, long_window)]:
        if gate_path is None or any(g['dataset'] == gate_path.name for g in gates):
            continue
        print(f'Correctness gate: {gate_path.name} ({sw}/{lw})')
        try:
            gates.append(_python_numpy_parity_gate(gate_path, sw, lw, c_spec,
                                                   trace=gate_path.name.lower().find('test20') >= 0))
        except Exception as exc:
            gates.append({'dataset': gate_path.name, 'status': 'FAIL', 'errors': [str(exc)]})
            (out / 'correctness_gate.json').write_text(json.dumps(gates, indent=2) + '\n', encoding='utf-8')
            print(f'Correctness gate failed: {exc}')
            return 1
    (out / 'correctness_gate.json').write_text(json.dumps(gates, indent=2) + '\n', encoding='utf-8')

    task_prefix, taskset_applied = taskset_prefix(taskset_cpus)
    worker_rows = []
    commands = {
        'C': task_prefix + [str(exe), str(dataset.resolve()), str(short_window), str(long_window), '--bench-phases'],
        'Python': task_prefix + [sys.executable, str(ROOT / 'python' / 'market_python.py'),
                                 str(dataset.resolve()), '--short', str(short_window), '--long', str(long_window),
                                 '--benchmark-summary'],
        'NumPy': task_prefix + [sys.executable, str(ROOT / 'python' / 'market_numpy.py'),
                                str(dataset.resolve()), '--short', str(short_window), '--long', str(long_window),
                                '--benchmark-summary'],
    }
    for engine, command in commands.items():
        for repeat in range(1, repeats + 1):
            start = time.perf_counter_ns()
            proc = subprocess.run(command, cwd=str(ROOT), capture_output=True, text=True, check=False)
            wall_ns = time.perf_counter_ns() - start
            if proc.returncode:
                print(f'{engine} repeat {repeat} failed: {proc.stderr.strip()}')
                return 1
            inner = {}
            if engine == 'C':
                phase_line = next((line for line in proc.stdout.splitlines() if line.startswith('PHASES|')), '')
                for part in phase_line.split('|')[1:]:
                    if '=' in part:
                        k, v = part.split('=', 1)
                        inner[k] = int(v) if v.isdigit() else v
                summary, _, _ = output_summary(proc.stdout)
                trades = int(summary.get('trades', 0))
                load_ns, compute_ns = inner.get('loading_ns'), inner.get('compute_ns')
                engine_ns = (load_ns or 0) + (compute_ns or 0) + (inner.get('counting_ns') or 0)
            else:
                data = json.loads(proc.stdout)
                phase = data.get('result', {})
                trades = int(phase.get('trade_count', 0))
                load_ns, compute_ns = data.get('load_parse_ns'), data.get('compute_ns')
                engine_ns = data.get('engine_total_ns')
            worker_rows.append({'engine': engine, 'repeat': repeat, 'wall_ns': wall_ns,
                                'load_parse_ns': load_ns, 'compute_ns': compute_ns,
                                'engine_total_ns': engine_ns, 'trades': trades,
                                'throughput_trades_per_sec': trades / (wall_ns / 1e9) if wall_ns else ''})
            print(f'{engine} repeat {repeat}/{repeats}: process wall {wall_ns/1e9:.3f}s')
    raw_path = out / 'comparison_raw.csv'
    fields = ['engine', 'repeat', 'wall_ns', 'load_parse_ns', 'compute_ns', 'engine_total_ns',
              'trades', 'throughput_trades_per_sec']
    with raw_path.open('w', newline='', encoding='utf-8') as f:
        writer = csv.DictWriter(f, fieldnames=fields)
        writer.writeheader(); writer.writerows(worker_rows)
    medians = {}
    for engine in ('C', 'Python', 'NumPy'):
        subset = [r for r in worker_rows if r['engine'] == engine]
        medians[engine] = {
            'median_wall_ns': median(r['wall_ns'] for r in subset),
            'median_load_parse_ns': median(r['load_parse_ns'] for r in subset if r['load_parse_ns'] is not None)
                if any(r['load_parse_ns'] is not None for r in subset) else None,
            'median_compute_ns': median(r['compute_ns'] for r in subset if r['compute_ns'] is not None)
                if any(r['compute_ns'] is not None for r in subset) else None,
            'median_engine_total_ns': median(r['engine_total_ns'] for r in subset if r['engine_total_ns'] is not None)
                if any(r['engine_total_ns'] is not None for r in subset) else None,
            'median_throughput_trades_per_sec': median(r['throughput_trades_per_sec'] for r in subset),
        }
    median_path = out / 'comparison_medians.csv'
    with median_path.open('w', newline='', encoding='utf-8') as f:
        fields = ['engine', 'median_wall_ns', 'median_load_parse_ns', 'median_compute_ns',
                  'median_engine_total_ns', 'median_throughput_trades_per_sec']
        writer = csv.DictWriter(f, fieldnames=fields); writer.writeheader()
        for engine, values in medians.items():
            writer.writerow({'engine': engine, **values})
    _comparison_svg(out / 'comparison.svg', medians, comparison_class)
    dataset_digest = hashlib.sha256()
    with dataset.open('rb') as data_handle:
        for chunk in iter(lambda: data_handle.read(1024 * 1024), b''):
            dataset_digest.update(chunk)
    manifest = {
        'dataset': {'path': str(dataset.resolve()), 'bytes': dataset.stat().st_size,
                    'sha256': dataset_digest.hexdigest()},
        'windows': {'short': short_window, 'long': long_window}, 'repeats': repeats,
        'comparison_class': comparison_class,
        'c_version': c_version, 'python_version': sys.version.replace('\n', ' '),
        'numpy_version': numpy_version, 'host': platform.platform(),
        'architecture': platform.machine(), 'pi_model': pi_model_name() if on_pi() else None,
        'cpu_governors': cpu_governors(), 'taskset_requested': taskset_cpus,
        'taskset_applied': taskset_applied, 'taskset_prefix': task_prefix,
        'timing_boundary': {
            'process_wall': 'subprocess launch through compact summary output for all engines',
            'C_phases': 'native count/load/compute PHASES; selected version may have zero counting phase',
            'Python_phases': 'streaming CSV parse and computation interleave; phase split is unavailable',
            'NumPy_phases': 'CSV-to-array parsing plus batch engine compute; includes ordered Python SMA/crossovers',
            'warning': 'NumPy is an offline batch workload and is not replay latency; C process wall includes startup too.'},
        'correctness_gates': gates,
    }
    (out / 'comparison_manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    lines = [f'# C / Python / NumPy comparison — {comparison_class}', '',
             f"Dataset: `{dataset.name}` ({dataset.stat().st_size} bytes), windows {short_window}/{long_window}, {repeats} repeats.",
             '', '| Engine | Median process wall (s) | Throughput (trades/s) | Load/parse (ms) | Compute (ms) |',
             '|---|---:|---:|---:|---:|']
    for engine, v in medians.items():
        load = '' if v['median_load_parse_ns'] is None else f"{v['median_load_parse_ns']/1e6:.3f}"
        comp = '' if v['median_compute_ns'] is None else f"{v['median_compute_ns']/1e6:.3f}"
        lines.append(f"| {engine} | {v['median_wall_ns']/1e9:.6f} | {v['median_throughput_trades_per_sec']:.2f} | {load} | {comp} |")
    lines += [f'', f'Classification: **{comparison_class}** ({repeats} repeat(s)). Process wall includes interpreter/executable startup and compact result output. The plain Python engine streams rows, so parsing and calculation cannot be split without changing its memory/timing path. NumPy is batch/offline; its wall time must not be described as replay latency. No engine is assumed to win.', '']
    (out / 'comparison.md').write_text('\n'.join(lines), encoding='utf-8')
    print('Comparison class:', comparison_class)
    print('Comparison evidence:', out)
    print('Raw timings:', raw_path)
    print('Medians:', median_path)
    return 0


def write_correctness_matrix(entries, output_path: Path = CORRECTNESS_MATRIX) -> None:
    fields = ['dataset', 'dataset_path', 'windows', 'version_id', 'executable',
              'status', 'trades', 'signal_events', 'ohlc_bars', 'detail']
    output_path.parent.mkdir(parents=True, exist_ok=True)
    rows = {}
    if output_path.exists():
        with output_path.open(newline='', encoding='utf-8') as handle:
            for row in csv.DictReader(handle):
                rows[(row['dataset'], row['version_id'])] = row
    for entry in entries:
        rows[(entry['dataset'], entry['version_id'])] = entry
    with output_path.open('w', newline='', encoding='utf-8') as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows.values())


def verify_dataset_matrix(dataset: Path, short_window: int, long_window: int,
                          correctness_path: Path = CORRECTNESS_MATRIX) -> int:
    specs = version_matrix()
    reference_spec = next(spec for spec in specs if spec['id'] == 'v1_baseline_strtok_running_double_heap_O2')
    reference_exe = reference_spec['exe'].resolve()
    ref_args = [str(reference_exe), str(dataset.resolve()), str(short_window), str(long_window),
                '--signals-only', '--bars-only']
    ref_result = subprocess.run(ref_args, cwd=str(ROOT), capture_output=True, text=True, check=False)
    if ref_result.returncode != 0:
        print('Reference Version 1 failed:', ref_result.stderr.strip())
        return 1
    ref_summary, ref_signals, ref_bars = output_summary(ref_result.stdout)
    dataset_id = dataset.name
    windows = f'{short_window}:{long_window}'
    entries = []
    all_passed = True
    for spec in specs:
        executable = spec['exe'].resolve()
        print(f"VERSION_ID: {spec['id']}")
        print(f'EXECUTABLE: {executable}')
        print('SELECTION:', spec['selection'])
        metadata_path = spec['metadata']
        if not spec['implemented']:
            print('STATUS: UNIMPLEMENTED')
            all_passed = False
            entries.append({'dataset': dataset_id, 'dataset_path': str(dataset.resolve()), 'windows': windows,
                            'version_id': spec['id'], 'executable': str(executable), 'status': 'UNIMPLEMENTED',
                            'trades': ref_summary.get('trades', ''), 'signal_events': len(ref_signals),
                            'ohlc_bars': len(ref_bars), 'detail': 'required version not implemented'})
            continue
        if not executable.exists() or not metadata_path.exists():
            print('STATUS: FAIL (executable or metadata missing)')
            all_passed = False
            entries.append({'dataset': dataset_id, 'dataset_path': str(dataset.resolve()), 'windows': windows,
                            'version_id': spec['id'], 'executable': str(executable), 'status': 'FAIL',
                            'trades': ref_summary.get('trades', ''), 'signal_events': len(ref_signals),
                            'ohlc_bars': len(ref_bars), 'detail': 'executable or metadata missing'})
            continue
        metadata = json.loads(metadata_path.read_text(encoding='utf-8'))
        if not build_metadata_matches(spec, metadata):
            print('STATUS: FAIL (stale build metadata)')
            all_passed = False
            entries.append({'dataset': dataset_id, 'dataset_path': str(dataset.resolve()), 'windows': windows,
                            'version_id': spec['id'], 'executable': str(executable), 'status': 'FAIL',
                            'trades': ref_summary.get('trades', ''), 'signal_events': len(ref_signals),
                            'ohlc_bars': len(ref_bars), 'detail': 'stale build metadata'})
            continue
        command = [str(executable), str(dataset.resolve()), str(short_window), str(long_window),
                   '--signals-only', '--bars-only']
        result = subprocess.run(command, cwd=str(ROOT), capture_output=True, text=True, check=False)
        if result.returncode != 0:
            print('ERROR:', result.stderr.strip())
            print(f'STATUS: FAIL (exit={result.returncode})')
            all_passed = False
            entries.append({'dataset': dataset_id, 'dataset_path': str(dataset.resolve()), 'windows': windows,
                            'version_id': spec['id'], 'executable': str(executable), 'status': 'FAIL',
                            'trades': ref_summary.get('trades', ''), 'signal_events': len(ref_signals),
                            'ohlc_bars': len(ref_bars), 'detail': f'exit {result.returncode}: {result.stderr.strip()}'})
            continue
        mismatches = compare_dataset_outputs(ref_result.stdout, result.stdout)
        if mismatches:
            for mismatch in mismatches[:20]:
                print('MISMATCH:', mismatch)
            print(f'STATUS: FAIL ({len(mismatches)} discrepancy(s) against frozen v1)')
            all_passed = False
            status = 'FAIL'
            detail = '; '.join(mismatches[:5])
        else:
            print('STATUS: PASS (summary, signals, OHLC match frozen v1)')
            status = 'PASS'
            detail = 'summary, signal positions/SMA values, and all OHLC fields match frozen v1'
        entries.append({'dataset': dataset_id, 'dataset_path': str(dataset.resolve()), 'windows': windows,
                        'version_id': spec['id'], 'executable': str(executable), 'status': status,
                        'trades': ref_summary.get('trades', ''), 'signal_events': len(ref_signals),
                        'ohlc_bars': len(ref_bars), 'detail': detail})
    write_correctness_matrix(entries, correctness_path)
    return 0 if all_passed else 1


def verify_dataset(dataset_name: str, all_versions: bool = False) -> int:
    dataset = choose_dataset(dataset_name)
    if dataset is None:
        print(f'No dataset matching {dataset_name!r} found.')
        return 2
    spec = version_specs()[1]
    if dataset_name.lower() == 'test20':
        expected_rows, expected_bars = read_test20_oracle()
        return 0 if verify_version(spec, dataset, expected_rows, expected_bars) else 1
    if 'dev' in dataset.name.lower() or 'stress' in dataset.name.lower():
        return verify_dataset_matrix(dataset, 30, 60)
    return verify_dataset_matrix(dataset, 30, 60)


def version_matrix():
    return version_specs()


def verify_all_versions(implemented_only: bool = False,
                        correctness_path: Path = CORRECTNESS_MATRIX) -> int:
    dataset = choose_dataset('test20')
    if dataset is None:
        print('Official test20 CSV is missing; verification incomplete.')
        return 2
    try:
        expected_rows, expected_bars = read_test20_oracle()
    except (OSError, ValueError) as exc:
        print(f'Official test20 oracle unavailable: {exc}')
        return 2
    all_passed = True
    incomplete = False
    matrix_entries = []
    expected_signal_events = sum(row[7] in {'BUY', 'SELL'} for row in expected_rows)
    for spec in version_matrix():
        if not spec['implemented']:
            print(f"VERSION_ID: {spec['id']}\nEXECUTABLE: {spec['exe'].resolve()}\nSELECTION: {spec['selection']}\nSTATUS: UNIMPLEMENTED")
            incomplete = True
            matrix_entries.append({'dataset': dataset.name, 'dataset_path': str(dataset.resolve()),
                                   'windows': '3:5', 'version_id': spec['id'],
                                   'executable': str(spec['exe'].resolve()), 'status': 'UNIMPLEMENTED',
                                   'trades': len(expected_rows), 'signal_events': expected_signal_events,
                                   'ohlc_bars': len(expected_bars), 'detail': 'required version not implemented'})
            if implemented_only:
                continue
            all_passed = False
            continue
        passed = verify_version(spec, dataset, expected_rows, expected_bars)
        matrix_entries.append({'dataset': dataset.name, 'dataset_path': str(dataset.resolve()),
                               'windows': '3:5', 'version_id': spec['id'],
                               'executable': str(spec['exe'].resolve()), 'status': 'PASS' if passed else 'FAIL',
                               'trades': len(expected_rows), 'signal_events': expected_signal_events,
                               'ohlc_bars': len(expected_bars),
                               'detail': 'all official rows/signals/OHLC match' if passed else 'version verification failed'})
        if not passed:
            all_passed = False
    write_correctness_matrix(matrix_entries, correctness_path)
    if incomplete and not implemented_only:
        print('VERIFICATION: INCOMPLETE (required versions remain unimplemented)')
        return 1
    return 0 if all_passed else 1


def dataset_facts(dataset: Path):
    digest = hashlib.sha256()
    with dataset.open('rb') as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b''):
            digest.update(block)
    trade_count = 0
    with dataset.open(newline='', encoding='utf-8') as handle:
        reader = csv.reader(handle)
        for row in reader:
            if not row or row[0].strip().lower() == 'trade_id':
                continue
            if len(row) == 7:
                trade_count += 1
    stat = dataset.stat()
    return {'sha256': digest.hexdigest(), 'bytes': stat.st_size, 'trades': trade_count}


def parse_phase_line(output: str):
    for line in output.splitlines():
        if line.startswith('PHASES|'):
            return {key: value for key, value in (part.split('=', 1) for part in line.split('|')[1:])}
    return None


def write_csv(path: Path, rows, fields):
    with path.open('w', newline='', encoding='utf-8') as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def build_benchmark_reports(raw_rows, specs, facts, dataset: Path,
                            result_root: Path, run_status: str, preliminary_reason: str) -> int:
    medians = {}
    summaries = []
    environment = raw_rows[0] if raw_rows else {}
    report_context = {
        'host_class': environment.get('host_class', 'Pi' if on_pi() else 'laptop/host'),
        'governor_performance': environment.get('governor_performance', False),
        'taskset_used': environment.get('taskset_used', False),
        'final_controlled': environment.get('final_controlled', False),
        'status': run_status,
        'preliminary_reason': preliminary_reason,
    }
    for spec in specs:
        runs = [row for row in raw_rows if row['version_id'] == spec['id']]
        end_median = median(int(row['end_to_end_ns']) for row in runs)
        compute_median = median(int(row['compute_ns']) for row in runs)
        medians[spec['id']] = end_median
        summaries.append({
            'version_id': spec['id'],
            'executable': str(spec['exe'].resolve()),
            'median_end_to_end_ns': end_median,
            'median_counting_ns': median(int(row['counting_ns']) for row in runs),
            'median_loading_ns': median(int(row['loading_ns']) for row in runs),
            'median_compute_ns': compute_median,
            'median_process_peak_memory_bytes': median(int(row['process_peak_memory_bytes']) for row in runs),
            'process_memory_metric': runs[0]['process_memory_metric'],
            'trades_per_second': facts['trades'] * 1_000_000_000 / end_median if end_median else 0,
            'status': run_status,
            'preliminary_reason': preliminary_reason,
            **report_context,
        })
    reference_id = 'v0_baseline_strtok_running_double_heap_O0'
    reference_median = medians[reference_id]
    for row in summaries:
        row['speedup_vs_v0'] = reference_median / row['median_end_to_end_ns'] if row['median_end_to_end_ns'] else 0

    result_root.mkdir(parents=True, exist_ok=True)
    raw_fields = ['run', 'timestamp_utc', 'version_id', 'executable', 'dataset_path', 'dataset_sha256', 'dataset_bytes',
                  'trade_count', 'platform_label', 'host_class', 'host_os', 'host_machine', 'pi_model', 'host_cpu', 'governor_performance',
                  'logical_cpu_count', 'cpu_governor', 'cpu_governors', 'taskset_requested_cpus', 'taskset_used', 'final_controlled', 'run_status', 'preliminary_reason',
                  'compiler_version', 'compiler_command',
                  'source', 'source_sha256', 'source_inputs', 'end_to_end_ns', 'counting_ns', 'loading_ns', 'compute_ns',
                  'compute_scope', 'logical_allocated_bytes', 'logical_used_bytes',
                  'process_peak_memory_bytes', 'process_memory_metric', 'return_code']
    write_csv(result_root / 'benchmark_raw.csv', raw_rows, raw_fields)
    write_csv(result_root / 'benchmark_medians.csv', summaries,
              ['version_id', 'executable', 'host_class', 'governor_performance', 'taskset_used', 'final_controlled', 'median_end_to_end_ns', 'median_counting_ns',
               'median_loading_ns', 'median_compute_ns', 'median_process_peak_memory_bytes',
               'process_memory_metric', 'trades_per_second', 'speedup_vs_v0', 'status', 'preliminary_reason'])
    speedup_rows = [{
        'version_id': row['version_id'],
        'median_end_to_end_ns': row['median_end_to_end_ns'],
        'speedup_vs_frozen_v0': row['speedup_vs_v0'],
        'trades_per_second': row['trades_per_second'],
        **report_context,
        'status': row['status'],
        'preliminary_reason': row['preliminary_reason'],
    } for row in summaries]
    write_csv(result_root / 'speedup_table.csv', speedup_rows,
              ['version_id', 'host_class', 'governor_performance', 'taskset_used', 'final_controlled', 'median_end_to_end_ns', 'speedup_vs_frozen_v0', 'trades_per_second', 'status', 'preliminary_reason'])

    controlled_pairs = [
        ('compiler_O0_to_O2', 'v0_baseline_strtok_running_double_heap_O0', 'v1_baseline_strtok_running_double_heap_O2', 'Same frozen baseline/parser/SMA/representation/storage; optimization flag changes.'),
        ('compiler_O2_to_O3', 'v5_manual_digit_parser_running_double_stream_O2', 'v7_manual_digit_parser_running_double_stream_O3', 'Same reader/SMA/representation/storage; optimization flag changes.'),
        ('reader_helper_to_pointer', 'v2_helper_sscanf_naive_double_stream_O2', 'v3_own_pointer_reader_naive_double_stream_O2', 'Same naive SMA, double price, stream storage, and O2.'),
        ('reader_pointer_to_manual', 'v3_own_pointer_reader_naive_double_stream_O2', 'v4_manual_digit_parser_naive_double_stream_O2', 'Same naive SMA, double price, stream storage, and O2.'),
        ('sma_naive_to_running', 'v4_manual_digit_parser_naive_double_stream_O2', 'v5_manual_digit_parser_running_double_stream_O2', 'Same manual parser, double price, stream storage, and O2; naive path retains shared running comparator sums for stable equality semantics.'),
        ('price_double_to_fixed_i64', 'v5_manual_digit_parser_running_double_stream_O2', 'v6_manual_digit_parser_running_fixed_i64_1e8_stream_O2', 'Same manual parser, running SMA, stream storage, and O2; fixed path keeps a double comparator shadow to preserve signals.'),
        ('storage_stream_to_static', 'v5_manual_digit_parser_running_double_stream_O2', 'v8_manual_running_double_static_trade_array_O2', 'Same manual parser, running double SMA, price, and O2; load phase is separately reported.'),
        ('storage_stream_to_exact_malloc', 'v5_manual_digit_parser_running_double_stream_O2', 'v9_manual_running_double_exact_malloc_trade_array_O2', 'Same parser/algorithm/representation/O2; exact malloc includes a valid-row counting pass.'),
        ('storage_static_to_exact_malloc', 'v8_manual_running_double_static_trade_array_O2', 'v9_manual_running_double_exact_malloc_trade_array_O2', 'Same parser/algorithm/representation/O2; compares global capacity reservation with exact two-pass heap allocation.'),
        ('storage_exact_malloc_aos_to_soa', 'v9_manual_running_double_exact_malloc_trade_array_O2', 'v10_manual_running_double_soa_O2', 'Same manual parser, running double SMA, double representation, O2 flags, exact two-pass count/load boundary, and dataset. The smaller SoA payload may reduce bytes fetched, while separate memory streams may hurt locality; both are hypotheses, not promised gains, and slower results remain visible.'),
    ]
    controlled_rows = []
    for comparison, left_id, right_id, note in controlled_pairs:
        if left_id not in medians or right_id not in medians:
            continue
        left = next(row for row in summaries if row['version_id'] == left_id)
        right = next(row for row in summaries if row['version_id'] == right_id)
        left_raw = next(row for row in raw_rows if row['version_id'] == left_id)
        right_raw = next(row for row in raw_rows if row['version_id'] == right_id)
        left_memory_median = median(int(row['process_peak_memory_bytes']) for row in raw_rows
                                    if row['version_id'] == left_id)
        right_memory_median = median(int(row['process_peak_memory_bytes']) for row in raw_rows
                                     if row['version_id'] == right_id)
        controlled_rows.append({
            'comparison': comparison,
            'left_version': left_id,
            'right_version': right_id,
            'left_median_end_to_end_ns': left['median_end_to_end_ns'],
            'right_median_end_to_end_ns': right['median_end_to_end_ns'],
            'right_speedup_vs_left': left['median_end_to_end_ns'] / right['median_end_to_end_ns'],
            'left_median_compute_ns': left['median_compute_ns'],
            'right_median_compute_ns': right['median_compute_ns'],
            'left_logical_allocated_bytes': left_raw['logical_allocated_bytes'],
            'right_logical_allocated_bytes': right_raw['logical_allocated_bytes'],
            'left_logical_used_bytes': left_raw['logical_used_bytes'],
            'right_logical_used_bytes': right_raw['logical_used_bytes'],
            'left_median_process_peak_memory_bytes': left_memory_median,
            'right_median_process_peak_memory_bytes': right_memory_median,
            'note': note,
            **report_context,
        })
    write_csv(result_root / 'controlled_comparisons.csv', controlled_rows,
              ['comparison', 'left_version', 'right_version', 'left_median_end_to_end_ns',
               'right_median_end_to_end_ns', 'right_speedup_vs_left', 'left_median_compute_ns',
               'right_median_compute_ns', 'left_logical_allocated_bytes', 'right_logical_allocated_bytes',
               'left_logical_used_bytes', 'right_logical_used_bytes',
               'left_median_process_peak_memory_bytes', 'right_median_process_peak_memory_bytes',
               'note', 'host_class', 'governor_performance', 'taskset_used', 'final_controlled',
               'status', 'preliminary_reason'])

    comparison_table = [
        '# Controlled comparison table', '',
        'A speedup above 1.0 means the right version was faster; below 1.0 means it was slower. '
        'Values are kept as measured, including regressions.', '',
        '| Comparison | Left → right | End-to-end speedup | Left / right median ns | Left / right logical bytes | Left / right median process peak bytes | Note |',
        '|---|---|---:|---:|---:|---:|---|',
    ]
    for row in controlled_rows:
        note = row['note'].replace('|', '\\|')
        comparison_table.append(
            f"| {row['comparison']} | {row['left_version']} → {row['right_version']} | "
            f"{row['right_speedup_vs_left']:.6f}x | {row['left_median_end_to_end_ns']} / "
            f"{row['right_median_end_to_end_ns']} | {row['left_logical_allocated_bytes']} / "
            f"{row['right_logical_allocated_bytes']} | {row['left_median_process_peak_memory_bytes']} / "
            f"{row['right_median_process_peak_memory_bytes']} | {note} |")
    (result_root / 'controlled_comparisons.md').write_text('\n'.join(comparison_table) + '\n', encoding='utf-8')

    memory_rows = []
    for spec in specs:
        runs = [row for row in raw_rows if row['version_id'] == spec['id']]
        memory_rows.append({
            'version_id': spec['id'],
            'logical_allocated_bytes': runs[0]['logical_allocated_bytes'],
            'logical_used_bytes': runs[0]['logical_used_bytes'],
            'median_process_peak_memory_bytes': median(int(row['process_peak_memory_bytes']) for row in runs),
            'process_memory_metric': runs[0]['process_memory_metric'],
            **report_context,
        })
    write_csv(result_root / 'memory_comparison.csv', memory_rows,
              ['version_id', 'logical_allocated_bytes', 'logical_used_bytes',
               'median_process_peak_memory_bytes', 'process_memory_metric', 'host_class',
               'governor_performance', 'taskset_used', 'final_controlled', 'status', 'preliminary_reason'])

    data_file = result_root / 'speedup_ladder.dat'
    short_labels = {
        'v0_baseline_strtok_running_double_heap_O0': 'v0 O0',
        'v1_baseline_strtok_running_double_heap_O2': 'v1 O2',
        'v2_helper_sscanf_naive_double_stream_O2': 'helper sscanf',
        'v3_own_pointer_reader_naive_double_stream_O2': 'own pointer',
        'v4_manual_digit_parser_naive_double_stream_O2': 'manual naive',
        'v5_manual_digit_parser_running_double_stream_O2': 'manual running O2',
        'v6_manual_digit_parser_running_fixed_i64_1e8_stream_O2': 'fixed i64',
        'v7_manual_digit_parser_running_double_stream_O3': 'manual running O3',
        'v8_manual_running_double_static_trade_array_O2': 'static AoS',
        'v9_manual_running_double_exact_malloc_trade_array_O2': 'exact malloc AoS',
        'v10_manual_running_double_soa_O2': 'exact malloc SoA',
    }
    with data_file.open('w', encoding='utf-8') as handle:
        for index, row in enumerate(summaries, start=1):
            handle.write(f"{index} {row['speedup_vs_v0']:.8f} '{short_labels[row['version_id']]}'\n")
    plot_script = result_root / 'speedup_ladder.gnuplot'
    output_png = (result_root / 'speedup_ladder.png').as_posix()
    plot_data = data_file.as_posix()
    plot_script.write_text(
        "set terminal pngcairo size 1500,850 font 'Verdana,10'\n"
        f"set output '{output_png}'\n"
        "set title 'Stress-file speedup against frozen Version 0 (-O0)'\n"
        "set xlabel 'Version / controlled rung'\n"
        "set ylabel 'Median end-to-end speedup'\n"
        "set style data boxes\n"
        "set style fill solid 0.75\n"
        "set grid ytics\n"
        "set xtics rotate by -30\n"
        f"plot '{plot_data}' using 1:2:xticlabels(3) notitle\n",
        encoding='utf-8',
    )
    gnuplot = locate_gnuplot()
    if gnuplot is None:
        max_speed = max((float(row['speedup_vs_v0']) for row in summaries), default=1.0) or 1.0
        bar_lines = []
        for index, row in enumerate(summaries):
            y = 48 + index * 34
            width = max(1, int(650 * float(row['speedup_vs_v0']) / max_speed))
            bar_lines.append(f'<text x="8" y="{y + 17}" font-size="12">{html.escape(short_labels[row["version_id"]])}</text>'
                             f'<rect x="175" y="{y}" width="{width}" height="22" fill="#3478b8"/>'
                             f'<text x="{183 + width}" y="{y + 17}" font-size="12">{float(row["speedup_vs_v0"]):.2f}x</text>')
        svg = ('<svg xmlns="http://www.w3.org/2000/svg" width="1050" height="%d">' % (80 + len(summaries) * 34) +
               '<text x="8" y="22" font-size="16">Median end-to-end speedup vs frozen v0</text>' +
               ''.join(bar_lines) + '</svg>')
        (result_root / 'speedup_ladder.svg').write_text(svg, encoding='utf-8')
        print('GNUplot unavailable; generated dependency-free SVG graph; PNG not created.')
    else:
        plot_result = subprocess.run([gnuplot, str(plot_script)], cwd=str(ROOT), capture_output=True, text=True, check=False)
        if plot_result.returncode != 0 or not (result_root / 'speedup_ladder.png').exists():
            print('GNUplot failed:', plot_result.stderr.strip())
    print('Benchmark evidence:')
    for name in ('benchmark_raw.csv', 'benchmark_medians.csv', 'speedup_ladder.png', 'speedup_ladder.svg',
                 'speedup_ladder.gnuplot', 'speedup_table.csv', 'controlled_comparisons.csv', 'controlled_comparisons.md', 'memory_comparison.csv',
                 'correctness_matrix.csv'):
        print(' ', (result_root / name).resolve())
    print(f"dataset_sha256={facts['sha256']} bytes={facts['bytes']} trades={facts['trades']}")
    return 0


def unique_benchmark_run_dir(platform_label: str, dataset_sha256: str) -> Path:
    slug = re.sub(r'[^a-zA-Z0-9_.-]+', '-', platform_label.strip()).strip('-.').lower() or 'unknown-host'
    parent = ROOT / 'results' / 'benchmark-runs' / slug
    parent.mkdir(parents=True, exist_ok=True)
    base = time.strftime('%Y%m%dT%H%M%SZ', time.gmtime()) + '-' + dataset_sha256[:10]
    for suffix in range(1000):
        candidate = parent / (base if suffix == 0 else f'{base}-{suffix}')
        try:
            candidate.mkdir()
            return candidate
        except FileExistsError:
            continue
    raise RuntimeError(f'Unable to allocate unique benchmark directory under {parent}')


def benchmark_dataset(dataset_name: str, repeats: int = 5,
                      platform_label: str | None = None, versions: list[str] | None = None,
                      taskset_cpus: str | None = None, final_controlled: bool = False) -> int:
    if repeats < 5:
        print('Benchmark requires at least five repeats.')
        return 2
    dataset = choose_dataset(dataset_name)
    if dataset is None:
        print(f'No dataset matching {dataset_name!r} found.')
        return 2
    if 'stress' not in dataset.name.lower():
        print('The benchmark ladder is defined on stress_full.csv; choose --dataset stress.')
        return 2

    facts = dataset_facts(dataset)
    if platform_label is None:
        platform_label = f'{platform.system().lower()}-{platform.machine().lower()}'
    selected_ids = list(dict.fromkeys(versions or [item['id'] for item in version_specs()]))
    all_specs = {item['id']: item for item in version_specs()}
    if FROZEN_V0 not in selected_ids:
        print(f'Version selection must include frozen reference {FROZEN_V0}.')
        return 2
    unknown = [item for item in selected_ids if item not in all_specs]
    if unknown:
        print('Unknown versions:', ', '.join(unknown))
        return 2
    specs = [all_specs[item] for item in selected_ids]
    if final_controlled and (not on_pi() or repeats < 5):
        print('--final-controlled requires a native Linux AArch64 Pi run with at least five repeats.')
        return 2
    try:
        run_root = unique_evidence_dir('performance', platform_label)
    except OSError as exc:
        print(f'Cannot create unique benchmark output directory: {exc}')
        return 2
    governors = cpu_governors()
    governor = cpu_governor()
    prefix, taskset_available = taskset_prefix(taskset_cpus)
    pinned = bool(taskset_cpus and taskset_available)
    governor_ok = bool(governors) and all(value == 'performance' for value in governors.values())
    if governor_ok and pinned:
        run_status = 'CONTROLLED'
        preliminary_reason = ''
    else:
        run_status = 'PRELIMINARY'
        reasons = []
        if not governor_ok:
            reasons.append('not every CPU governor is performance')
        if taskset_cpus and not taskset_available:
            reasons.append('taskset requested but unavailable; affinity was not applied')
        if not pinned:
            reasons.append('no selected CPU pinning was applied')
        preliminary_reason = '; '.join(reasons)
    if final_controlled and not (governor_ok and pinned):
        print('Final-controlled run rejected: require performance governor on every CPU and applied taskset pinning.')
        return 2
    manifest = {
        'run_directory': str(run_root.resolve()),
        'platform_label': platform_label,
        'actual_platform': platform.platform(),
        'machine': platform.machine(),
        'cpu': platform.processor() or platform.machine(),
        'pi_model': pi_model_name(),
        'logical_cpu_count': os.cpu_count() or 0,
        'cpu_governor': governor,
        'cpu_governors': governors,
        'taskset_requested_cpus': taskset_cpus,
        'taskset_used': pinned,
        'taskset_available': taskset_available,
        'compiler_path': find_executable(['gcc', 'gcc.exe']),
        'datetime_utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
        'final_controlled_requested': final_controlled,
        'is_pi': on_pi(),
        'status': run_status,
        'preliminary_reason': preliminary_reason,
        'dataset_path': str(dataset.resolve()),
        'dataset_sha256': facts['sha256'],
        'dataset_bytes': facts['bytes'],
        'trade_count': facts['trades'],
        'repeats': repeats,
        'selected_versions': selected_ids,
        'final_controlled': bool(final_controlled and governor_ok and pinned and on_pi()),
        'affinity_changed': False,
        'system_settings_changed': False,
    }
    (run_root / 'run_manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    correctness_path = run_root / 'correctness_matrix.csv'

    gates = []
    capture = io.StringIO()
    with contextlib.redirect_stdout(capture):
        test20_status = verify_all_versions(correctness_path=correctness_path)
    gates.append(('official test20', test20_status))
    dev_dataset = choose_dataset('dev_1M')
    stress_dataset = choose_dataset('stress_full')
    if dev_dataset is None or stress_dataset is None:
        print('Development or stress dataset missing; benchmark is blocked.')
        return 2
    for label, path in (('development', dev_dataset), ('stress reference', stress_dataset)):
        capture = io.StringIO()
        with contextlib.redirect_stdout(capture):
            gate_status = verify_dataset_matrix(path, 30, 60, correctness_path)
        gates.append((label, gate_status))
    failed_gates = [label for label, status in gates if status != 0]
    for label, status in gates:
        print(f'correctness_gate|{label}|{"PASS" if status == 0 else "FAIL"}')
    print(f'benchmark_status|{run_status}|platform_label={platform_label}|governor={governor}')
    print(f'run_directory|{run_root.resolve()}')
    if failed_gates:
        print('Benchmark blocked; unresolved correctness gates:', ', '.join(failed_gates))
        print(capture.getvalue()[-6000:])
        return 1

    specs = [spec for spec in specs if spec['implemented']]
    raw_rows = []
    raw_path = run_root / 'benchmark_raw.csv'
    raw_fields = ['run', 'timestamp_utc', 'version_id', 'executable', 'dataset_path', 'dataset_sha256', 'dataset_bytes',
                  'trade_count', 'platform_label', 'host_class', 'host_os', 'host_machine', 'pi_model', 'host_cpu', 'governor_performance', 'logical_cpu_count', 'cpu_governor', 'cpu_governors', 'taskset_requested_cpus', 'taskset_used', 'final_controlled',
                  'run_status', 'preliminary_reason',
                  'compiler_version', 'compiler_command',
                  'source', 'source_sha256', 'source_inputs', 'end_to_end_ns', 'counting_ns', 'loading_ns', 'compute_ns',
                  'compute_scope', 'logical_allocated_bytes', 'logical_used_bytes',
                  'process_peak_memory_bytes', 'process_memory_metric', 'return_code']
    for spec in specs:
        if not spec['exe'].exists() or not spec['metadata'].exists():
            print(f"Benchmark blocked: missing build for {spec['id']}; run project.py build --profile release.")
            return 3
    with raw_path.open('w', newline='', encoding='utf-8') as handle:
        writer = csv.DictWriter(handle, fieldnames=raw_fields)
        writer.writeheader()
        for repeat_index in range(1, repeats + 1):
            for spec in specs:
                metadata = json.loads(spec['metadata'].read_text(encoding='utf-8'))
                executable = spec['exe'].resolve()
                command = prefix + [str(executable), str(dataset.resolve()), '30', '60', '--bench-phases']
                started = time.perf_counter_ns()
                result = subprocess.run(command, cwd=str(ROOT), capture_output=True, text=True, check=False)
                elapsed_ns = time.perf_counter_ns() - started
                summary, _, _ = output_summary(result.stdout)
                phases = parse_phase_line(result.stdout) or {}
                if result.returncode != 0 or summary.get('trades') != str(facts['trades']) or not phases:
                    print(f"Ineligible benchmark run: {spec['id']} repeat={repeat_index} rc={result.returncode}")
                    print(result.stderr.strip())
                    print('summary=', summary, 'phases=', phases)
                    return 1
                source_hash = metadata.get('source_sha256', '')
                row = {
                    'run': repeat_index,
                    'timestamp_utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
                    'version_id': spec['id'],
                    'executable': str(executable),
                    'dataset_path': str(dataset.resolve()),
                    'dataset_sha256': facts['sha256'],
                    'dataset_bytes': facts['bytes'],
                    'trade_count': facts['trades'],
                    'platform_label': platform_label,
                    'host_class': 'Pi' if on_pi() else 'laptop/host',
                    'host_os': platform.platform(),
                    'host_machine': platform.machine(),
                    'pi_model': pi_model_name() or '',
                    'governor_performance': all(value == 'performance' for value in cpu_governors().values()),
                    'host_cpu': platform.processor() or platform.machine(),
                    'logical_cpu_count': os.cpu_count() or 0,
                    'cpu_governor': cpu_governor(),
                    'cpu_governors': json.dumps(cpu_governors(), sort_keys=True),
                    'taskset_requested_cpus': taskset_cpus or '',
                    'taskset_used': pinned,
                    'final_controlled': manifest['final_controlled'],
                    'cpu_governors': json.dumps(cpu_governors(), sort_keys=True),
                    'taskset_requested_cpus': taskset_cpus or '',
                    'taskset_used': pinned,
                    'final_controlled': final_controlled,
                    'run_status': run_status,
                    'preliminary_reason': preliminary_reason,
                    'compiler_version': metadata.get('compiler_version', 'unknown'),
                    'compiler_command': json.dumps(metadata.get('command', [])),
                    'source': metadata.get('source', ''),
                    'source_sha256': source_hash,
                    'source_inputs': json.dumps(metadata.get('source_inputs', {}), sort_keys=True),
                    'end_to_end_ns': elapsed_ns,
                    'counting_ns': phases.get('counting_ns', '0'),
                    'loading_ns': phases.get('loading_ns', '0'),
                    'compute_ns': phases.get('compute_ns', '0'),
                    'compute_scope': phases.get('compute_scope', 'unknown'),
                    'logical_allocated_bytes': phases.get('logical_allocated_bytes', '0'),
                    'logical_used_bytes': phases.get('logical_used_bytes', '0'),
                    'process_peak_memory_bytes': phases.get('process_peak_memory_bytes', '0'),
                    'process_memory_metric': phases.get('process_memory_metric', 'unavailable'),
                    'return_code': result.returncode,
                }
                raw_rows.append(row)
                writer.writerow(row)
                handle.flush()
                print(f"timed|run={repeat_index}|version={spec['id']}|end_to_end_ns={elapsed_ns}")

    status = build_benchmark_reports(raw_rows, specs, facts, dataset, run_root,
                                     run_status, preliminary_reason)
    (run_root / 'environment_metadata.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    (run_root / 'benchmark_summary.md').write_text(
        '# Benchmark summary\n\n'
        f'- Platform: {platform_label} ({"Pi" if on_pi() else "laptop/host"})\n'
        f'- Status: {run_status}; final controlled: {manifest["final_controlled"]}\n'
        f'- Governors by CPU: `{json.dumps(governors, sort_keys=True)}`\n'
        f'- Taskset requested: `{taskset_cpus or "none"}`; actually used: {pinned}\n'
        f'- Dataset: `{dataset.resolve()}`; bytes: {facts["bytes"]}; SHA256: `{facts["sha256"]}`; trades: {facts["trades"]}\n'
        f'- Repeats: {repeats}; versions: {", ".join(selected_ids)}\n\n'
        'Timing includes process launch and computation. Phase columns retain count, load, and compute distinctions.\n',
        encoding='utf-8')
    print('Benchmark summary:', (run_root / 'benchmark_summary.md').resolve())
    return status


def unique_replay_run_dir(version_id: str, speed: int, dataset_sha256: str) -> Path:
    host = re.sub(r'[^a-zA-Z0-9_.-]+', '-',
                  f'{platform.system().lower()}-{platform.machine().lower()}').strip('-.')
    if sys.platform.startswith('linux') and platform.machine().lower() in {'aarch64', 'arm64'}:
        parent = ROOT / 'results' / 'pi'
    else:
        parent = ROOT / 'results' / 'replays' / host
    parent.mkdir(parents=True, exist_ok=True)
    base = f'{time.strftime("%Y%m%dT%H%M%SZ", time.gmtime())}-{dataset_sha256[:10]}-{version_id}-x{speed}'
    for suffix in range(1000):
        candidate = parent / (base if suffix == 0 else f'{base}-{suffix}')
        try:
            candidate.mkdir()
            return candidate
        except FileExistsError:
            continue
    raise RuntimeError(f'Unable to allocate unique replay directory under {parent}')


def replay_command(args) -> int:
    dataset = choose_dataset(args.dataset)
    if dataset is None:
        print(f'No dataset matching {args.dataset!r} found.')
        return 2
    spec = next((item for item in replay_specs() if item['id'] == args.version), None)
    if spec is None:
        print(f'Unknown replay version: {args.version}')
        return 2
    executable = spec['exe'].resolve()
    metadata_path = spec['metadata']
    if not executable.exists() or not metadata_path.exists():
        print(f'Replay executable missing: {executable}; run `python scripts/project.py build --profile release`.')
        return 3
    metadata = json.loads(metadata_path.read_text(encoding='utf-8'))
    if not build_metadata_matches(spec, metadata):
        print('Replay executable metadata is stale; rebuild replay targets.')
        return 3

    facts = dataset_facts(dataset)
    try:
        run_dir = unique_replay_run_dir(spec['id'], args.speed, facts['sha256'])
    except OSError as exc:
        print(f'Cannot create replay output directory: {exc}')
        return 2
    is_test20 = 'test20' in dataset.name.lower()
    short_window = args.short if args.short is not None else (3 if is_test20 else 30)
    long_window = args.long if args.long is not None else (5 if is_test20 else 60)
    latency_path = (run_dir / 'latency_samples.csv') if args.latency_csv is None else Path(args.latency_csv).resolve()
    decision_path = None
    if args.decision_log:
        decision_path = (run_dir / 'decisions.csv') if args.decision_csv is None else Path(args.decision_csv).resolve()
    elif args.decision_csv is not None:
        decision_path = Path(args.decision_csv).resolve()
    for output_path in (latency_path, decision_path):
        if output_path is not None:
            output_path.parent.mkdir(parents=True, exist_ok=True)

    manifest = {
        'version_id': spec['id'],
        'executable': str(executable),
        'configuration': spec['selection'],
        'build_metadata': metadata,
        'dataset_path': str(dataset.resolve()),
        'dataset_sha256': facts['sha256'],
        'dataset_bytes': facts['bytes'],
        'dataset_trade_count': facts['trades'],
        'speed': args.speed,
        'windows': {'short': short_window, 'long': long_window},
        'duration_seconds': args.duration_seconds,
        'record_limit': args.limit,
        'output_mode': args.output,
        'signal_policy': args.signal_policy,
        'latency_csv': str(latency_path.resolve()),
        'decision_csv': str(decision_path.resolve()) if decision_path else None,
        'host_os': platform.platform(),
        'host_machine': platform.machine(),
        'datetime_utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
        'compiler_version': metadata.get('compiler_version', 'unknown'),
        'build_flags': metadata.get('command', [])[1:],
        'taskset_cpu_list': os.environ.get('TASKSET_CPUS'),
        'hardware_led_claimed': False,
        'timing': 'monotonic absolute release deadlines anchored to first timestamp',
    }
    (run_dir / 'run_manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    command = [str(executable), str(dataset.resolve()), '--version-id', spec['id'],
               '--speed', str(args.speed), '--short', str(short_window), '--long', str(long_window),
               '--output', args.output, '--signal-policy', args.signal_policy,
               '--latency-csv', str(latency_path.resolve())]
    if args.duration_seconds is not None:
        command += ['--duration-seconds', str(args.duration_seconds)]
    if args.limit is not None:
        command += ['--limit', str(args.limit)]
    if args.capacity is not None:
        command += ['--capacity', str(args.capacity)]
    if decision_path is not None:
        command += ['--decision-csv', str(decision_path.resolve())]
    print('Replay executable:', executable)
    print('Replay build command:', json.dumps(metadata.get('command', [])))
    print('Replay configuration:', spec['selection'])
    print('Replay run directory:', run_dir.resolve())
    print('Replay command:', json.dumps(command))
    with subprocess.Popen(command, cwd=str(ROOT), stdout=subprocess.PIPE, text=True,
                          bufsize=1) as process:
        output_lines = []
        if process.stdout is not None:
            for line in process.stdout:
                output_lines.append(line)
                print(line, end='', flush=True)
        return_code = process.wait()
    (run_dir / 'summary.log').write_text(''.join(output_lines), encoding='utf-8')
    print('Replay latency samples:', latency_path.resolve())
    if decision_path is not None:
        print('Replay primary/shadow decisions:', decision_path.resolve())
    print('Replay run manifest:', (run_dir / 'run_manifest.json').resolve())
    return return_code


def replay_metadata_for(version_id: str):
    spec = next((item for item in replay_specs() if item['id'] == version_id), None)
    if spec is None:
        return None, None, f'Unknown replay version: {version_id}'
    exe, metadata_path = spec['exe'].resolve(), spec['metadata']
    if not exe.is_file() or not metadata_path.is_file() or not os.access(exe, os.X_OK):
        return spec, None, f'Replay executable/metadata missing or executable unavailable: {exe}; run `python3 scripts/project.py build --profile release`.'
    try:
        metadata = json.loads(metadata_path.read_text(encoding='utf-8'))
        if not build_metadata_matches(spec, metadata):
            return spec, None, f'Stale or mismatched replay build metadata for {version_id}; rebuild replay targets.'
    except (OSError, ValueError, KeyError) as exc:
        return spec, None, f'Cannot validate replay metadata for {version_id}: {exc}'
    return spec, metadata, None


def select_fast_version_from_pi_evidence() -> tuple[str | None, str]:
    evidence_files = sorted((ROOT / 'results' / 'pi').rglob('benchmark_medians.csv'),
                            key=lambda path: path.stat().st_mtime, reverse=True)
    valid_ids = {spec['id'] for spec in replay_specs()}
    for report_path in evidence_files:
        try:
            with report_path.open(newline='', encoding='utf-8') as handle:
                eligible = [row for row in csv.DictReader(handle)
                            if row.get('status', '').upper() == 'ELIGIBLE' and
                            row.get('version_id') in valid_ids and
                            float(row.get('median_end_to_end_ns', 'nan')) > 0]
            if eligible:
                winner = min(eligible, key=lambda row: float(row['median_end_to_end_ns']))
                return winner['version_id'], str(report_path)
        except (OSError, ValueError, TypeError):
            continue
    return None, 'No eligible results/pi/**/benchmark_medians.csv evidence was found.'


def benchmark_evidence_record(version_id: str, report_path: str | None):
    if report_path is None:
        return None
    path = Path(report_path)
    try:
        with path.open(newline='', encoding='utf-8') as handle:
            row = next((item for item in csv.DictReader(handle)
                        if item.get('version_id') == version_id and
                        item.get('status', '').upper() == 'ELIGIBLE'), None)
        manifest_path = path.parent / 'run_manifest.json'
        run_manifest = json.loads(manifest_path.read_text(encoding='utf-8')) if manifest_path.exists() else None
        return {'report': str(path.resolve()), 'selected_row': row,
                'run_manifest': run_manifest}
    except (OSError, ValueError):
        return {'report': str(path.resolve()), 'selected_row': None, 'run_manifest': None}


def unique_analysis_run_dir(kind: str, dataset_hash: str, output_dir: str | None) -> Path:
    if output_dir:
        chosen = Path(output_dir).expanduser().resolve()
        chosen.mkdir(parents=True, exist_ok=False)
        return chosen
    if sys.platform.startswith('linux') and platform.machine().lower() in {'aarch64', 'arm64'}:
        parent = ROOT / 'results' / 'pi' / f'{kind}-runs'
    else:
        parent = ROOT / 'results' / 'laptop' / f'{kind}-runs'
    parent.mkdir(parents=True, exist_ok=True)
    stamp = time.strftime('%Y%m%dT%H%M%SZ', time.gmtime())
    for index in range(1000):
        suffix = '' if index == 0 else f'-{index}'
        candidate = parent / f'{stamp}-{dataset_hash[:10]}{suffix}'
        try:
            candidate.mkdir()
            return candidate
        except FileExistsError:
            continue
    raise RuntimeError(f'Cannot allocate unique {kind} run directory beneath {parent}')


def replay_options_for_analysis(args, output_mode='off') -> list[str]:
    opts = ['--speed', str(args.speed), '--output', output_mode,
            '--signal-policy', args.signal_policy]
    if args.short is not None:
        opts += ['--short', str(args.short)]
    if args.long is not None:
        opts += ['--long', str(args.long)]
    if getattr(args, 'limit', None) is not None:
        opts += ['--limit', str(args.limit)]
    if getattr(args, 'duration_seconds', None) is not None:
        opts += ['--duration-seconds', str(args.duration_seconds)]
    return opts


def launch_replay_analysis(spec, dataset: Path, args, directory: Path,
                           label: str, shared_start_ns: int | None = None,
                           progress: bool = False, hardware: bool = False):
    latency_path = directory / f'{label}_latency_samples.csv'
    decisions_path = directory / f'{label}_decisions.csv'
    command = [str(spec['exe'].resolve()), str(dataset.resolve()), '--version-id', spec['id']]
    command += replay_options_for_analysis(args, 'hardware' if hardware else 'off')
    command += ['--latency-csv', str(latency_path), '--decision-csv', str(decisions_path)]
    if shared_start_ns is not None:
        command += ['--start-at-ns', str(shared_start_ns)]
    if progress:
        command += ['--race-progress']
    return command, latency_path, decisions_path


def rival_run_is_complete(run_dir: Path) -> bool:
    """An output folder is valid only after its required summaries are committed."""
    try:
        manifest = json.loads((run_dir / 'run_manifest.json').read_text(encoding='utf-8'))
        return (manifest.get('status') == 'complete' and
                all((run_dir / name).is_file() for name in
                    ('baseline_summary.json', 'fast_summary.json', 'rival_decisions.csv',
                     'pnl_summary.json', 'reaction_sweep.csv', 'reaction_sweep.svg',
                     'compact_progress.log')))
    except (OSError, ValueError):
        return False


def run_compact_rival(dataset: Path, run_dir: Path, run_config: dict, outputs: dict,
                      args, fast_version: str) -> int:
    required_bytes = max(256 * 1024 * 1024, dataset.stat().st_size * 5)
    free_bytes = shutil.disk_usage(run_dir.parent).free
    if free_bytes < required_bytes:
        print(f'Insufficient free disk space for compact replay: need about {required_bytes} bytes, '
              f'have {free_bytes} bytes.')
        return 5
    progress_path = run_dir / 'compact_progress.log'
    progress_path.write_text('compact rival analysis started\n', encoding='utf-8')
    temp_dir = None
    try:
        temp_dir = Path(tempfile.mkdtemp(prefix='rival-replay-', dir=run_dir.parent))
        for label, spec in (('baseline', outputs['baseline_spec']), ('fast', outputs['fast_spec'])):
            command, latency_path, decisions_path = launch_replay_analysis(
                spec, dataset, args, temp_dir, label)
            outputs[label] = {'command': command, 'latency': latency_path,
                              'decisions': decisions_path}
            print(f'Running {label} replay: {json.dumps(command)}', flush=True)
            result = subprocess.run(command, cwd=str(ROOT), capture_output=True,
                                    text=True, check=False)
            summary_path = run_dir / f'{label}_summary.log'
            with summary_path.open('w', encoding='utf-8') as handle:
                handle.write(result.stdout)
                handle.write(result.stderr)
                handle.flush()
                os.fsync(handle.fileno())
            if result.returncode != 0:
                print(f'{label} replay failed with exit {result.returncode}: {result.stderr.strip()}')
                print(f'Temporary replay evidence retained at {temp_dir}')
                return result.returncode
        run_config['commands'] = {key: value['command'] for key, value in outputs.items()
                                  if key in ('baseline', 'fast')}
        manifest_path = run_dir / 'run_manifest.json'
        manifest_path.write_text(json.dumps(run_config, indent=2) + '\n', encoding='utf-8')
        records, processed, ignored_duplicates = stream_replay_pair(
            dataset, outputs['baseline']['decisions'], outputs['baseline']['latency'],
            outputs['fast']['decisions'], outputs['fast']['latency'], progress_path)
        base_result = compact_simulate(records, 'baseline', args.rival_reaction_us, ignored_duplicates)
        fast_result = compact_simulate(records, 'fast', args.rival_reaction_us, ignored_duplicates)
        write_compact_decisions(records, run_dir / 'rival_decisions.csv', args.rival_reaction_us)

        def summary_value(result):
            return {key: decimal_text(value) if isinstance(value, Decimal) else value
                    for key, value in result.items()}

        for label, result in (('baseline', base_result), ('fast', fast_result)):
            path = run_dir / f'{label}_summary.json'
            temp_path = path.with_suffix('.json.tmp')
            temp_path.write_text(json.dumps(summary_value(result), indent=2) + '\n', encoding='utf-8')
            os.replace(temp_path, path)
        summary = {'rival_reaction_us': args.rival_reaction_us,
                   'baseline_version': args.baseline_version, 'fast_version': fast_version,
                   'processed_baseline_rows': processed, 'processed_fast_rows': processed,
                   'actionable_signals_retained': len(records),
                   'ignored_duplicate_signals': ignored_duplicates,
                   'baseline': summary_value(base_result), 'fast': summary_value(fast_result),
                   'money_lost': decimal_text(fast_result['pnl'] - base_result['pnl']),
                   'cash_flow_pnl_definition': 'sum(sell execution prices) - sum(buy execution prices); no fees/forced close'}
        summary_path = run_dir / 'pnl_summary.json'
        temp_summary = run_dir / 'pnl_summary.json.tmp'
        temp_summary.write_text(json.dumps(summary, indent=2) + '\n', encoding='utf-8')
        os.replace(temp_summary, summary_path)
        sweep_rows = compact_sweep(records, ignored_duplicates)
        fields = ['rival_reaction_us', 'baseline_pnl', 'fast_pnl', 'money_lost',
                  'baseline_trigger_price_count', 'baseline_next_price_count',
                  'fast_trigger_price_count', 'fast_next_price_count',
                  'baseline_unfilled_orders', 'fast_unfilled_orders']
        sweep_path = run_dir / 'reaction_sweep.csv'
        with sweep_path.open('w', newline='', encoding='utf-8') as handle:
            writer = csv.DictWriter(handle, fieldnames=fields)
            writer.writeheader()
            for row in sweep_rows:
                writer.writerow({key: decimal_text(value) if isinstance(value, Decimal) else value
                                 for key, value in row.items()})
            handle.flush()
            os.fsync(handle.fileno())
        write_sweep_svg(run_dir / 'reaction_sweep.svg', sweep_rows,
                        args.baseline_version, fast_version)
        shutil.rmtree(temp_dir)
        run_config.update({'status': 'complete', 'compact_evidence': True,
                           'processed_rows': processed,
                           'actionable_signals_retained': len(records)})
        manifest_path = run_dir / 'run_manifest.json'
        temp_manifest = run_dir / 'run_manifest.json.tmp'
        temp_manifest.write_text(json.dumps(run_config, indent=2) + '\n', encoding='utf-8')
        os.replace(temp_manifest, manifest_path)
        signal_size = sys.getsizeof(records[0]) if records else 0
        memory_estimate = sys.getsizeof(records) + len(records) * (
            signal_size + 2 * sys.getsizeof(records[0].trigger_price if records else None))
        print(f'Processed baseline rows: {processed}; fast rows: {processed}; '
              f'actionable signals retained: {len(records)}; '
              f'retained memory estimate: {memory_estimate} bytes')
        print(f'Rival output directory: {run_dir.resolve()}')
        print(f'Compact rival summary: {summary_path}; sweep: {sweep_path}')
        return 0
    except (OSError, ValueError, KeyError, csv.Error) as exc:
        print(f'Compact rival analysis failed: {exc}')
        if temp_dir is not None and temp_dir.exists():
            print(f'Temporary replay evidence retained at {temp_dir}')
        return 4


def run_rival_command(args) -> int:
    dataset = choose_dataset(args.dataset)
    if dataset is None:
        print(f'No dataset matching {args.dataset!r} found.')
        return 2
    fast_version = args.fast_version
    evidence_path = None
    if fast_version is None:
        fast_version, evidence_path = select_fast_version_from_pi_evidence()
        if fast_version is None:
            print(f'Cannot select a default fast version: {evidence_path} Supply --fast-version explicitly.')
            return 2
    baseline_spec, baseline_meta, error = replay_metadata_for(args.baseline_version)
    if error:
        print(error)
        return 3
    fast_spec, fast_meta, error = replay_metadata_for(fast_version)
    if error:
        print(error)
        return 3
    facts = dataset_facts(dataset)
    try:
        run_dir = unique_analysis_run_dir('rival', facts['sha256'], args.output_dir)
    except (OSError, FileExistsError) as exc:
        print(f'Cannot create rival output directory: {exc}')
        return 2
    run_config = {
        'dataset': str(dataset.resolve()), 'dataset_sha256': facts['sha256'],
        'baseline_version': args.baseline_version, 'fast_version': fast_version,
        'fast_version_evidence': benchmark_evidence_record(fast_version, evidence_path), 'speed': args.speed,
        'short': args.short, 'long': args.long, 'limit': args.limit,
        'duration_seconds': args.duration_seconds, 'signal_policy': args.signal_policy,
        'rival_reaction_us': args.rival_reaction_us, 'sweep': args.sweep,
        'host': platform.platform(), 'architecture': platform.machine(),
        'datetime_utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
        'baseline_build_metadata': baseline_meta, 'fast_build_metadata': fast_meta,
        'edge_policy': 'strictly faster iff processing_duration_us < reaction_us; final-dataset signal unfilled; no forced liquidation',
    }
    manifest_path = run_dir / 'run_manifest.json'
    manifest_path.write_text(json.dumps(run_config, indent=2) + '\n', encoding='utf-8')
    compact = (getattr(args, 'compact_evidence', False) or
               (getattr(args, 'limit', None) is None and
                getattr(args, 'duration_seconds', None) is None))
    run_config.update({'status': 'in_progress', 'compact_evidence': compact})
    manifest_path.write_text(json.dumps(run_config, indent=2) + '\n', encoding='utf-8')
    if compact:
        return run_compact_rival(dataset, run_dir, run_config,
                                 {'baseline_spec': baseline_spec, 'fast_spec': fast_spec},
                                 args, fast_version)
    outputs = {}
    for label, spec in (('baseline', baseline_spec), ('fast', fast_spec)):
        command, latency_path, decisions_path = launch_replay_analysis(spec, dataset, args, run_dir, label)
        outputs[label] = {'command': command, 'latency': latency_path, 'decisions': decisions_path}
    run_config['commands'] = {key: value['command'] for key, value in outputs.items()}
    manifest_path.write_text(json.dumps(run_config, indent=2) + '\n', encoding='utf-8')
    for label, output in outputs.items():
        print(f'Running {label} replay: {json.dumps(output["command"])}', flush=True)
        result = subprocess.run(output['command'], cwd=str(ROOT), capture_output=True, text=True, check=False)
        (run_dir / f'{label}_summary.log').write_text(result.stdout + result.stderr, encoding='utf-8')
        if result.returncode != 0:
            print(f'{label} replay failed with exit {result.returncode}: {result.stderr.strip()}')
            return result.returncode
    try:
        baseline_signals, prices, baseline_sequence = read_replay_signals(
            dataset, outputs['baseline']['decisions'], outputs['baseline']['latency'])
        fast_signals, fast_prices, fast_sequence = read_replay_signals(
            dataset, outputs['fast']['decisions'], outputs['fast']['latency'])
    except (OSError, ValueError, KeyError) as exc:
        print(f'Cannot analyze replay output: {exc}')
        return 4
    if baseline_sequence != fast_sequence or prices != fast_prices:
        print('Baseline and fast strategy signal sequences/trade prices differ; refusing to attribute P&L differences to speed.')
        return 4
    baseline_result = simulate(baseline_signals, prices, args.rival_reaction_us)
    fast_result = simulate(fast_signals, prices, args.rival_reaction_us)
    decision_path = run_dir / 'rival_decisions.csv'
    decision_fields = ['version', 'trade', 'signal', 'processing_duration_us', 'trigger_price',
                       'execution_price', 'decision', 'holding_before', 'holding_after', 'cash_pnl_after']
    with decision_path.open('w', newline='', encoding='utf-8') as handle:
        writer = csv.DictWriter(handle, fieldnames=decision_fields)
        writer.writeheader()
        for label, result in (('baseline', baseline_result), ('fast', fast_result)):
            for row in result.decisions or []:
                writer.writerow({'version': label, **{
                    key: decimal_text(value) if isinstance(value, Decimal) else value
                    for key, value in row.items()
                }})
    def result_summary(result):
        return {key: decimal_text(value) if isinstance(value, Decimal) else value
                for key, value in result.__dict__.items() if key != 'decisions'}

    summary = {'rival_reaction_us': args.rival_reaction_us,
               'baseline_version': args.baseline_version, 'fast_version': fast_version,
               'baseline': result_summary(baseline_result), 'fast': result_summary(fast_result),
               'money_lost': decimal_text(fast_result.pnl - baseline_result.pnl),
               'cash_flow_pnl_definition': 'sum(sell execution prices) - sum(buy execution prices); no fees/forced close'}
    (run_dir / 'pnl_summary.json').write_text(json.dumps(summary, indent=2) + '\n', encoding='utf-8')
    if args.sweep:
        baseline_sweep = reaction_sweep(baseline_signals, prices)
        fast_sweep = reaction_sweep(fast_signals, prices)
        sweep_path = run_dir / 'reaction_sweep.csv'
        fields = ['rival_reaction_us', 'baseline_pnl', 'fast_pnl', 'money_lost',
                  'baseline_trigger_price_count', 'baseline_next_price_count',
                  'fast_trigger_price_count', 'fast_next_price_count',
                  'baseline_unfilled_orders', 'fast_unfilled_orders']
        rows = []
        for base, fast in zip(baseline_sweep, fast_sweep):
            rows.append({
                'rival_reaction_us': base['rival_reaction_us'],
                'baseline_pnl': decimal_text(base['pnl']), 'fast_pnl': decimal_text(fast['pnl']),
                'money_lost': decimal_text(fast['pnl'] - base['pnl']),
                'baseline_trigger_price_count': base['trigger_price_count'],
                'baseline_next_price_count': base['next_price_count'],
                'fast_trigger_price_count': fast['trigger_price_count'],
                'fast_next_price_count': fast['next_price_count'],
                'baseline_unfilled_orders': base['unfilled_orders'],
                'fast_unfilled_orders': fast['unfilled_orders'],
            })
        with sweep_path.open('w', newline='', encoding='utf-8') as handle:
            writer = csv.DictWriter(handle, fieldnames=fields)
            writer.writeheader()
            writer.writerows(rows)
        write_sweep_svg(run_dir / 'reaction_sweep.svg', rows, args.baseline_version, fast_version)
    print(f'Rival output directory: {run_dir.resolve()}')
    print(f'Baseline cash P&L: {decimal_text(baseline_result.pnl)}; fast cash P&L: {decimal_text(fast_result.pnl)}; money_lost=fast-baseline={summary["money_lost"]}')
    print(f'Decisions: {decision_path}; summary: {run_dir / "pnl_summary.json"}')
    if args.sweep:
        print(f'Reaction sweep: {run_dir / "reaction_sweep.csv"}; graph: {run_dir / "reaction_sweep.svg"}')
    return 0


class RaceAccount:
    def __init__(self):
        self.holding = 0
        self.pnl = Decimal(0)
        self.buy_count = 0
        self.sell_count = 0
        self.pending = None
        self.unfilled = 0
        self.open_position_entry_price = None

    def fill(self, side: int, price: float):
        price = Decimal(str(price))
        if side > 0 and self.holding == 0:
            self.pnl -= price
            self.holding = 1
            self.buy_count += 1
            self.open_position_entry_price = price
        elif side < 0 and self.holding == 1:
            self.pnl += price
            self.holding = 0
            self.sell_count += 1
            self.open_position_entry_price = None

    def on_trade(self, ordinal: int, price: Decimal, side: int, duration_us: float,
                 reaction_us: float, final_ordinal: int, prices: list[Decimal]):
        if self.pending is not None and ordinal >= self.pending[1]:
            self.fill(self.pending[0], prices[self.pending[1] - 1])
            self.pending = None
        if side == 0:
            return
        actionable = (side > 0 and self.holding == 0) or (side < 0 and self.holding == 1)
        if not actionable:
            return
        if ordinal >= final_ordinal:
            self.unfilled += 1
        elif duration_us < reaction_us:
            self.fill(side, price)
        else:
            self.pending = (side, ordinal + 1)

    def finish(self, next_dataset_price: float | None):
        if self.pending is not None:
            side, next_ordinal = self.pending
            if next_ordinal <= 0 or next_dataset_price is None:
                self.unfilled += 1
            else:
                self.fill(side, next_dataset_price)
            self.pending = None


def run_race_command(args) -> int:
    dataset = choose_dataset(args.dataset)
    if dataset is None:
        print(f'No dataset matching {args.dataset!r} found.')
        return 2
    fast_version = args.fast_version
    evidence_path = None
    if fast_version is None:
        fast_version, evidence_path = select_fast_version_from_pi_evidence()
        if fast_version is None:
            print(f'Cannot select a default fast version: {evidence_path} Supply --fast-version explicitly.')
            return 2
    if fast_version == args.baseline_version:
        print('Baseline and fast versions must differ.')
        return 2
    base_spec, base_meta, error = replay_metadata_for(args.baseline_version)
    if error:
        print(error)
        return 3
    fast_spec, fast_meta, error = replay_metadata_for(fast_version)
    if error:
        print(error)
        return 3
    if args.hardware_led and not sys.platform.startswith('linux'):
        print('--hardware-led requires Linux on the Pi; refusing to substitute console output.')
        return 2
    facts = dataset_facts(dataset)
    try:
        race_prices = read_prices(dataset)
    except (OSError, ValueError) as exc:
        print(f'Cannot read race trade prices: {exc}')
        return 2
    try:
        run_dir = unique_analysis_run_dir('race', facts['sha256'], args.output_dir)
    except (OSError, FileExistsError) as exc:
        print(f'Cannot create race output directory: {exc}')
        return 2
    # Both children receive one future monotonic epoch after preload so their
    # replay clocks begin together; later contention remains part of race data.
    start_epoch_ns = time.monotonic_ns() + 3_000_000_000
    base_command, _, _ = launch_replay_analysis(base_spec, dataset, args, run_dir, 'baseline',
                                                start_epoch_ns, progress=True)
    fast_command, _, _ = launch_replay_analysis(fast_spec, dataset, args, run_dir, 'fast',
                                                start_epoch_ns, progress=True, hardware=args.hardware_led)
    manifest = {
        'dataset': str(dataset.resolve()), 'dataset_sha256': facts['sha256'],
        'baseline_version': args.baseline_version, 'fast_version': fast_version,
        'fast_version_evidence': benchmark_evidence_record(fast_version, evidence_path), 'speed': args.speed,
        'short': args.short, 'long': args.long, 'limit': args.limit,
        'duration_seconds': args.duration_seconds, 'signal_policy': args.signal_policy,
        'rival_reaction_us': args.rival_reaction_us, 'shared_monotonic_start_ns': start_epoch_ns,
        'start_method': 'both replay processes wait for the same monotonic epoch after preload',
        'hardware_led_fast_process': args.hardware_led,
        'resource_contention_note': 'Concurrent processes compete for CPU/cache/I/O; durations and lag are race-condition measurements, not isolated benchmark timings.',
        'telemetry_note': 'Progress records are emitted for the first trade, each 1000th trade, and every signal after its decision timestamp; processing duration excludes rendering, while telemetry I/O can affect later release lag. Reported max lag is over emitted progress records.',
        'baseline_build_metadata': base_meta, 'fast_build_metadata': fast_meta,
        'commands': {'baseline': base_command, 'fast': fast_command},
        'host': platform.platform(), 'architecture': platform.machine(),
        'datetime_utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
    }
    (run_dir / 'run_manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    command_skew_ns = {}
    messages: queue.Queue = queue.Queue()
    processes = {}
    threads = []
    for name, command in (('baseline', base_command), ('fast', fast_command)):
        command_skew_ns[name] = time.monotonic_ns()
        try:
            proc = subprocess.Popen(command, cwd=str(ROOT), stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE, text=True, bufsize=1)
        except OSError as exc:
            for running in processes.values():
                running.terminate()
            print(f'Cannot launch {name} race process: {exc}; race did not complete.')
            return 3
        processes[name] = proc
        def pump(stream, side_name, channel):
            try:
                for line in stream:
                    messages.put((side_name, channel, line.rstrip('\r\n')))
            finally:
                messages.put((side_name, channel, None))
        for stream, channel in ((proc.stdout, 'stdout'), (proc.stderr, 'stderr')):
            thread = threading.Thread(target=pump, args=(stream, name, channel), daemon=True)
            thread.start()
            threads.append(thread)

    states = {'baseline': RaceAccount(), 'fast': RaceAccount()}
    signal_sequences = {'baseline': [], 'fast': []}
    progress_counts = {'baseline': 0, 'fast': 0}
    max_lag_us = {'baseline': 0.0, 'fast': 0.0}
    return_codes = {}
    processed_summaries = {}
    end_streams = set()
    summary_text = {'baseline': [], 'fast': []}
    live_path = run_dir / 'race_live.csv'
    live_fields = ['version', 'trade', 'replay_speed', 'lag_us', 'buy_count', 'sell_count',
                   'position_btc', 'cash_pnl', 'elapsed_since_start_s']
    with live_path.open('w', newline='', encoding='utf-8') as live_handle:
        writer = csv.DictWriter(live_handle, fieldnames=live_fields)
        writer.writeheader()
        while len(end_streams) < 4:
            try:
                name, channel, line = messages.get(timeout=0.2)
            except queue.Empty:
                for name, proc in processes.items():
                    if proc.poll() is not None:
                        return_codes[name] = proc.returncode
                if len(return_codes) == 2 and len(end_streams) == 4:
                    break
                continue
            if line is None:
                end_streams.add((name, channel))
                continue
            if channel == 'stderr':
                if line:
                    print(f'[{name} stderr] {line}', file=sys.stderr)
                continue
            if line.startswith('RACE|'):
                fields = line.split('|')
                _, ordinal, timestamp, price, signal, duration_ns, lag_ns = fields
                ordinal = int(ordinal)
                price = race_prices[ordinal - 1]
                signal = int(signal)
                duration_us = int(duration_ns) / 1000.0
                lag_us = int(lag_ns) / 1000.0
                progress_counts[name] = ordinal
                max_lag_us[name] = max(max_lag_us[name], lag_us)
                signal_sequences[name].append((ordinal, signal)) if signal else None
                states[name].on_trade(ordinal, price, signal, duration_us,
                                      args.rival_reaction_us, facts['trades'], race_prices)
                should_render = signal != 0 or ordinal % 1000 == 0
                if should_render:
                    snapshot = {
                        'version': base_spec['id'] if name == 'baseline' else fast_spec['id'],
                        'trade': ordinal, 'replay_speed': args.speed, 'lag_us': lag_us,
                        'buy_count': states[name].buy_count, 'sell_count': states[name].sell_count,
                        'position_btc': states[name].holding, 'cash_pnl': states[name].pnl,
                        'elapsed_since_start_s': max(0.0, (time.monotonic_ns() - start_epoch_ns) / 1e9),
                    }
                    writer.writerow(snapshot)
                    live_handle.flush()
                    if not args.log_only:
                        print(f"{name:8s} {snapshot['version']} speed={args.speed}x trade={ordinal} lag={lag_us:.3f}us "
                              f"BUY/SELL={states[name].buy_count}/{states[name].sell_count} "
                              f"pos={states[name].holding}BTC cash_PnL={states[name].pnl:.8f} "
                              f"elapsed={snapshot['elapsed_since_start_s']:.3f}s", flush=True)
            elif '=' in line:
                summary_text[name].append(line)
                if line.startswith('processed_count='):
                    processed_summaries[name] = int(line.split('=', 1)[1])
    for name, proc in processes.items():
        return_codes[name] = proc.wait()
    for thread in threads:
        thread.join(timeout=1)
    for proc in processes.values():
        if proc.stdout is not None:
            proc.stdout.close()
        if proc.stderr is not None:
            proc.stderr.close()
    if any(code != 0 for code in return_codes.values()):
        print(f'Race process failure codes={return_codes}; race did not complete.')
        return 4
    if set(processed_summaries) != {'baseline', 'fast'}:
        print('Race process summaries were missing; race did not complete.')
        return 4
    progress_counts.update(processed_summaries)
    if processed_summaries['baseline'] != processed_summaries['fast']:
        print(f'Race processed-count mismatch: {processed_summaries}; race did not complete.')
        return 4
    if signal_sequences['baseline'] != signal_sequences['fast']:
        print('Race versions produced different strategy signal sequences; race did not complete.')
        return 4
    skew = max(command_skew_ns.values()) - min(command_skew_ns.values())
    results = {}
    for name, account in states.items():
        next_ordinal = account.pending[1] if account.pending is not None else 0
        next_price = race_prices[next_ordinal - 1] if 0 < next_ordinal <= len(race_prices) else None
        account.finish(next_price)
        results[name] = {
            'processed_count': progress_counts[name], 'sampled_max_lag_us': max_lag_us[name],
            'buy_count': account.buy_count, 'sell_count': account.sell_count,
            'position_btc': account.holding,
            'open_position_entry_price': str(account.open_position_entry_price) if account.open_position_entry_price is not None else None,
            'cash_pnl': decimal_text(account.pnl), 'unfilled_orders': account.unfilled,
            'replay_speed': args.speed,
            'elapsed_since_start_s': max(0.0, (time.monotonic_ns() - start_epoch_ns) / 1e9),
        }
    results['start_skew_ns'] = skew
    results['return_codes'] = return_codes
    results['money_lost_fast_minus_baseline'] = decimal_text(states['fast'].pnl - states['baseline'].pnl)
    (run_dir / 'race_summary.json').write_text(json.dumps(results, indent=2) + '\n', encoding='utf-8')
    print(f'Race completed: baseline={progress_counts["baseline"]} trades, fast={progress_counts["fast"]} trades, process-launch skew={skew}ns.')
    print(f'Race logs: {live_path}; summary: {run_dir / "race_summary.json"}; manifest: {run_dir / "run_manifest.json"}')
    return 0


PERF_COUNTERS = ('cycles', 'instructions', 'cache-misses')


def parse_perf_count(value: str) -> int | None:
    """Parse a perf event count without accepting locale-formatted junk."""
    value = value.strip().replace('\u00a0', '').replace(' ', '')
    if not re.fullmatch(r'[+]?(?:\d+)(?:\.0+)?', value):
        return None
    try:
        return int(Decimal(value))
    except (InvalidOperation, ValueError):
        return None


def parse_perf_stat(path: Path):
    """Parse perf -x, output while retaining unavailable/malformed diagnostics."""
    counters = {name: None for name in PERF_COUNTERS}
    notes = []
    try:
        lines = path.read_text(encoding='utf-8', errors='replace').splitlines()
    except OSError as exc:
        return counters, [f'cannot read raw perf file: {exc}']

    diagnostic_terms = ('not supported', 'not counted', 'permission', 'not permitted',
                        'access denied', 'failed to', 'error:', 'no permission')
    event_seen = set()
    for line_number, line in enumerate(lines, 1):
        if not line.strip() or line.strip().startswith('#'):
            continue
        try:
            parts = next(csv.reader([line], skipinitialspace=True))
        except (csv.Error, StopIteration) as exc:
            notes.append(f'line {line_number}: malformed CSV ({exc}): {line}')
            continue
        parts = [part.strip() for part in parts]
        lowered = line.lower()
        if any(term in lowered for term in diagnostic_terms):
            notes.append(f'line {line_number}: perf diagnostic: {line.strip()}')
        if len(parts) <= 2:
            if line.strip() != '[perf process stderr]':
                notes.append(f'line {line_number}: malformed perf row: {line}')
            continue
        raw_event = parts[2]
        event = raw_event.split(':', 1)[0].strip()
        if event not in counters:
            shifted = [(index, value.split(':', 1)[0].strip())
                       for index, value in enumerate(parts) if value.split(':', 1)[0].strip() in counters]
            if shifted:
                index, shifted_event = shifted[0]
                notes.append(f'line {line_number}: malformed perf row for {shifted_event}; '
                             f'event is in CSV field {index + 1}, expected field 3 (check locale/delimiter): {line}')
            continue
        if event in event_seen:
            notes.append(f'line {line_number}: duplicate event {raw_event}')
        event_seen.add(event)
        raw_count = parts[0] if parts else ''
        count = parse_perf_count(raw_count)
        if count is None:
            note = f'{event}: {raw_count or "<empty>"}'
            if note not in notes:
                notes.append(note)
            continue
        counters[event] = count
    return counters, notes


def format_perf_value(value, digits: int = 8) -> str:
    if value is None or value == '':
        return ''
    if isinstance(value, float):
        return f'{value:.{digits}f}'
    return str(value)


def write_perf_summary(out_dir: Path, rows: list[dict], manifest: dict) -> None:
    """Write normalized CSV/Markdown from rows; raw perf files are never touched."""
    fields = ['version_id', 'run', 'host_class', 'governor_performance', 'taskset_used',
              'final_controlled', 'status', 'cycles', 'instructions', 'ipc', 'cache-misses',
              'return_code', 'counter_notes', 'raw_file', 'stdout_file']
    write_csv(out_dir / 'perf_summary.csv', rows, fields)
    table = ['# Perf stat summary', '',
             f'Host: {manifest.get("host_class", "unknown")}; governor performance: '
             f'{manifest.get("governor_performance", False)}; taskset used: '
             f'{manifest.get("taskset_used", False)}; status: {manifest.get("status", "unknown")}.', '',
             'Counts below are normalized from the preserved raw `perf stat -x,` files. '
             'Unavailable counters are blank; they are never represented as zero.', '',
             '| Version | Run | Cycles | Instructions | IPC | Cache misses | Exit | Counter notes |',
             '|---|---:|---:|---:|---:|---:|---:|---|']
    for row in rows:
        table.append('| {version_id} | {run} | {cycles} | {instructions} | {ipc} | '
                     '{cache-misses} | {return_code} | {counter_notes} |'.format(
                         **{**row, 'cycles': format_perf_value(row.get('cycles')),
                            'instructions': format_perf_value(row.get('instructions')),
                            'ipc': format_perf_value(row.get('ipc')),
                            'cache-misses': format_perf_value(row.get('cache-misses')),
                            'counter_notes': str(row.get('counter_notes', '')).replace('|', '\\|')}))
    (out_dir / 'perf_summary.md').write_text('\n'.join(table) + '\n', encoding='utf-8')


def refresh_perf_summary(profile_dir: Path) -> int:
    """Reparse a profile run's preserved raw files and replace only its summaries."""
    if not profile_dir.is_dir():
        print(f'Profile directory does not exist: {profile_dir}')
        return 2
    manifest_path = profile_dir / 'profile_manifest.json'
    try:
        manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
    except (OSError, json.JSONDecodeError) as exc:
        print(f'Cannot read profile manifest {manifest_path}: {exc}')
        return 2
    prior_rows = {}
    summary_path = profile_dir / 'perf_summary.csv'
    if summary_path.is_file():
        with summary_path.open(newline='', encoding='utf-8') as handle:
            for row in csv.DictReader(handle):
                prior_rows[(row.get('version_id'), row.get('run'))] = row
    rows = []
    for raw in sorted(profile_dir.glob('*-run*.perf.txt')):
        match = re.fullmatch(r'(.+)-run(\d+)\.perf\.txt', raw.name)
        if not match:
            continue
        version_id, run_text = match.groups()
        run = int(run_text)
        old = prior_rows.get((version_id, run_text), {})
        counters, notes = parse_perf_stat(raw)
        cycles, instructions = counters['cycles'], counters['instructions']
        ipc = instructions / cycles if instructions is not None and cycles not in (None, 0) else None
        rows.append({
            'version_id': version_id, 'run': run,
            'host_class': old.get('host_class') or manifest.get('host_class', 'unknown'),
            'governor_performance': old.get('governor_performance', manifest.get('governor_performance', False)),
            'taskset_used': old.get('taskset_used', manifest.get('taskset_used', False)),
            'final_controlled': old.get('final_controlled', manifest.get('final_controlled', False)),
            'status': old.get('status') or manifest.get('status', 'unknown'),
            **counters, 'ipc': ipc,
            'return_code': old.get('return_code', ''),
            'counter_notes': '; '.join(notes),
            'raw_file': str(raw.resolve()),
            'stdout_file': old.get('stdout_file', str((profile_dir / f'{version_id}-run{run}.stdout.txt').resolve())),
        })
    if not rows:
        print(f'No raw files matching *-runN.perf.txt in {profile_dir}')
        return 2
    write_perf_summary(profile_dir, rows, manifest)
    print('Updated normalized summaries from preserved raw files:', summary_path.resolve())
    return 0


def build_soa(compiler: str | None = None) -> int:
    """Build only the separately registered v10 Struct-of-Arrays executable."""
    spec = next(item for item in version_specs() if item['id'] == 'v10_manual_running_double_soa_O2')
    return compile_version_spec(spec, compiler)


def asm_kernel(mode: str, repeats: int = 5, platform_label: str | None = None,
               taskset_cpus: str | None = None) -> int:
    """Build/test the portable kernel or measure it beside the C reference."""
    if mode == 'benchmark' and repeats < 5:
        print('Assembly-kernel benchmark requires at least five repeats.')
        return 2
    compiler = find_executable(['gcc', 'gcc.exe', 'clang', 'clang.exe'])
    if compiler is None:
        print('A C compiler (GCC or Clang) is required.')
        return 2
    machine = platform.machine().lower()
    use_asm = sys.platform.startswith('linux') and machine in {'aarch64', 'arm64'}
    if platform_label == 'pi-aarch64-kernel-final' and not use_asm:
        print('Refusing the Pi-final label: native Linux AArch64 assembly execution is required.')
        return 2
    flags = ['-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O3',
             '-Iinclude', '-Iprovided']
    if sys.platform.startswith('linux'):
        flags.extend(platform_build_flags())
    sources = ['asm/aarch64_dotprod_ref.c', 'asm/aarch64_dotprod_bench.c']
    if use_asm:
        flags.append('-DTT_USE_AARCH64_ASM')
        sources.append('asm/aarch64_dotprod.S')
    output = BUILD_ROOT / 'asm-kernel' / ('asm_kernel.exe' if os.name == 'nt' else 'asm_kernel')
    output.parent.mkdir(parents=True, exist_ok=True)
    command = [compiler, *flags, *sources, '-o', str(output)]
    built = subprocess.run(command, cwd=str(ROOT), capture_output=True, text=True, check=False)
    if built.returncode:
        print(built.stderr)
        return built.returncode
    test = subprocess.run([str(output), '--test'], cwd=str(ROOT),
                          capture_output=True, text=True, check=False)
    if test.returncode:
        print(test.stdout, end='')
        print(test.stderr, end='', file=sys.stderr)
        return test.returncode
    print(test.stdout, end='')
    print('Kernel implementation:', 'hand-written AArch64 assembly' if use_asm else
          'portable C fallback (native AArch64 execution pending)')
    if mode == 'test':
        return 0

    label = platform_label or ('pi-aarch64-kernel-final' if use_asm else
                               f'{platform.system().lower()}-{machine}-asm-kernel-smoke')
    run_dir = unique_evidence_dir('assembly-kernel', label)
    prefix, taskset_available = taskset_prefix(taskset_cpus)
    run = subprocess.run(prefix + [str(output), '--benchmark', str(repeats)],
                         cwd=str(ROOT), capture_output=True, text=True, check=False)
    if run.returncode:
        print(run.stdout, end='')
        print(run.stderr, end='', file=sys.stderr)
        return run.returncode
    rows = []
    for line in run.stdout.splitlines():
        if not line or line.startswith('#') or line.startswith('assembly kernel correctness:') or line.startswith('kernel,'):
            continue
        fields = line.split(',')
        if len(fields) != 6:
            continue
        kernel, repeat_text, elements_text, calls_text, elapsed_text, checksum_text = fields
        elapsed, count, calls = int(elapsed_text), int(elements_text), int(calls_text)
        rows.append({'kernel': kernel, 'repeat': int(repeat_text), 'elements': count,
                     'calls': calls, 'elapsed_ns': elapsed,
                     'ns_per_call': elapsed / calls if calls else None,
                     'elements_per_second': (count * calls * 1_000_000_000 / elapsed) if elapsed else None,
                     'checksum': checksum_text})
    expected_rows = repeats * 2
    kernel_name = 'aarch64_asm' if use_asm else 'portable_c_fallback'
    if len(rows) != expected_rows or {row['kernel'] for row in rows} != {'c_reference', kernel_name}:
        print(f'Benchmark output malformed: expected {expected_rows} timing rows, got {len(rows)}.')
        return 1
    write_csv(run_dir / 'raw.csv', rows,
              ['kernel', 'repeat', 'elements', 'calls', 'elapsed_ns', 'ns_per_call',
               'elements_per_second', 'checksum'])
    medians = []
    for kernel in ('c_reference', kernel_name):
        selected = [row for row in rows if row['kernel'] == kernel]
        medians.append({'kernel': kernel, 'repeats': len(selected),
                        'median_ns_per_call': median(row['ns_per_call'] for row in selected),
                        'median_elements_per_second': median(row['elements_per_second'] for row in selected),
                        'checksum': selected[0]['checksum']})
    write_csv(run_dir / 'median.csv', medians,
              ['kernel', 'repeats', 'median_ns_per_call', 'median_elements_per_second', 'checksum'])
    c_median = next(row['median_ns_per_call'] for row in medians if row['kernel'] == 'c_reference')
    asm_median = next(row['median_ns_per_call'] for row in medians if row['kernel'] == kernel_name)
    report = [
        '# AArch64 dot-product kernel comparison', '',
        f'- Execution: {"native AArch64 assembly" if use_asm else "portable C fallback; Pi assembly benchmark pending"}',
        f'- Repeats: {repeats}; each timing contains 128 calls over 262144 uint64_t pairs.',
        '- Inputs were populated at runtime. Correctness tests and a full-input equality gate ran before warm-up and timing.',
        '- Timed sections include only repeated kernel calls and monotonic clock reads; allocation, initialization, printing, and correctness checks are outside.',
        f'- Median time ratio (selected kernel entry / C reference): {asm_median / c_median:.6f}.',
        '- No performance winner is assumed. GCC -O3 may match or beat scalar assembly through scheduling, unrolling, or vectorization.',
        '', '| Kernel | Median ns/call | Median elements/s | Checksum |',
        '|---|---:|---:|---:|',
    ]
    for row in medians:
        report.append(f'| {row["kernel"]} | {row["median_ns_per_call"]:.2f} | '
                      f'{row["median_elements_per_second"]:.2f} | {row["checksum"]} |')
    (run_dir / 'comparison.md').write_text('\n'.join(report) + '\n', encoding='utf-8')
    max_rate = max(row['median_elements_per_second'] for row in medians) or 1
    bars = []
    for index, row in enumerate(medians):
        width = 500 * row['median_elements_per_second'] / max_rate
        y = 65 + index * 65
        bars.append(f'<text x="10" y="{y + 20}">{row["kernel"]}</text>'
                    f'<rect x="180" y="{y}" width="{width:.2f}" height="32" fill="#3572b0"/>'
                    f'<text x="{190 + width:.2f}" y="{y + 20}">{row["median_elements_per_second"]:.0f} elem/s</text>')
    svg = ('<svg xmlns="http://www.w3.org/2000/svg" width="820" height="205">'
           '<text x="10" y="25" font-size="16">Median throughput (higher is faster)</text>' +
           ''.join(bars) + '</svg>\n')
    (run_dir / 'throughput.svg').write_text(svg, encoding='utf-8')
    source_paths = [ROOT / name for name in sources]
    source_paths.append(ROOT / 'include' / 'aarch64_dotprod.h')
    compiler_version = subprocess.run([compiler, '--version'], capture_output=True,
                                      text=True, check=False).stdout.splitlines()
    governors = cpu_governors()
    manifest = {
        'platform_label': label, 'mode': mode, 'implementation': 'aarch64_asm' if use_asm else 'portable_c_fallback',
        'native_aarch64_execution': use_asm, 'host_os': platform.platform(),
        'architecture': platform.machine(), 'compiler_path': compiler,
        'compiler_version': compiler_version[0] if compiler_version else 'unknown',
        'flags': flags, 'command': command, 'source_sha256': {
            str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest() for path in source_paths},
        'repeats': repeats, 'elements_per_call': 262144, 'calls_per_repeat': 128,
        'cpu_governors': governors,
        'governor_performance': bool(governors) and all(value == 'performance' for value in governors.values()),
        'taskset_requested_cpus': taskset_cpus,
        'taskset_used': taskset_available, 'datetime_utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
        'benchmark_stdout': run.stdout, 'status': 'Pi AArch64 kernel benchmark' if use_asm else 'host fallback only; Pi benchmark pending',
    }
    (run_dir / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    print('Assembly-kernel evidence:', run_dir.resolve())
    print(run.stdout.splitlines()[0] if run.stdout else '')
    return 0


def profile_versions(dataset_name: str, version_ids: list[str], repeats: int,
                     taskset_cpus: str | None, platform_label: str | None) -> int:
    dataset = choose_dataset(dataset_name)
    specs_by_id = {item['id']: item for item in version_specs()}
    unknown = [version for version in version_ids if version not in specs_by_id]
    if dataset is None or unknown:
        print('Choose registered versions and an existing dataset.', ', '.join(unknown))
        return 2
    if not on_pi():
        print('Pi perf profiling is pending: run this command on the Linux AArch64 Pi.')
        return 2
    perf = find_executable(['perf'])
    if perf is None:
        print('Linux perf is missing. Check with `command -v perf`; install the distribution perf package on the Pi.')
        return 2
    label = platform_label or 'pi-perf'
    out_dir = unique_evidence_dir('profile', label)
    prefix, taskset_available = taskset_prefix(taskset_cpus)
    governors = cpu_governors()
    governor_ok = bool(governors) and all(value == 'performance' for value in governors.values())
    taskset_used = bool(taskset_cpus and taskset_available)
    manifest = {'platform_label': label, 'platform': platform.platform(), 'architecture': platform.machine(),
                'host_class': 'Pi' if on_pi() else 'laptop/host',
                'datetime_utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
                'dataset_path': str(dataset.resolve()), 'dataset_facts': dataset_facts(dataset),
                'versions': version_ids, 'repeats': repeats, 'cpu_governors': governors,
                'taskset_requested_cpus': taskset_cpus, 'taskset_used': taskset_used,
                'governor_performance': governor_ok,
                'final_controlled': bool(on_pi() and governor_ok and taskset_used),
                'perf_path': perf,
                'perf_locale': 'C',
                'status': 'CONTROLLED' if on_pi() and governor_ok and taskset_used else 'PRELIMINARY'}
    manifest['build_metadata'] = {}
    for version_id in version_ids:
        sidecar = specs_by_id[version_id]['metadata']
        if sidecar.is_file():
            manifest['build_metadata'][version_id] = json.loads(sidecar.read_text(encoding='utf-8'))
    (out_dir / 'profile_manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    rows = []
    for version_id in version_ids:
        spec = specs_by_id[version_id]
        if not spec['exe'].is_file() or not spec['metadata'].is_file():
            print(f'Missing build for {version_id}; run project.py build --profile release.')
            return 3
        for run in range(1, repeats + 1):
            raw = out_dir / f'{version_id}-run{run}.perf.txt'
            stdout_path = out_dir / f'{version_id}-run{run}.stdout.txt'
            command = prefix + [perf, 'stat', '-x,', '-e', 'cycles,instructions,cache-misses', '-o', str(raw), '--',
                                str(spec['exe'].resolve()), str(dataset.resolve()), '30', '60', '--signals-only']
            profile_env = os.environ.copy()
            profile_env['LC_ALL'] = 'C'
            result = subprocess.run(command, cwd=str(ROOT), capture_output=True, text=True,
                                    check=False, env=profile_env)
            stdout_path.write_text(result.stdout, encoding='utf-8')
            if result.stderr:
                with raw.open('a', encoding='utf-8') as handle:
                    handle.write('\n[perf process stderr]\n' + result.stderr)
            counters, notes = parse_perf_stat(raw)
            ipc = (counters['instructions'] / counters['cycles']
                   if counters['instructions'] is not None and counters['cycles'] not in (None, 0) else None)
            rows.append({'version_id': version_id, 'run': run, **counters, 'ipc': ipc,
                         'host_class': manifest['host_class'],
                         'governor_performance': governor_ok,
                         'taskset_used': taskset_used,
                         'final_controlled': manifest['final_controlled'],
                         'status': manifest['status'],
                         'return_code': result.returncode, 'counter_notes': '; '.join(notes),
                         'raw_file': str(raw.resolve()), 'stdout_file': str(stdout_path.resolve())})
    write_perf_summary(out_dir, rows, manifest)
    print('Profile evidence:', out_dir.resolve())
    return 0 if all(row['return_code'] == 0 for row in rows) else 1


def profile_version(dataset_name: str, version_id: str, repeats: int,
                    taskset_cpus: str | None = None, platform_label: str | None = None) -> int:
    return profile_versions(dataset_name, [version_id], repeats, taskset_cpus, platform_label)


def classify_assembly_instruction(mnemonic: str, operands: str) -> tuple[str, str, bool]:
    """Return a heuristic category, short explanation, and review-needed flag."""
    op = mnemonic.lower()
    args = operands.lower()
    if op in {'ret', 'eret'} or op in {'bx'} and args.strip() in {'lr', 'x30'}:
        return 'Return', 'returns from the current function', False
    if op in {'bl', 'blr', 'call', 'callq'} or op.startswith('call.'):
        return 'Function call', 'calls another function or indirect call target', False
    stack_register = bool(re.search(r'(^|[\s,])sp([,\s!]|$)', args))
    if op in {'stp', 'ldp', 'str', 'stur', 'stnp', 'push', 'pushq', 'pop', 'popq',
              'mov', 'sub', 'add'} and (
            '[sp' in args or stack_register):
        return 'Stack setup / restore', 'appears to save/restore registers or adjust/copy the stack pointer', True
    if op in {'ldp', 'ldr', 'ldur', 'ldnp', 'ld1', 'ld2', 'ld3', 'ld4', 'pop', 'popq'}:
        return 'Loads', 'loads data from memory or a register stack slot', False
    if op in {'stp', 'str', 'stur', 'stnp', 'st1', 'st2', 'st3', 'st4', 'push', 'pushq'}:
        return 'Stores', 'stores data to memory', False
    if op.startswith(('fadd', 'fsub', 'fmul', 'fdiv', 'fsqrt', 'fmla', 'fmsub', 'fcmp', 'fcvt', 'scvtf', 'ucvtf', 'fcvtz')):
        return 'Floating-point work', 'performs floating-point arithmetic, comparison, or conversion', False
    if op.startswith(('b.', 'cbz', 'cbnz', 'tbz', 'tbnz')) or op in {'b', 'br', 'jmp', 'je', 'jne', 'ja', 'jb', 'jg', 'jl', 'jge', 'jle', 'jc', 'jnc', 'test', 'cmp'}:
        if op in {'cmp', 'test'}:
            return 'Arithmetic / compare', 'sets condition state for a later conditional branch', False
        return 'Branches', 'changes control flow, often for a conditional or loop edge', False
    if op in {'sub', 'add', 'mov', 'movz', 'movk', 'adrp', 'adr', 'mul', 'madd', 'msub',
              'sdiv', 'udiv', 'and', 'orr', 'eor', 'lsl', 'lsr', 'asr', 'neg', 'csel',
              'cmn', 'inc', 'dec', 'lea', 'xor', 'imul', 'addq', 'subq'} or op.startswith(('add.', 'sub.', 'mul.', 'and.', 'orr.', 'eor.')):
        return 'Integer / address arithmetic', 'updates values, addresses, or condition operands', False
    if op.startswith(('vadd', 'vsub', 'vmul', 'vdiv', 'vld', 'vst', 'fmov', 'fcsel', 'movi', 'umov', 'ins', 'dup')):
        return 'Vector / floating-point register work', 'uses vector or floating-point registers; exact semantics need inspection', True
    return 'Review-needed', 'instruction was not confidently classified by the annotator', True


def annotate_assembly(assembly_path: Path, version_id: str) -> int:
    """Create a reproducible, heuristic annotation beside a real assembly manifest."""
    assembly_path = assembly_path.expanduser().resolve()
    if not assembly_path.is_file() or assembly_path.suffix.lower() != '.s':
        print(f'Assembly file must be an existing .s file: {assembly_path}')
        return 2
    manifest_path = next((parent / 'assembly_manifest.json'
                          for parent in (assembly_path.parent, assembly_path.parent.parent,
                                         assembly_path.parent.parent.parent)
                          if (parent / 'assembly_manifest.json').is_file()), None)
    if manifest_path is None:
        print('Could not find assembly_manifest.json beside the assembly run directory.')
        return 2
    try:
        manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
    except (OSError, json.JSONDecodeError) as exc:
        print(f'Cannot read assembly manifest {manifest_path}: {exc}')
        return 2
    if manifest.get('version_id') != version_id:
        print(f'Manifest version is {manifest.get("version_id")!r}, not {version_id!r}.')
        return 2
    manifest_output = Path(manifest.get('output_file', '')).expanduser().resolve()
    if manifest_output != assembly_path:
        print(f'Manifest output path does not match supplied assembly: {manifest_output}')
        return 2
    actual_hash = hashlib.sha256(assembly_path.read_bytes()).hexdigest()
    if manifest.get('output_sha256') != actual_hash:
        print('Assembly SHA256 does not match the manifest; refusing to annotate a changed file.')
        return 2

    lines = assembly_path.read_text(encoding='utf-8', errors='replace').splitlines()
    function_names = set()
    for line in lines:
        type_match = re.match(r'^\s*\.type\s+([^,\s]+)\s*,.*(?:@|%)function\b', line)
        if type_match:
            function_names.add(type_match.group(1))
    if not function_names:
        function_names.update(re.findall(r'^([A-Za-z_.$][\w.$]*):\s*$', '\n'.join(lines), re.M))
        function_names = {name for name in function_names if not name.startswith(('.L', '$'))}

    functions = {}
    active = None
    for line_number, line in enumerate(lines, 1):
        label_match = re.match(r'^([A-Za-z_.$][\w.$]*):\s*$', line)
        if label_match:
            label = label_match.group(1)
            if label in function_names:
                active = label
                functions.setdefault(active, [])
            continue
        if active is None:
            continue
        instruction_match = re.match(r'^\s*([A-Za-z][A-Za-z0-9_.]*)(?:\s+(.*?))?\s*$', line)
        if not instruction_match or line.lstrip().startswith('.'):
            continue
        mnemonic, operands = instruction_match.groups()
        operands = operands or ''
        category, explanation, review = classify_assembly_instruction(mnemonic, operands)
        functions[active].append({'line': line_number, 'text': line.strip(),
                                  'category': category, 'explanation': explanation,
                                  'review': review})

    observed_symbols = []
    for line_number, line in enumerate(lines, 1):
        if re.search(r'\b(?:bl|blr|callq?|call\.[A-Za-z0-9_.]+)\b', line):
            observed_symbols.append((line_number, line.strip()))
    all_labels = [match.group(1) for match in
                  re.finditer(r'^([A-Za-z_.$][\w.$]*):\s*$', '\n'.join(lines), re.M)]

    report = [
        '# Automatic assembly annotation', '',
        '> Generated from the actual assembly text. Categories and explanations are heuristic; '
        'they have not been manually reviewed. Check every interpretation against the source, '
        'compiler, and target ABI.', '',
        f'- Version: `{version_id}`',
        f'- Assembly: `{assembly_path}`',
        f'- SHA256: `{actual_hash}`',
        f'- Manifest: `{manifest_path}`',
        f'- Compiler: `{manifest.get("compiler_path", "unknown")}` '
        f'({manifest.get("compiler_version", "unknown")})',
        f'- Flags: `{" ".join(manifest.get("flags", []))}`',
        f'- Host / architecture: `{manifest.get("host_os", "unknown")}` / '
        f'`{manifest.get("architecture", "unknown")}`', '',
        '## Functions and labels', '',
        'Functions below are recognized from compiler `.type` declarations (or non-local labels '
        'when those declarations are absent). Instruction classification is automatic.', '',
    ]
    if not functions:
        report.append('No function labels with recognizable instructions were found; review the file format manually.')
    for name, instructions in functions.items():
        report.extend([f'### `{name}`', '', f'Instruction lines recognized: {len(instructions)}.', ''])
        categories = {}
        for ins in instructions:
            categories.setdefault(ins['category'], []).append(ins)
        for category, entries in categories.items():
            report.extend([f'**{category}** ({len(entries)} recognized):'])
            for ins in entries[:8]:
                marker = ' **[review-needed]**' if ins['review'] else ''
                report.append(f'- Line {ins["line"]}: `{ins["text"]}` — {ins["explanation"]}.{marker}')
            if len(entries) > 8:
                report.append(f'- … {len(entries) - 8} additional matching instruction line(s) omitted.')
            report.append('')
    report.extend(['## Replay and timing references found in this listing', '',
                   'Entries below quote actual call lines or labels from this file. A missing symbol is not inferred from source code.', ''])
    timing_observed = [(number, text) for number, text in observed_symbols
                       if re.search(r'clock_gettime|nanosleep|clock_nanosleep', text)]
    replay_observed = [(number, text) for number, text in observed_symbols
                       if re.search(r'replay|signal|latency|trade|indicator', text, re.I)]
    report.append('Timing calls:')
    if timing_observed:
        for number, text in timing_observed:
            report.append(f'- Line {number}: {text} — libc timing/wait call; confirm arguments and units from surrounding instructions. [REVIEW NEEDED]')
    else:
        report.append('- No clock_gettime, nanosleep, or clock_nanosleep call line was observed.')
    report.extend(['', 'Relevant replay/signal/latency labels or calls:'])
    relevant_labels = sorted(set(name for name in all_labels
                                 if re.search(r'replay|signal|latency|trade|indicator', name, re.I)))
    if relevant_labels:
        report.extend(f'- Label {name} (actual assembly label).' for name in relevant_labels)
    if replay_observed:
        report.extend(f'- Line {number}: {text} (actual call line).' for number, text in replay_observed)
    if not relevant_labels and not replay_observed:
        report.append('- No matching label or call was observed; inspect the full listing and source mapping.')
    report.append('')
    report.extend([
        '## Code and report annotation checklist', '',
        '- [ ] Timing helper: trace the clock source, units, conversion, and call overhead.',
        '- [ ] Replay scheduling: identify deadline calculation, wait/sleep path, and late-trade handling.',
        '- [ ] Signal decision path: follow input through indicator updates and BUY/SELL decision branches.',
        '- [ ] Latency capture: identify start/end timestamp reads and stored processing/release latency fields.',
        '- [ ] Relevant loops: mark dataset/replay loops and their exit conditions; distinguish loop branches from error branches.',
        '- [ ] `-O2` layout: compare with the matching `-O0` build; optimization can inline helpers, fold constants, '
        'remove redundant loads/stores, schedule instructions, and change register/stack usage. Confirm each change in both listings.',
        '',
        '## Review-needed instructions', '',
        'Any instruction marked `[review-needed]` needs a human to confirm its exact semantics and role. '
        'An unmarked category is still an automatic classification, not proof of manual review.', '',
    ])
    annotation_path = manifest_path.parent / 'annotation.md'
    annotation_path.write_text('\n'.join(report), encoding='utf-8')
    print('Annotation:', annotation_path.resolve())
    print('Assembly preserved:', assembly_path)
    return 0


def generate_assembly(version_id: str, platform_label: str | None = None) -> int:
    """Compile actual assembly from the selected registered source and flags."""
    replay = version_id == 'replay-engine'
    if replay:
        spec = next(item for item in replay_specs() if item['id'] == 'v1_baseline_strtok_running_double_heap_O2')
        source = 'src/replay_engine.c'
        inputs = spec['input_files']
        flags = list(spec['flags'])
    else:
        spec = next((item for item in version_specs() if item['id'] == version_id), None)
        if spec is None:
            print('Unknown assembly version:', version_id)
            return 2
        source = spec['source']
        inputs = spec.get('input_files', [source])
        flags = list(spec['flags'])
    compiler = find_executable(['gcc', 'gcc.exe']) or 'gcc'
    label = platform_label or f'{platform.system().lower()}-{platform.machine().lower()}'
    out_dir = unique_evidence_dir('assembly', label)
    assembly_dir = out_dir / 'assembly'
    assembly_dir.mkdir()
    stem = 'replay_engine' if replay else version_id
    output = assembly_dir / f'{stem}.s'
    command = [compiler] + flags + ['-S', source, '-o', str(output)]
    result = subprocess.run(command, cwd=str(ROOT), capture_output=True, text=True, check=False)
    if result.returncode != 0:
        print(result.stderr)
        return result.returncode
    if not output.is_file() or output.stat().st_size == 0:
        print('Compiler reported success but did not create a non-empty assembly file.')
        return 1
    if on_pi() and platform.machine().lower() not in {'aarch64', 'arm64'}:
        print('Pi assembly must target AArch64; refusing to label this output as Pi assembly.')
        return 2

    primary_source = ROOT / source
    source_hashes = {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in inputs}
    compiler_version = subprocess.run([compiler, '--version'], capture_output=True, text=True, check=False).stdout.splitlines()
    manifest = {
        'version_id': version_id,
        'command': command,
        'compiler_path': compiler,
        'compiler_version': compiler_version[0] if compiler_version else 'unknown',
        'source_path': str(primary_source.resolve()),
        'source_sha256': hashlib.sha256(primary_source.read_bytes()).hexdigest(),
        'source_hashes': source_hashes,
        'flags': flags,
        'host_os': platform.platform(),
        'architecture': platform.machine(),
        'timestamp_utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
        'output_file': str(output.resolve()),
        'output_sha256': hashlib.sha256(output.read_bytes()).hexdigest(),
        'is_pi': on_pi(),
        'status': 'Pi AArch64 assembly generated' if on_pi() else 'host assembly only; Pi AArch64 output pending',
    }
    manifest_path = out_dir / 'assembly_manifest.json'
    manifest_path.write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    labels = sorted(set(re.findall(r'^([A-Za-z_.$][\w.$]*):$', output.read_text(encoding='utf-8', errors='replace'), re.M)))
    inspection = f'rg -n "^[A-Za-z_.$].*:|\\b(ld|ldr|str|stur|add|sub|mul|cmp|b\\w*|bl)\\b" "{output}"'
    annotation = (
        '# Assembly annotation template\n\n'
        f'Generated file: `{output.name}`. Review this output before filling annotations; this template is not a completed analysis.\n\n'
        f'Inspect instructions with: `{inspection}`\n\nFunction labels found:\n'
        + ''.join(f'- `{label}`: review loads, stores, arithmetic, branches, calls, and register use.\n' for label in labels)
    )
    (out_dir / 'annotation_template.md').write_text(annotation, encoding='utf-8')
    (out_dir / 'inspection_command.txt').write_text(inspection + '\n', encoding='utf-8')
    print('Assembly:', output.resolve())
    print('Manifest:', manifest_path.resolve())
    print('Inspection:', inspection)
    if not on_pi():
        print('This is host assembly; generate AArch64 evidence on the Pi.')
    return 0

def generate_signal_plot(dataset_name: str) -> int:
    dataset = choose_dataset(dataset_name)
    if dataset is None:
        print('No dataset found for plot-signals.')
        return 2
    gnuplot = locate_gnuplot()
    plot_dir = ROOT / 'plots'
    plot_dir.mkdir(exist_ok=True)
    rows = []
    with open(dataset, newline='') as handle:
        reader = csv.reader(handle)
        for row in reader:
            if not row or row[0].strip().lower() == 'trade_id':
                continue
            if len(row) != 7:
                continue
            try:
                price = float(row[1])
                qty = float(row[2])
                ts = int(row[4])
                try:
                    is_buy = 1 if row[5].strip().lower() == 'false' else 0
                except Exception:
                    is_buy = 0
                rows.append((price, qty, ts, is_buy))
            except Exception:
                continue
    if len(rows) < 2500:
        print('Dataset too small to generate a regional signals plot; plotting skipped.')
        return 0
    short = 30
    long = 60
    short_window = []
    long_window = []
    output = []
    previous_diff = 0.0
    diff_ready = False
    signal_count = 0
    for idx, (price, qty, ts, is_buy) in enumerate(rows):
        short_window.append(price)
        long_window.append(price)
        if len(short_window) > short:
            short_window.pop(0)
        if len(long_window) > long:
            long_window.pop(0)
        if len(short_window) == short and len(long_window) == long:
            short_sma = sum(short_window) / short
            long_sma = sum(long_window) / long
            current_diff = short_sma - long_sma
            if diff_ready:
                if previous_diff <= 0.0 and current_diff > 0.0:
                    signal_count += 1
                    output.append((idx + 1, price, short_sma, long_sma, 1))
                elif previous_diff >= 0.0 and current_diff < 0.0:
                    signal_count += 1
                    output.append((idx + 1, price, short_sma, long_sma, -1))
            previous_diff = current_diff
            diff_ready = True
    signal_window = [row for row in output if row[0] >= 1500 and row[0] <= 3500]
    if not signal_window:
        print('No qualifying BUY/SELL region found in the development data; plot generation skipped.')
        return 0
    start_idx = max(1, min(x[0] for x in signal_window) - 200)
    end_idx = min(len(rows), max(x[0] for x in signal_window) + 200)
    signal_dat = plot_dir / 'signals.dat'
    with open(signal_dat, 'w', encoding='utf-8') as handle:
        handle.write('# ordinal price short_sma long_sma signal\n')
        for idx in range(start_idx - 1, end_idx):
            price = rows[idx][0]
            short_window_local = rows[max(0, idx - short + 1):idx + 1]
            long_window_local = rows[max(0, idx - long + 1):idx + 1]
            if len(short_window_local) >= short and len(long_window_local) >= long:
                short_sma = sum(p[0] for p in short_window_local[-short:]) / short
                long_sma = sum(p[0] for p in long_window_local[-long:]) / long
                signal = 0
                for row in output:
                    if row[0] == idx + 1:
                        signal = row[4]
                        break
                handle.write(f'{idx + 1} {price} {short_sma} {long_sma} {signal}\n')
    if gnuplot:
        gnuplot_cmd = (
            "set terminal pngcairo size 1200,700 font 'Verdana,10'; "
            "set output 'plots/signals.png'; "
            "set title 'Tick-to-Trade signals'; "
            "set xlabel 'ordinal trade'; "
            "set ylabel 'price'; "
            "plot 'plots/signals.dat' using 1:2 with lines title 'price', "
            "'plots/signals.dat' using 1:3 with lines title 'short SMA', "
            "'plots/signals.dat' using 1:4 with lines title 'long SMA', "
            "'plots/signals.dat' using 1:($5==1 ? $2 : 1/0) with points pt 7 ps 1.2 lc rgb 'green' title 'BUY', "
            "'plots/signals.dat' using 1:($5==-1 ? $2 : 1/0) with points pt 7 ps 1.2 lc rgb 'red' title 'SELL'; "
        )
        proc = subprocess.run([gnuplot, '-e', gnuplot_cmd], cwd=str(ROOT), check=False, capture_output=True, text=True)
        if proc.returncode != 0:
            print(proc.stderr)
            return proc.returncode
        print(f'Generated signal plot at {plot_dir / "signals.png"}')
    print(f'Generated signal data at {signal_dat}')
    return 0


def plot_signals(dataset_name: str) -> int:
    return generate_signal_plot(dataset_name)


def report() -> int:
    print('Project status: core C baseline, v2-v10 offline variants, SoA, replay, and dataset-aware verification are implemented; the new Pi SoA/AoS controlled comparison remains pending.')
    return 0


def readiness() -> int:
    """Report repository/submission readiness without writing or running builds."""
    required_files = [
        'Makefile', 'README.md', '.gitignore', 'scripts/project.py',
        'src/stage0.c', 'src/market_baseline.c', 'src/market_variant.c',
        'src/market_soa.c', 'src/replay_engine.c', 'src/replay_support.c',
        'include/market.h', 'include/replay_support.h',
        'asm/aarch64_dotprod.S', 'asm/aarch64_dotprod_ref.c',
        'asm/aarch64_dotprod_bench.c', 'include/aarch64_dotprod.h',
        'provided/timing.h', 'provided/led.h', 'provided/trade_io.c',
        'provided/trade_io.h', 'provided/test_helpers.c',
        'tests/test_market.py', 'tests/test_performance_workflow.py',
        'tests/test_replay.py', 'tests/test_rival.py', 'tests/test_asm_kernel.py',
        'tests/test_readiness.py',
        'tests/test_replay_support.c',
        'docs/codebase_map.md', 'docs/performance_methodology.md',
        'docs/requirements_matrix.md', 'docs/stage_status.md',
        'docs/version_manifest.md', 'docs/pi_handoff.md',
        'docs/numeric_policy.md', 'docs/code_walkthrough.md',
        'docs/ai_assistance.md', 'docs/evidence_inventory.md',
        'docs/submission_checklist.md', 'docs/report_evidence_map.md',
    ]
    checks: list[tuple[str, str, str, bool]] = []

    def add(name: str, status: str, detail: str, fatal: bool = False) -> None:
        checks.append((name, status, detail, fatal))

    missing = [rel for rel in required_files if not (ROOT / rel).is_file()]
    add('Required source, build, and documentation files',
        'MISSING' if missing else 'PASS',
        ', '.join(missing) if missing else f'{len(required_files)} required paths exist',
        bool(missing))

    if (ROOT / 'tests').is_dir():
        discovered = sorted((ROOT / 'tests').glob('test_*.py'))
        add('Python test discovery', 'PASS' if discovered else 'MISSING',
            f'{len(discovered)} test_*.py files under tests/' if discovered else 'No discoverable test_*.py files',
            not bool(discovered))
    else:
        add('Python test discovery', 'MISSING', 'tests/ directory is absent', True)

    version_ids = [spec['id'] for spec in version_specs()]
    expected_version_ids = 11
    add('Visible version registry', 'PASS' if len(version_ids) == expected_version_ids else 'MISSING',
        f'{len(version_ids)} registered offline versions; expected v0-v10',
        len(version_ids) != expected_version_ids)
    add('Makefile and README', 'PASS' if (ROOT / 'Makefile').is_file() and (ROOT / 'README.md').is_file()
        else 'MISSING', 'required build and usage entry points',
        not ((ROOT / 'Makefile').is_file() and (ROOT / 'README.md').is_file()))

    build_specs = version_specs() + replay_specs()
    built_ok = 0
    stale = []
    for spec in build_specs:
        exe, meta_path = spec['exe'], spec['metadata']
        if not exe.is_file() or not meta_path.is_file():
            stale.append(f"{spec['id']}: binary or metadata absent")
            continue
        try:
            metadata = json.loads(meta_path.read_text(encoding='utf-8'))
        except (OSError, json.JSONDecodeError):
            stale.append(f"{spec['id']}: metadata unreadable")
            continue
        if build_metadata_matches(spec, metadata):
            built_ok += 1
        else:
            stale.append(f"{spec['id']}: metadata/source mismatch")
    stage0_bins = [BUILD_ROOT / 'debug' / ('stage0.exe' if os.name == 'nt' else 'stage0'),
                   BUILD_ROOT / 'release' / ('stage0.exe' if os.name == 'nt' else 'stage0')]
    stage0_missing = [str(path.relative_to(ROOT)) for path in stage0_bins if not path.is_file()]
    if stage0_missing:
        stale.extend(f'{path}: build artifact absent' for path in stage0_missing)
    add('Existing build artifacts (read-only metadata check)',
        'PASS' if not stale else 'WARNING',
        f'{built_ok}/{len(build_specs)} registered binaries match metadata' +
        (('; ' + '; '.join(stale[:8])) if stale else '') +
        ('; rebuild with make all' if stale else ''),
        False)

    pi_root = ROOT / 'results' / 'pi'
    pi_files = [path for path in pi_root.rglob('*') if path.is_file() and
                path.name.lower() != 'readme.md' and
                not path.name.lower().startswith('readiness-final-')] if pi_root.is_dir() else []
    add('Pi evidence tree', 'PI EVIDENCE REQUIRED',
        (f'{len(pi_files)} non-readme file(s) are visible under results/pi/; inspect manifests and raw files '
         'before marking any requirement PI VERIFIED') if pi_files else
        'No Pi run evidence is locally visible; expected historical results cannot be verified here')

    expected_pi = [
        ('Stage 0 timing and LED', 'results/pi/stage0-runs/'),
        ('Controlled performance and memory ladder', 'results/pi/performance-runs/pi-final-performance/'),
        ('Raw perf and normalized summary', 'results/pi/profile-runs/pi-final-performance/'),
        ('Replay, latency, rival, and race', 'results/pi/replay-runs/; results/pi/rival-runs/; results/pi/race-runs/'),
        ('Generated AArch64 assembly and annotation', 'results/pi/assembly-runs/pi-final-performance/'),
        ('SoA and Python/NumPy comparison', 'results/pi/performance-runs/pi-soa-aos-final/; results/pi/python-comparisons-runs/'),
        ('Hand-written AArch64 kernel benchmark', 'results/pi/assembly-kernel-runs/pi-aarch64-kernel-final/'),
    ]
    for label, path_text in expected_pi:
        path_parts = [part.strip() for part in path_text.split(';')]
        matches = []
        for part in path_parts:
            candidate = ROOT / part.rstrip('/')
            if candidate.is_dir():
                matches.extend(path for path in candidate.rglob('*') if path.is_file())
            elif candidate.is_file():
                matches.append(candidate)
        visible = len([path for path in matches if path.name.lower() != 'readme.md'])
        add(label, 'PI EVIDENCE REQUIRED',
            f'expected {path_text}; {visible} candidate file(s) visible but require manifest/raw-file inspection'
            if visible else f'expected {path_text}; no local candidate evidence files found')

    pi_annotations = [path for path in pi_files if path.name == 'annotation.md']
    add('Actual Pi assembly annotation',
        'PASS' if pi_annotations else 'PI EVIDENCE REQUIRED',
        ', '.join(str(path.relative_to(ROOT)) for path in pi_annotations[:3])
        if pi_annotations else
        'No annotation.md is visible under results/pi/; laptop listings do not substitute')

    local_annotations = list((ROOT / 'results' / 'laptop').rglob('annotation.md')) \
        if (ROOT / 'results' / 'laptop').is_dir() else []
    add('Local annotation workflow artifacts', 'PASS' if local_annotations else 'WARNING',
        f'{len(local_annotations)} laptop annotation file(s); not Pi assembly evidence')

    root_suspicious = []
    root_canonical = {'.c': 'src/', '.h': 'include/', '.py': 'scripts/', '.S': 'asm/'}
    for path in ROOT.iterdir():
        if path.is_file() and path.suffix in root_canonical:
            root_suspicious.append(f'{path.name} (expected under {root_canonical[path.suffix]})')
    root_signal = ROOT / 'signals.png'
    plot_signal = ROOT / 'plots' / 'signals.png'
    if root_signal.is_file() and plot_signal.is_file():
        root_suspicious.append(
            'signals.png (0-byte root duplicate; canonical plot is plots/signals.png; verify before manual removal)')
    add('Root-level source/layout audit', 'WARNING' if root_suspicious else 'PASS',
        '; '.join(root_suspicious) if root_suspicious else
        'no root-level .c/.h/.py/.S duplicates detected')

    build_dir = ROOT / 'build'
    add('Generated build directory', 'WARNING' if build_dir.exists() else 'PASS',
        'build/ exists and is ignored; exclude generated binaries from the manual submission copy'
        if build_dir.exists() else 'build/ is absent')
    caches = [p for p in ROOT.rglob('__pycache__') if p.is_dir() and
              not any(part in {'build', '.git', 'results'} for part in p.parts)]
    pycs = [p for p in ROOT.rglob('*.pyc') if
            not any(part in {'build', '.git', 'results'} for part in p.parts)]
    add('Python caches', 'WARNING' if caches or pycs else 'PASS',
        f'{len(caches)} __pycache__ director(ies), {len(pycs)} .pyc file(s); exclude from package'
        if caches or pycs else 'no Python caches visible')
    temp_logs = [p for ext in ('*.tmp', '*.bak', '*.log') for p in ROOT.glob(ext)]
    add('Root temporary logs', 'WARNING' if temp_logs else 'PASS',
        ', '.join(p.name for p in temp_logs) if temp_logs else
        'no root-level temporary log/backup files; result logs remain evidence')
    sensitive = []
    for path in ROOT.rglob('*'):
        if not path.is_file() or any(part in {'.git', 'build', '__pycache__'} for part in path.parts):
            continue
        name = path.name.lower()
        if name in {'.env', 'id_rsa', 'id_ed25519'} or name.endswith(('.pem', '.p12', '.pfx', '.key')) \
                or 'credential' in name or 'secret' in name:
            if path.relative_to(ROOT).as_posix() != 'config/example.env':
                sensitive.append(path.relative_to(ROOT).as_posix())
    add('Sensitive/unsuitable filename scan', 'WARNING' if sensitive else 'PASS',
        ', '.join(sensitive[:12]) if sensitive else
        'no likely secret/key filenames detected; review personal names and large inputs manually')

    manual_items = [
        'PMCOAReportGroup05.pdf and team-written analysis',
        'approximately 15-minute group presentation video with cameras/member contributions',
        'approximately 5-minute laptop/Pi race and LED demonstration video',
        'signed declaration / Own Work forms for each member',
        'separate peer evaluation form',
        'six dated and member-signed predict-measure-explain entries',
        'final zip assembly, clean temporary extraction, and Pi make/test review',
    ]
    for item in manual_items:
        add(item, 'MANUAL ITEM', 'external/team action; not claimed complete from repository files')

    print('TICK-TO-TRADE READINESS (read-only; no build, benchmark, or evidence changes)')
    print(f'Workspace: {ROOT}')
    print(f'Platform: {platform.system()} / {platform.machine()}')
    for name, status, detail, _fatal in checks:
        print(f'[{status}] {name}: {detail}')
    failures = [name for name, status, _detail, fatal in checks if fatal and status == 'MISSING']
    print('Source/layout result:', 'BLOCKED' if failures else 'PASS')
    print('Pi-only and manual items are surfaced above and do not fail source readiness.')
    return 1 if failures else 0


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description='Tick-to-trade project orchestration')
    subparsers = parser.add_subparsers(dest='command', required=True)

    subparsers.add_parser('doctor', help='Print system and dataset health')

    build_parser = subparsers.add_parser('build', help='Build the project')
    build_parser.add_argument('--profile', choices=['debug', 'release'], default='debug')
    subparsers.add_parser('build-all', help='Build both baseline profiles, all variants, and all replay targets')
    subparsers.add_parser('build-variants', help='Build the registered normal v2-v10 ladder')
    subparsers.add_parser('build-soa', help='Build only v10 Struct-of-Arrays')
    asm_parser = subparsers.add_parser('asm-kernel', help='Test or benchmark the isolated dot-product kernel')
    asm_parser.add_argument('--mode', choices=['test', 'benchmark'], required=True)
    asm_parser.add_argument('--repeats', type=int, default=5)
    asm_parser.add_argument('--platform-label', default=None)
    asm_parser.add_argument('--taskset-cpus', default=None)
    subparsers.add_parser('build-matrix', help='Build normal variants and replay targets')
    subparsers.add_parser('build-replay', help='Build all replay targets')
    subparsers.add_parser('clean-build', help='Remove generated build artifacts only')

    test_parser = subparsers.add_parser('test', help='Run test suite')
    test_parser.add_argument('--suite', choices=['quick', 'full'], default='quick')
    subparsers.add_parser('replay-test', help='Build and run replay-support C unit tests')

    verify_parser = subparsers.add_parser('verify', help='Verify a dataset against the C baseline')
    verify_parser.add_argument('--dataset', default='test20')
    verify_modes = verify_parser.add_mutually_exclusive_group()
    verify_modes.add_argument('--all-versions', action='store_true')
    verify_modes.add_argument('--implemented-only', action='store_true')

    plot_parser = subparsers.add_parser('plot-signals', help='Create signal plot scaffolding')
    plot_parser.add_argument('--dataset', default='dev')

    bench_parser = subparsers.add_parser('benchmark', help='Measure C runner speed')
    bench_parser.add_argument('--dataset', default='stress')
    bench_parser.add_argument('--repeats', type=int, default=5)
    bench_parser.add_argument('--platform-label', default=None,
                              help='explicit results platform label; default is detected OS and machine')
    bench_parser.add_argument('--versions', default=None,
                              help='comma-separated registered version IDs; frozen v0 must be included')
    bench_parser.add_argument('--taskset-cpus', default=None, help='CPU list passed to taskset, for example 3')
    bench_parser.add_argument('--final-controlled', action='store_true',
                              help='require Pi, performance governor on every CPU, taskset, and at least five repeats')

    replay_parser = subparsers.add_parser('replay', help='Replay timestamped trades with a selected strategy version')
    replay_parser.add_argument('--version', required=True, choices=[spec['id'] for spec in replay_specs()])
    replay_parser.add_argument('--dataset', default='test20')
    replay_parser.add_argument('--speed', type=int, choices=[1, 100, 1000], default=1)
    replay_parser.add_argument('--short', type=int, default=None)
    replay_parser.add_argument('--long', type=int, default=None)
    replay_parser.add_argument('--duration-seconds', type=int, default=None)
    replay_parser.add_argument('--limit', type=int, default=None)
    replay_parser.add_argument('--capacity', type=int, default=None)
    replay_parser.add_argument('--output', choices=['off', 'console', 'hardware'], default='off')
    replay_parser.add_argument('--signal-policy', choices=['primary', 'legacy-hybrid-shadow'], default='primary')
    replay_parser.add_argument('--latency-csv', default=None)
    replay_parser.add_argument('--decision-csv', default=None)
    replay_parser.add_argument('--decision-log', action='store_true')

    analysis_versions = [spec['id'] for spec in replay_specs()]
    rival_parser = subparsers.add_parser('rival', help='Compare cash P&L using measured per-signal replay processing durations')
    rival_parser.add_argument('--baseline-version', choices=analysis_versions,
                              default='v0_baseline_strtok_running_double_heap_O0')
    rival_parser.add_argument('--fast-version', choices=analysis_versions, default=None,
                              help='default: fastest eligible version in Pi benchmark_medians.csv evidence')
    rival_parser.add_argument('--dataset', default='test20')
    rival_parser.add_argument('--speed', type=int, choices=[1, 100, 1000], default=1000)
    rival_parser.add_argument('--short', type=int, default=None)
    rival_parser.add_argument('--long', type=int, default=None)
    rival_parser.add_argument('--duration-seconds', type=int, default=None)
    rival_parser.add_argument('--limit', type=int, default=None)
    rival_parser.add_argument('--rival-reaction-us', type=float, default=1.0)
    rival_parser.add_argument('--sweep', action='store_true')
    rival_parser.add_argument('--compact-evidence', action='store_true',
                              help='stream replay evidence and retain only compact actionable-signal outputs')
    rival_parser.add_argument('--output-dir', default=None)
    rival_parser.add_argument('--signal-policy', choices=['primary', 'legacy-hybrid-shadow'], default='primary')

    race_parser = subparsers.add_parser('race', help='Run baseline and selected fast replay processes from a shared monotonic start')
    race_parser.add_argument('--baseline-version', choices=analysis_versions,
                             default='v0_baseline_strtok_running_double_heap_O0')
    race_parser.add_argument('--fast-version', choices=analysis_versions, default=None,
                             help='default: fastest eligible version in Pi benchmark_medians.csv evidence')
    race_parser.add_argument('--dataset', default='test20')
    race_parser.add_argument('--speed', type=int, choices=[1, 100, 1000], default=1000)
    race_parser.add_argument('--short', type=int, default=None)
    race_parser.add_argument('--long', type=int, default=None)
    race_parser.add_argument('--duration-seconds', type=int, default=None)
    race_parser.add_argument('--limit', type=int, default=None)
    race_parser.add_argument('--rival-reaction-us', type=float, default=1.0)
    race_parser.add_argument('--output-dir', default=None)
    race_parser.add_argument('--signal-policy', choices=['primary', 'legacy-hybrid-shadow'], default='primary')
    race_parser.add_argument('--hardware-led', action='store_true', help='enable hardware LED output only for the fast process')
    race_parser.add_argument('--log-only', action='store_true', help='write progress to race_live.csv without live terminal rendering')

    profile_parser = subparsers.add_parser('profile', help='Profile one version using Linux perf')
    profile_parser.add_argument('--versions', default=None, help='comma-separated registered version IDs')
    profile_parser.add_argument('--version', default=None, help='single version ID (legacy spelling)')
    profile_parser.add_argument('--dataset', default='stress_full')
    profile_parser.add_argument('--repeats', type=int, default=1)
    profile_parser.add_argument('--taskset-cpus', default=None)
    profile_parser.add_argument('--platform-label', default=None)

    refresh_profile_parser = subparsers.add_parser(
        'refresh-perf-summary', help='Reparse preserved raw perf files without rerunning perf')
    refresh_profile_parser.add_argument('--profile-dir', required=True,
                                        help='existing profile run directory containing profile_manifest.json')

    assembly_parser = subparsers.add_parser('assembly', help='Generate actual compiler assembly for the selected source')
    assembly_parser.add_argument('--version', required=True,
                                 choices=[item['id'] for item in version_specs()] + ['replay-engine'])
    assembly_parser.add_argument('--platform-label', default=None)

    annotate_parser = subparsers.add_parser(
        'annotate-assembly', help='Create a line-referenced automatic annotation from an existing .s file')
    annotate_parser.add_argument('--assembly', required=True, help='path to actual compiler-generated .s file')
    annotate_parser.add_argument('--version', required=True,
                                 choices=[item['id'] for item in version_specs()] + ['replay-engine'])

    replay_parser = subparsers.add_parser('replay-check', help='Replay scheduling check')
    replay_parser.add_argument('--dataset', default='stress')

    compare_parser = subparsers.add_parser(
        'compare-python', help='Correctness-gated C/Python/NumPy comparison',
        description='Compare C, streaming Python, and batch NumPy after parity gates.',
        epilog=('1-4 repeats are SMOKE / NON-FINAL and use results/*/python-comparison-smoke-runs/. '
                'At least 5 repeats are FINAL-COMPARISON ELIGIBLE and use results/*/python-comparisons-runs/. '
                'A final-controlled/performance platform label requires at least 5 repeats.'))
    compare_parser.add_argument('--dataset', default='stress_full')
    compare_parser.add_argument('--repeats', type=int, default=5,
                                help='number of process runs per engine; 1-4 smoke, 5+ final-comparison eligible')
    compare_parser.add_argument('--platform-label', default=None,
                                help='host/result label; final-controlled labels require at least 5 repeats')
    compare_parser.add_argument('--c-version', default='v5_manual_digit_parser_running_double_stream_O2',
                               choices=[spec['id'] for spec in version_specs()])
    compare_parser.add_argument('--taskset-cpus', default=None)
    compare_parser.add_argument('--short', type=int, default=None)
    compare_parser.add_argument('--long', type=int, default=None)

    subparsers.add_parser('report', help='Print project report')
    subparsers.add_parser('readiness', help='Print readiness summary')

    args = parser.parse_args(argv)

    if args.command == 'doctor':
        return doctor()
    if args.command == 'build':
        return build(args.profile)
    if args.command == 'build-all':
        return build_all()
    if args.command == 'build-variants':
        return build_normal_variants()
    if args.command == 'build-soa':
        return build_soa()
    if args.command == 'asm-kernel':
        return asm_kernel(args.mode, args.repeats, args.platform_label, args.taskset_cpus)
    if args.command == 'build-matrix':
        return build_matrix()
    if args.command == 'build-replay':
        return build_replay_matrix()
    if args.command == 'clean-build':
        return clean_build()
    if args.command == 'test':
        return run_unittest_suite()
    if args.command == 'replay-test':
        status = build_replay_support_test()
        if status != 0:
            return status
        test_exe = BUILD_ROOT / 'tests' / ('replay_support_tests.exe' if os.name == 'nt' else 'replay_support_tests')
        return subprocess.run([str(test_exe)], cwd=str(ROOT), check=False).returncode
    if args.command == 'verify':
        if args.all_versions or args.implemented_only:
            return verify_all_versions(implemented_only=args.implemented_only)
        return verify_dataset(args.dataset, args.all_versions)
    if args.command == 'plot-signals':
        return plot_signals(args.dataset)
    if args.command == 'benchmark':
        versions = args.versions.split(',') if args.versions else None
        return benchmark_dataset(args.dataset, args.repeats, args.platform_label, versions,
                                 args.taskset_cpus, args.final_controlled)
    if args.command == 'replay':
        return replay_command(args)
    if args.command == 'rival':
        if args.rival_reaction_us <= 0 or not math.isfinite(args.rival_reaction_us):
            parser.error('--rival-reaction-us must be a finite positive value')
        if args.short is not None and args.short <= 0 or args.long is not None and args.long <= 0:
            parser.error('--short and --long must be positive')
        if args.short is not None and args.long is not None and args.short > args.long:
            parser.error('--short must not exceed --long')
        return run_rival_command(args)
    if args.command == 'race':
        if args.rival_reaction_us <= 0 or not math.isfinite(args.rival_reaction_us):
            parser.error('--rival-reaction-us must be a finite positive value')
        if args.short is not None and args.short <= 0 or args.long is not None and args.long <= 0:
            parser.error('--short and --long must be positive')
        if args.short is not None and args.long is not None and args.short > args.long:
            parser.error('--short must not exceed --long')
        return run_race_command(args)
    if args.command == 'profile':
        selected = args.versions.split(',') if args.versions else ([args.version] if args.version else [])
        if not selected:
            parser.error('profile requires --versions or --version')
        return profile_versions(args.dataset, selected, args.repeats, args.taskset_cpus, args.platform_label)
    if args.command == 'refresh-perf-summary':
        return refresh_perf_summary(Path(args.profile_dir))
    if args.command == 'assembly':
        return generate_assembly(args.version, args.platform_label)
    if args.command == 'annotate-assembly':
        return annotate_assembly(Path(args.assembly), args.version)
    if args.command == 'replay-check':
        return verify_dataset(args.dataset, True)
    if args.command == 'compare-python':
        return compare_python_numpy(args.dataset, args.repeats, args.platform_label,
                                    args.c_version, args.taskset_cpus, args.short, args.long)
    if args.command == 'report':
        return report()
    if args.command == 'readiness':
        return readiness()
    parser.error('Unknown command')
    return 2


if __name__ == '__main__':
    raise SystemExit(main())
