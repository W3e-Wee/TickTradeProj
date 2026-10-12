"""Market-engine regression tests for all registered offline variants."""
#!/usr/bin/env python3
import csv
import os
import subprocess
import sys
import tempfile
import unittest
from decimal import Decimal
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILD_DIR = ROOT / 'build' / 'release'
DATASET = ROOT / 'test20 - Copy (6).csv'
VARIANT_EXES = [
    BUILD_DIR.parent / 'debug' / 'market_baseline.exe',
    BUILD_DIR / 'market_baseline.exe',
    ROOT / 'build' / 'variants' / 'v2_helper_sscanf_naive_double_stream_O2.exe',
    ROOT / 'build' / 'variants' / 'v3_own_pointer_reader_naive_double_stream_O2.exe',
    ROOT / 'build' / 'variants' / 'v4_manual_digit_parser_naive_double_stream_O2.exe',
    ROOT / 'build' / 'variants' / 'v5_manual_digit_parser_running_double_stream_O2.exe',
    ROOT / 'build' / 'variants' / 'v6_manual_digit_parser_running_fixed_i64_1e8_stream_O2.exe',
    ROOT / 'build' / 'variants' / 'v7_manual_digit_parser_running_double_stream_O3.exe',
    ROOT / 'build' / 'variants' / 'v8_manual_running_double_static_trade_array_O2.exe',
    ROOT / 'build' / 'variants' / 'v9_manual_running_double_exact_malloc_trade_array_O2.exe',
    ROOT / 'build' / 'variants' / 'v10_manual_running_double_soa_O2.exe',
]


def synthetic_trades():
    prices = [
        '10.00000000', '10.00000000', '10.00000000', '10.00000000', '10.00000000',
        '10.00000000', '11.00000000', '12.00000000', '10.00000000', '9.00000000',
        '10.00000000', '10.00000000', '10.00000000', '11.00000000', '11.00000000',
        '9.00000000', '10.00000000', '10.00000000', '12.00000000', '8.00000000',
        '10.00000000', '10.00000000', '10.00000000',
    ]
    trades = []
    for index, price_text in enumerate(prices):
        price = Decimal(price_text)
        qty = Decimal(f'0.{index % 7 + 1:02d}')
        timestamp = 1787294000000000 + (index // 4) * 1000000 + (index % 4) * 10000
        trades.append({
            'id': index + 1,
            'price_text': price_text,
            'price': price,
            'qty': qty,
            'timestamp': timestamp,
            'is_buy': int(index % 3 != 0),
        })
    return trades


def write_trade_csv(path, trades):
    with open(path, 'w', newline='', encoding='utf-8') as handle:
        writer = csv.writer(handle)
        writer.writerow(['trade_id', 'price', 'qty', 'quoteQty', 'time', 'isBuyerMaker', 'isBestMatch'])
        for trade in trades:
            quote_qty = trade['price'] * trade['qty']
            maker = 'False' if trade['is_buy'] else 'True'
            writer.writerow([
                trade['id'], trade['price_text'], f"{trade['qty']:.8f}",
                f'{quote_qty:.8f}', trade['timestamp'], maker, 'True',
            ])


def synthetic_reference(trades, short_window=3, long_window=5):
    rows = []
    bars = []
    vwap_numerator = Decimal(0)
    vwap_denominator = Decimal(0)
    previous_diff = None
    current_bar = None
    for index, trade in enumerate(trades, start=1):
        price = trade['price']
        qty = trade['qty']
        vwap_numerator += price * qty
        vwap_denominator += qty
        short_ready = index >= short_window
        long_ready = index >= long_window
        short_sma = sum((item['price'] for item in trades[index - short_window:index]), Decimal(0)) / short_window if short_ready else None
        long_sma = sum((item['price'] for item in trades[index - long_window:index]), Decimal(0)) / long_window if long_ready else None
        signal = '-'
        if short_ready and long_ready:
            difference = short_sma - long_sma
            if previous_diff is not None:
                if previous_diff <= 0 and difference > 0:
                    signal = 'BUY'
                elif previous_diff >= 0 and difference < 0:
                    signal = 'SELL'
            previous_diff = difference
        rows.append([
            str(index), trade['price_text'], f'{qty:.8f}', str(trade['is_buy']),
            f'{vwap_numerator / vwap_denominator:.8f}',
            f'{short_sma:.8f}' if short_ready else '-',
            f'{long_sma:.8f}' if long_ready else '-', signal,
        ])

        second = trade['timestamp'] // 1000000
        if current_bar is None or current_bar[0] != second:
            if current_bar is not None:
                bars.append(current_bar)
            current_bar = [second, price, price, price, price, 1]
        else:
            current_bar[2] = max(current_bar[2], price)
            current_bar[3] = min(current_bar[3], price)
            current_bar[4] = price
            current_bar[5] += 1
    if current_bar is not None:
        bars.append(current_bar)
    formatted_bars = [[str(bar[0]), *(f'{value:.8f}' for value in bar[1:5]), str(bar[5])] for bar in bars]
    return rows, formatted_bars


def run_executable(executable, *args, env=None):
    if not executable.exists():
        raise FileNotFoundError(f'Missing executable: {executable}')
    return subprocess.run([str(executable), *map(str, args)], capture_output=True, text=True,
                          cwd=str(ROOT), check=False, env=env)


def run_binary(name, *args):
    exe = BUILD_DIR / name
    if not exe.exists():
        raise FileNotFoundError(f'Missing test binary: {exe}')
    return subprocess.run([str(exe), *map(str, args)], capture_output=True, text=True, cwd=str(ROOT), check=False)


class MarketTests(unittest.TestCase):
    def test_stage0_program(self):
        result = run_binary('stage0.exe', '100000')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('N=100000', result.stdout)
        self.assertIn('iterative_sum=', result.stdout)

    def test_test20_expected_output(self):
        result = run_binary('market_baseline.exe', str(DATASET), '3', '5')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('final_vwap=75436.93671742', result.stdout)
        self.assertIn('buy_signals=1', result.stdout)
        self.assertIn('sell_signals=1', result.stdout)

    def test_parser_rejects_invalid_input(self):
        bad = ROOT / 'tests' / 'fixtures' / 'invalid.csv'
        bad.write_text('trade_id,price,qty,quoteQty,time,isBuyerMaker,isBestMatch\n1,abc,1,1,1,False,True\n', encoding='utf-8')
        result = run_binary('market_baseline.exe', str(bad), '3', '5')
        self.assertNotEqual(result.returncode, 0)

    def test_all_versions_match_independent_synthetic_reference(self):
        trades = synthetic_trades()
        expected_rows, expected_bars = synthetic_reference(trades)
        with tempfile.TemporaryDirectory() as temp_dir:
            dataset = Path(temp_dir) / 'synthetic.csv'
            write_trade_csv(dataset, trades)
            for executable in VARIANT_EXES:
                with self.subTest(executable=executable.name):
                    result = run_executable(executable, dataset, '3', '5', '--trace')
                    self.assertEqual(result.returncode, 0, result.stderr)
                    rows = [line.split('|')[1:] for line in result.stdout.splitlines() if line.startswith('TRACE_ROW|')]
                    bars = [line.split('|')[1:] for line in result.stdout.splitlines() if line.startswith('TRACE_BAR|')]
                    self.assertEqual(len(rows), len(expected_rows))
                    self.assertEqual(len(bars), len(expected_bars))
                    for row_number, (actual, expected) in enumerate(zip(rows, expected_rows), start=1):
                        self.assertEqual(len(actual), 8, f'row {row_number}: malformed trace')
                        for field_index, (got, want) in enumerate(zip(actual, expected)):
                            if field_index in {1, 2, 4, 5, 6} and got != '-' and want != '-':
                                self.assertLessEqual(abs(Decimal(got) - Decimal(want)), Decimal('0.00000001'),
                                                     f'row {row_number} field {field_index}')
                            else:
                                self.assertEqual(got, want, f'row {row_number} field {field_index}')
                    for bar_number, (actual, expected) in enumerate(zip(bars, expected_bars), start=1):
                        self.assertEqual(len(actual), 6, f'bar {bar_number}: malformed trace')
                        for field_index, (got, want) in enumerate(zip(actual, expected)):
                            if field_index in {1, 2, 3, 4}:
                                self.assertLessEqual(abs(Decimal(got) - Decimal(want)), Decimal('0.00000001'),
                                                     f'bar {bar_number} field {field_index}')
                            else:
                                self.assertEqual(got, want, f'bar {bar_number} field {field_index}')

    def test_soa_matches_aos_on_official_test20_and_field_alignment_fixture(self):
        aos = ROOT / 'build' / 'variants' / 'v9_manual_running_double_exact_malloc_trade_array_O2.exe'
        soa = ROOT / 'build' / 'variants' / 'v10_manual_running_double_soa_O2.exe'
        aos_result = run_executable(aos, DATASET, '3', '5', '--trace')
        soa_result = run_executable(soa, DATASET, '3', '5', '--trace')
        self.assertEqual(aos_result.returncode, 0, aos_result.stderr)
        self.assertEqual(soa_result.returncode, 0, soa_result.stderr)
        self.assertEqual(soa_result.stdout, aos_result.stdout)

        trades = [
            {'id': 1, 'price_text': '10.25000000', 'price': Decimal('10.25000000'),
             'qty': Decimal('0.12500000'), 'timestamp': 1787294000000000, 'is_buy': 1},
            {'id': 2, 'price_text': '21.50000000', 'price': Decimal('21.50000000'),
             'qty': Decimal('0.25000000'), 'timestamp': 1787294001000000, 'is_buy': 0},
            {'id': 3, 'price_text': '33.75000000', 'price': Decimal('33.75000000'),
             'qty': Decimal('0.50000000'), 'timestamp': 1787294002000000, 'is_buy': 1},
        ]
        with tempfile.TemporaryDirectory() as temp_dir:
            dataset = Path(temp_dir) / 'soa-field-alignment.csv'
            write_trade_csv(dataset, trades)
            aos_result = run_executable(aos, dataset, '2', '3', '--trace')
            soa_result = run_executable(soa, dataset, '2', '3', '--trace')
        self.assertEqual(aos_result.returncode, 0, aos_result.stderr)
        self.assertEqual(soa_result.returncode, 0, soa_result.stderr)
        self.assertEqual(soa_result.stdout, aos_result.stdout)
        first_row = next(line for line in soa_result.stdout.splitlines() if line.startswith('TRACE_ROW|1|'))
        self.assertEqual(first_row.split('|')[:5],
                         ['TRACE_ROW', '1', '10.25000000', '0.12500000', '1'])

    def test_soa_matches_available_development_and_stress_signal_ohlc_summaries(self):
        aos = ROOT / 'build' / 'variants' / 'v9_manual_running_double_exact_malloc_trade_array_O2.exe'
        soa = ROOT / 'build' / 'variants' / 'v10_manual_running_double_soa_O2.exe'
        for dataset in (ROOT / 'dev_1M - Copy (6).csv', ROOT / 'stress_full.csv'):
            if not dataset.is_file():
                continue
            with self.subTest(dataset=dataset.name):
                aos_result = run_executable(aos, dataset, '30', '60', '--signals-only', '--bars-only')
                soa_result = run_executable(soa, dataset, '30', '60', '--signals-only', '--bars-only')
                self.assertEqual(aos_result.returncode, 0, aos_result.stderr)
                self.assertEqual(soa_result.returncode, 0, soa_result.stderr)
                self.assertEqual(soa_result.stdout, aos_result.stdout)

    def test_pointer_and_fast_readers_reject_bad_rows(self):
        executables = [exe for exe in VARIANT_EXES if any(name in exe.name for name in ('v3_', 'v4_', 'v5_', 'v6_', 'v7_', 'v8_', 'v9_'))]
        invalid_rows = [
            '9223372036854775808,1.00000000,0.10000000,0.10000000,1787294000000000,False,True\n',
            '1,not-a-price,0.10000000,0.10000000,1787294000000000,False,True\n',
            '1,1.00000000,0.10000000,0.10000000,1787294000000000,maybe,True\n',
        ]
        with tempfile.TemporaryDirectory() as temp_dir:
            dataset = Path(temp_dir) / 'invalid.csv'
            for row in invalid_rows:
                dataset.write_text('trade_id,price,qty,quoteQty,time,isBuyerMaker,isBestMatch\n' + row,
                                   encoding='utf-8')
                for executable in executables:
                    with self.subTest(executable=executable.name, row=row):
                        result = run_executable(executable, dataset, '3', '5')
                        self.assertNotEqual(result.returncode, 0)

    def test_fixed_parser_rejects_precision_loss_and_int64_overflow(self):
        fixed_exe = next(exe for exe in VARIANT_EXES if 'v6_' in exe.name)
        with tempfile.TemporaryDirectory() as temp_dir:
            dataset = Path(temp_dir) / 'fixed.csv'
            dataset.write_text(
                'trade_id,price,qty,quoteQty,time,isBuyerMaker,isBestMatch\n'
                '1,1.000000001,0.10000000,0.10000000,1787294000000000,False,True\n',
                encoding='utf-8',
            )
            precision_result = run_executable(fixed_exe, dataset, '1', '1')
            self.assertNotEqual(precision_result.returncode, 0)

            dataset.write_text(
                'trade_id,price,qty,quoteQty,time,isBuyerMaker,isBestMatch\n'
                '1,90000000000.00000000,0.10000000,9000000000.00000000,1787294000000000,False,True\n'
                '2,90000000000.00000000,0.10000000,9000000000.00000000,1787294001000000,False,True\n',
                encoding='utf-8',
            )
            overflow_result = run_executable(fixed_exe, dataset, '1', '2')
            self.assertNotEqual(overflow_result.returncode, 0)
            self.assertIn('overflow', overflow_result.stderr.lower())

    def test_storage_capacity_empty_and_allocation_failures(self):
        static_exe = next(exe for exe in VARIANT_EXES if 'v8_' in exe.name)
        malloc_exe = next(exe for exe in VARIANT_EXES if 'v9_' in exe.name)
        soa_exe = next(exe for exe in VARIANT_EXES if 'v10_' in exe.name)
        with tempfile.TemporaryDirectory() as temp_dir:
            dataset = Path(temp_dir) / 'storage.csv'
            trades = synthetic_trades()
            write_trade_csv(dataset, trades)
            for executable in (static_exe, malloc_exe, soa_exe):
                with self.subTest(executable=executable.name, check='capacity'):
                    result = run_executable(executable, dataset, '3', '5', '--capacity', '2')
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn('capacity', result.stderr.lower())

            dataset.write_text('trade_id,price,qty,quoteQty,time,isBuyerMaker,isBestMatch\n', encoding='utf-8')
            for executable in (static_exe, malloc_exe, soa_exe):
                with self.subTest(executable=executable.name, check='empty'):
                    result = run_executable(executable, dataset, '3', '5')
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertIn('trades=0', result.stdout)
                    self.assertIn('final_vwap=NA', result.stdout)

            dataset.write_text('trade_id,price,qty,quoteQty,time,isBuyerMaker,isBestMatch\n', encoding='utf-8')
            dataset.write_text(dataset.read_text(encoding='utf-8') +
                               '1,10.00000000,0.10000000,1.00000000,1787294000000000,False,True\n',
                               encoding='utf-8')
            fail_env = os.environ.copy()
            fail_env['MARKET_TEST_FAIL_ALLOC'] = '1'
            failed = run_executable(malloc_exe, dataset, '3', '5', env=fail_env)
            self.assertNotEqual(failed.returncode, 0)
            self.assertIn('test-injected failure', failed.stderr.lower())

            soa_fail_env = os.environ.copy()
            soa_fail_env['MARKET_TEST_FAIL_ALLOC_AT'] = '3'
            soa_failed = run_executable(soa_exe, dataset, '3', '5', env=soa_fail_env)
            self.assertNotEqual(soa_failed.returncode, 0)
            self.assertIn('all four soa arrays', soa_failed.stderr.lower())

            # Exactly two rows fit; one fewer slot fails and a capacity above
            # the compiled upper bound is rejected before array use.
            small_trades = synthetic_trades()[:2]
            write_trade_csv(dataset, small_trades)
            exact_fit = run_executable(soa_exe, dataset, '1', '2', '--capacity', '2')
            self.assertEqual(exact_fit.returncode, 0, exact_fit.stderr)
            self.assertIn('trades=2', exact_fit.stdout)
            under_capacity = run_executable(soa_exe, dataset, '1', '2', '--capacity', '1')
            self.assertNotEqual(under_capacity.returncode, 0)
            over_compiled = run_executable(soa_exe, dataset, '1', '2', '--capacity', '12000001')
            self.assertNotEqual(over_compiled.returncode, 0)


if __name__ == '__main__':
    unittest.main(verbosity=2)
