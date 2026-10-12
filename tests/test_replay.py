"""Timestamp replay correctness, timing-path, and version parity tests."""
#!/usr/bin/env python3
import csv
import os
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPLAY_DIR = ROOT / 'build' / 'replay'
OFFLINE_DIR = ROOT / 'build' / 'variants'
TEST20 = ROOT / 'test20 - Copy (6).csv'
VERSIONS = [
    'v0_baseline_strtok_running_double_heap_O0',
    'v1_baseline_strtok_running_double_heap_O2',
    'v2_helper_sscanf_naive_double_stream_O2',
    'v3_own_pointer_reader_naive_double_stream_O2',
    'v4_manual_digit_parser_naive_double_stream_O2',
    'v5_manual_digit_parser_running_double_stream_O2',
    'v6_manual_digit_parser_running_fixed_i64_1e8_stream_O2',
    'v7_manual_digit_parser_running_double_stream_O3',
    'v8_manual_running_double_static_trade_array_O2',
    'v9_manual_running_double_exact_malloc_trade_array_O2',
    'v10_manual_running_double_soa_O2',
]


def replay(executable, dataset, *args):
    return subprocess.run([str(executable), str(dataset), *map(str, args)],
                          cwd=str(ROOT), capture_output=True, text=True, check=False)


def replay_signals(decision_csv, column):
    result = []
    with decision_csv.open(newline='', encoding='utf-8') as handle:
        for row in csv.DictReader(handle):
            if row[column] != '0':
                result.append((int(row['trade']), int(row[column])))
    return result


def offline_signals(executable, dataset, windows):
    result = subprocess.run([str(executable), str(dataset), str(windows[0]), str(windows[1]),
                             '--signals-only'], cwd=str(ROOT), capture_output=True, text=True, check=False)
    if result.returncode != 0:
        raise AssertionError(result.stderr)
    events = []
    for line in result.stdout.splitlines():
        if line.startswith('SIGNAL|'):
            _, trade, signal, *_ = line.split('|')
            events.append((int(trade), 1 if signal == 'BUY' else -1))
    return events


def synthetic_equal_timestamp_file(path):
    prices = [10.0, 10.0, 10.0, 10.0, 10.0, 11.0, 12.0, 10.0, 9.0, 10.0, 10.0]
    with path.open('w', newline='', encoding='utf-8') as handle:
        writer = csv.writer(handle)
        writer.writerow(['trade_id', 'price', 'qty', 'quoteQty', 'time', 'isBuyerMaker', 'isBestMatch'])
        for index, price in enumerate(prices):
            timestamp = 1787294000000000 + (index // 3) * 1000000
            writer.writerow([index + 1, f'{price:.8f}', '0.10000000', f'{price / 10:.8f}',
                             timestamp, 'False', 'True'])


class ReplayTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        missing = [str(REPLAY_DIR / f'{version}.exe') for version in VERSIONS
                   if not (REPLAY_DIR / f'{version}.exe').is_file()]
        if missing:
            raise RuntimeError(
                'Replay test prerequisites are missing: ' + ', '.join(missing) +
                '. Run `python3 scripts/project.py build --profile release` first.'
            )

    def test_all_replay_versions_process_official_test20_without_drops(self):
        expected = [(7, 1), (10, -1)]
        with tempfile.TemporaryDirectory() as temp_dir:
            latency = Path(temp_dir) / 'latency.csv'
            decisions = Path(temp_dir) / 'decisions.csv'
            for version in VERSIONS:
                executable = REPLAY_DIR / f'{version}.exe'
                with self.subTest(version=version):
                    result = replay(executable, TEST20, '--version-id', version,
                                    '--speed', 1000, '--short', 3, '--long', 5,
                                    '--limit', 20, '--output', 'off',
                                    '--latency-csv', latency, '--decision-csv', decisions)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertIn('processed_count=20', result.stdout)
                    self.assertIn('latency_sample_count=20', result.stdout)
                    self.assertEqual(replay_signals(decisions, 'primary_decision'), expected)
                    if version == 'v5_manual_digit_parser_running_double_stream_O2':
                        offline = offline_signals(
                            OFFLINE_DIR / 'v5_manual_digit_parser_running_double_stream_O2.exe',
                            TEST20, (3, 5))
                        self.assertEqual(replay_signals(decisions, 'effective_decision'), offline)
                    with latency.open(newline='', encoding='utf-8') as handle:
                        latency_rows = list(csv.DictReader(handle))
                    self.assertEqual(len(latency_rows), 20)
                    self.assertEqual([int(row['trade']) for row in latency_rows], list(range(1, 21)))
                    self.assertTrue(all(int(row['processing_duration_ns']) >= 0 for row in latency_rows))

    def test_speed_schedule_equal_timestamps_and_output_modes(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            dataset = Path(temp_dir) / 'equal.csv'
            decisions = Path(temp_dir) / 'decisions.csv'
            latency = Path(temp_dir) / 'latency.csv'
            synthetic_equal_timestamp_file(dataset)
            executable = REPLAY_DIR / 'v5_manual_digit_parser_running_double_stream_O2.exe'
            for speed in (1, 100, 1000):
                result = replay(executable, dataset, '--version-id', 'v5_manual_digit_parser_running_double_stream_O2',
                                '--speed', speed, '--short', 3, '--long', 5, '--limit', 11,
                                '--output', 'off', '--latency-csv', latency)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn('processed_count=11', result.stdout)
                with latency.open(newline='', encoding='utf-8') as handle:
                    rows = list(csv.DictReader(handle))
                self.assertEqual(len(rows), 11)
                releases = [int(row['scheduled_release_ns']) for row in rows]
                self.assertEqual(releases[0], releases[1])
                self.assertTrue(all(left <= right for left, right in zip(releases, releases[1:])))

            console = replay(executable, dataset, '--version-id', 'v5_manual_digit_parser_running_double_stream_O2',
                             '--speed', 1000, '--short', 3, '--long', 5, '--limit', 11,
                             '--output', 'console', '--decision-csv', decisions)
            self.assertEqual(console.returncode, 0, console.stderr)
            self.assertIn('LED|trade=', console.stdout)
            self.assertIn('LED|cleanup|', console.stdout)
            with decisions.open(newline='', encoding='utf-8') as handle:
                decision_rows = list(csv.DictReader(handle))
            self.assertEqual(len(decision_rows), 11)
            for index, row in enumerate(decision_rows[:5], start=1):
                self.assertEqual(float(row['primary_short_sma']), 10.0 if index >= 3 else 0.0)
                self.assertEqual(float(row['primary_long_sma']), 10.0 if index >= 5 else 0.0)
                self.assertEqual(row['primary_decision'], '0')
                self.assertEqual(row['shadow_decision'], '0')
            off = replay(executable, dataset, '--version-id', 'v5_manual_digit_parser_running_double_stream_O2',
                         '--speed', 1000, '--short', 3, '--long', 5, '--limit', 11, '--output', 'off')
            self.assertEqual(off.returncode, 0, off.stderr)
            self.assertNotIn('LED|', off.stdout)

            zero_duration = replay(executable, dataset, '--version-id', 'v5_manual_digit_parser_running_double_stream_O2',
                                   '--speed', 1000, '--short', 3, '--long', 5,
                                   '--duration-seconds', 0, '--output', 'off')
            self.assertEqual(zero_duration.returncode, 0, zero_duration.stderr)
            self.assertIn('processed_count=3', zero_duration.stdout)
            self.assertIn('stop_reason=duration_limit', zero_duration.stdout)

            if os.name == 'nt':
                hardware = replay(executable, dataset, '--version-id', 'v5_manual_digit_parser_running_double_stream_O2',
                                  '--speed', 1000, '--limit', 1, '--output', 'hardware')
                self.assertNotEqual(hardware.returncode, 0)
                self.assertIn('unavailable', hardware.stderr.lower())

    def test_primary_and_shadow_paths_are_exposed_and_policy_selectable(self):
        dev = ROOT / 'dev_1M - Copy (6).csv'
        with tempfile.TemporaryDirectory() as temp_dir:
            decision_log = Path(temp_dir) / 'primary.csv'
            executable = REPLAY_DIR / 'v4_manual_digit_parser_naive_double_stream_O2.exe'
            command = [str(executable), str(dev), '--version-id', 'v4_manual_digit_parser_naive_double_stream_O2',
                       '--speed', '1000', '--short', '30', '--long', '60', '--limit', '500',
                       '--output', 'off', '--decision-csv', str(decision_log)]
            primary = subprocess.run(command, cwd=str(ROOT), capture_output=True, text=True, check=False)
            self.assertEqual(primary.returncode, 0, primary.stderr)
            with decision_log.open(newline='', encoding='utf-8') as handle:
                rows = list(csv.DictReader(handle))
            disagreements = [row for row in rows if row['primary_decision'] != row['shadow_decision']]
            self.assertTrue(rows)
            self.assertGreater(len(disagreements), 0, 'near-crossing fixture should expose primary/shadow differences')
            self.assertTrue(all('primary_short_sma' in row and 'shadow_short_sma' in row for row in rows))
            self.assertEqual(primary.stdout.count('primary_shadow_disagreements='), 1)
            if disagreements:
                self.assertTrue(all(row['effective_decision'] == row['primary_decision'] for row in disagreements))

            hybrid_path = Path(temp_dir) / 'hybrid.csv'
            command[command.index('--decision-csv') + 1] = str(hybrid_path)
            command += ['--signal-policy', 'legacy-hybrid-shadow']
            hybrid = subprocess.run(command, cwd=str(ROOT), capture_output=True, text=True, check=False)
            self.assertEqual(hybrid.returncode, 0, hybrid.stderr)
            with hybrid_path.open(newline='', encoding='utf-8') as handle:
                hybrid_rows = list(csv.DictReader(handle))
            self.assertTrue(all(row['effective_decision'] == row['shadow_decision'] for row in hybrid_rows))

    def test_replay_rejects_decreasing_timestamp(self):
        executable = REPLAY_DIR / 'v5_manual_digit_parser_running_double_stream_O2.exe'
        with tempfile.TemporaryDirectory() as temp_dir:
            dataset = Path(temp_dir) / 'decreasing.csv'
            dataset.write_text(
                'trade_id,price,qty,quoteQty,time,isBuyerMaker,isBestMatch\n'
                '1,10.00000000,0.10000000,1.00000000,1787294000000000,False,True\n'
                '2,10.00000000,0.10000000,1.00000000,1787293999000000,False,True\n',
                encoding='utf-8',
            )
            result = replay(executable, dataset, '--speed', 1000, '--short', 1, '--long', 1)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('decreasing timestamp', result.stderr.lower())


if __name__ == '__main__':
    unittest.main(verbosity=2)
