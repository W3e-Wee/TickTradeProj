"""Stage A parity tests for the Python and batch NumPy implementations."""
import csv
import importlib.util
import subprocess
import sys
import tempfile
import unittest
import warnings
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'python'))
import market_python
import market_numpy
NUMPY_AVAILABLE = importlib.util.find_spec('numpy') is not None

V1 = ROOT / 'build' / 'release' / 'market_baseline.exe'
TOL = 1.0e-7  # C traces print displayed floating values to eight decimal places.


def write_csv(path, prices, quantities=None, timestamps=None, makers=None, *,
              header=True, blank_before=0, blank_after_header=0, blank_between=False):
    quantities = quantities or [1.0] * len(prices)
    timestamps = timestamps or [1_800_000_000_000_000 + i * 100_000 for i in range(len(prices))]
    makers = makers or [i % 2 == 0 for i in range(len(prices))]
    with path.open('w', newline='', encoding='utf-8') as f:
        w = csv.writer(f)
        for _ in range(blank_before):
            w.writerow([])
        if header:
            w.writerow(['trade_id', 'price', 'qty', 'quoteQty', 'time', 'isBuyerMaker', 'isBestMatch'])
            for _ in range(blank_after_header):
                w.writerow([])
        for i, (p, q, ts, maker) in enumerate(zip(prices, quantities, timestamps, makers), 1):
            w.writerow([i, format(p, '.8f'), format(q, '.8f'), format(p*q, '.8f'), ts,
                        str(bool(maker)), 'True'])
            if blank_between and i < len(prices):
                w.writerow([])


def c_trace(dataset, short, long):
    result = subprocess.run([str(V1), str(dataset), str(short), str(long), '--trace'],
                            cwd=ROOT, capture_output=True, text=True, check=True)
    rows, signals, bars, summary = [], [], [], {}
    for line in result.stdout.splitlines():
        if line.startswith('TRACE_ROW|'):
            f = line.split('|')
            rows.append(f)
        elif line.startswith('SIGNAL|'):
            signals.append(line.split('|'))
        elif line.startswith('TRACE_BAR|'):
            bars.append(line.split('|'))
        elif '=' in line:
            k, v = line.split('=', 1)
            summary[k] = v
    return rows, signals, bars, summary


def assert_numeric(test, expected, actual):
    if expected is None:
        test.assertIsNone(actual)
    else:
        test.assertIsNotNone(actual)
        test.assertLessEqual(abs(float(expected) - float(actual)), TOL)


def compare_result(test, path, short, long, *, trace=False):
    py = market_python.compute_summary(path, short, long)
    if NUMPY_AVAILABLE:
        npy = market_numpy.compute_summary(path, short, long)
    else:
        npy = None
    if npy is not None:
        test.assertEqual(py['trade_count'], npy['trade_count'])
        test.assertEqual(py['buy_signals'], npy['buy_signals'])
        test.assertEqual(py['sell_signals'], npy['sell_signals'])
        test.assertEqual([(x['trade'], x['type']) for x in py['signals']],
                         [(x['trade'], x['type']) for x in npy['signals']])
        test.assertEqual(py['ohlc'], npy['ohlc'])
        assert_numeric(test, py['final_vwap'], npy['final_vwap'])
        test.assertEqual(len(py['rows']), len(npy['rows']))
        for a, b in zip(py['rows'], npy['rows']):
            for key in ('trade', 'trade_id', 'is_buy', 'short_ready', 'long_ready', 'signal'):
                test.assertEqual(a[key], b[key], f'{key} at trade {a["trade"]}')
            for key in ('price', 'qty', 'vwap', 'short_sma', 'long_sma'):
                assert_numeric(test, a[key], b[key])
    if not V1.exists():
        return py
    c_rows, c_signals, c_bars, c_summary = c_trace(path, short, long)
    test.assertEqual(len(c_rows), len(py['rows']))
    # Exact trade ordinal, buy/sell flag, warm-up, and signal decision; tolerance
    # applies only to the displayed floating-point fields.
    for c, p in zip(c_rows, py['rows']):
        test.assertEqual(int(c[1]), p['trade'])
        test.assertEqual(int(c[4]), int(p['is_buy']))
        assert_numeric(test, c[2], p['price'])
        assert_numeric(test, c[3], p['qty'])
        assert_numeric(test, c[5], p['vwap'])
        assert_numeric(test, None if c[6] == '-' else c[6], p['short_sma'])
        assert_numeric(test, None if c[7] == '-' else c[7], p['long_sma'])
        test.assertEqual(c[8], p['signal'] or '-')
    if c_signals:
        test.assertEqual([(int(x[1]), x[2]) for x in c_signals],
                         [(x['trade'], x['type']) for x in py['signals']])
    else:
        test.assertEqual([(int(row[1]), row[8]) for row in c_rows if row[8] in ('BUY', 'SELL')],
                         [(x['trade'], x['type']) for x in py['signals']])
    test.assertEqual(len(c_bars), len(py['ohlc']))
    for c, p in zip(c_bars, py['ohlc']):
        test.assertEqual(int(c[1]), p['second'])
        test.assertEqual(int(c[6]), p['count'])
        for ci, key in zip((2, 3, 4, 5), ('open', 'high', 'low', 'close')):
            assert_numeric(test, c[ci], p[key])
    assert_numeric(test, c_summary.get('final_vwap'), py['final_vwap'])
    test.assertEqual(int(c_summary.get('trades', 0)), py['trade_count'])
    return py


class PythonEngineParityTests(unittest.TestCase):
    @unittest.skipUnless(NUMPY_AVAILABLE, 'optional NumPy dependency is not installed')
    def test_numpy_official_test20_parity(self):
        path = ROOT / 'test20 - Copy (6).csv'
        self.assertEqual(20, market_numpy.compute_summary(path, 3, 5)['trade_count'])
        self.assertEqual(next(market_python.iter_csv(path))['trade_id'],
                         int(market_numpy.parse_numpy(path)['trade_id'][0]))
        compare_result(self, path, 3, 5, trace=True)

    def test_header_and_blank_lines_preserve_trade_order(self):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / 'header_blanks.csv'
            write_csv(path, [8, 9, 7, 11], [0.5, 2.0, 0.25, 1.75],
                      timestamps=[1_800_000_000_000_000] * 4,
                      header=True, blank_before=2, blank_after_header=1, blank_between=True)
            result = compare_result(self, path, 2, 3, trace=True)
            self.assertEqual(4, result['trade_count'])
            self.assertEqual([1, 2, 3, 4], [r['trade'] for r in result['rows']])
            self.assertEqual(4, result['ohlc'][0]['count'])

    @unittest.skipUnless(NUMPY_AVAILABLE, 'optional NumPy dependency is not installed')
    def test_numpy_header_blank_lines_and_duplicate_timestamps(self):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / 'header_blanks.csv'
            write_csv(path, [8, 9, 7, 11], [0.5, 2.0, 0.25, 1.75],
                      timestamps=[1_800_000_000_000_000] * 4,
                      header=True, blank_before=2, blank_after_header=1, blank_between=True)
            result = compare_result(self, path, 2, 3, trace=True)
            self.assertEqual(4, result['trade_count'])
            self.assertEqual([1, 2, 3, 4], [r['trade'] for r in result['rows']])
            self.assertEqual(4, result['ohlc'][0]['count'])

    def test_official_headerless_python_input_starts_with_trade_one(self):
        path = ROOT / 'test20 - Copy (6).csv'
        first = next(market_python.iter_csv(path))
        result = market_python.compute_summary(path, 3, 5)
        self.assertEqual(20, result['trade_count'])
        self.assertEqual(1, result['rows'][0]['trade'])
        self.assertEqual(first['trade_id'], result['rows'][0]['trade_id'])

    @unittest.skipUnless(NUMPY_AVAILABLE, 'optional NumPy dependency is not installed')
    def test_numpy_empty_inputs_emit_no_warning(self):
        with tempfile.TemporaryDirectory() as td:
            empty_paths = [Path(td) / 'zero_bytes.csv', Path(td) / 'header_only.csv']
            empty_paths[0].write_text('', encoding='utf-8')
            empty_paths[1].write_text(
                'trade_id,price,qty,quoteQty,time,isBuyerMaker,isBestMatch\n\n', encoding='utf-8')
            for path in empty_paths:
                with warnings.catch_warnings(record=True) as captured:
                    warnings.simplefilter('always')
                    result = market_numpy.compute_summary(path)
                self.assertFalse([w for w in captured if issubclass(w.category, UserWarning)])
                self.assertEqual(0, result['trade_count'])
                self.assertIsNone(result['final_vwap'])

    @unittest.skipUnless(NUMPY_AVAILABLE, 'optional NumPy dependency is not installed')
    def test_numpy_does_not_skip_malformed_trade_rows(self):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / 'malformed.csv'
            write_csv(path, [8, 9])
            with path.open('a', encoding='utf-8') as handle:
                handle.write('3,not-a-price,1,3,1800000000000000,False,True\n')
            with self.assertRaises(ValueError):
                market_numpy.parse_numpy(path)

    def test_official_test20(self):
        compare_result(self, ROOT / 'test20 - Copy (6).csv', 3, 5, trace=True)

    def test_development_dataset(self):
        path = ROOT / 'dev_1M - Copy (6).csv'
        if path.exists() and V1.exists():
            # Compare aggregate outcomes, then signal/bar identities with exact keys.
            py = market_python.compute_summary(path, 30, 60, include_rows=False)
            if NUMPY_AVAILABLE:
                npy = market_numpy.compute_summary(path, 30, 60, include_rows=False)
            else:
                npy = None
            if npy is not None:
                self.assertEqual(py['trade_count'], npy['trade_count'])
                self.assertEqual(py['buy_signals'], npy['buy_signals'])
                self.assertEqual(py['sell_signals'], npy['sell_signals'])
                self.assertEqual([(s['trade'], s['type']) for s in py['signals']],
                                 [(s['trade'], s['type']) for s in npy['signals']])
                self.assertEqual(py['ohlc'], npy['ohlc'])
                assert_numeric(self, py['final_vwap'], npy['final_vwap'])
            result = subprocess.run([str(V1), str(path), '30', '60', '--signals-only', '--bars-only'],
                                    cwd=ROOT, capture_output=True, text=True, check=True)
            c_signals, c_bars = [], []
            for line in result.stdout.splitlines():
                if line.startswith('SIGNAL|'):
                    p = line.split('|')
                    c_signals.append((int(p[1]), p[2]))
                if line.startswith('TRACE_BAR|'):
                    c_bars.append(line.split('|'))
            self.assertEqual([(s['trade'], s['type']) for s in py['signals']], c_signals)
            self.assertEqual(len(c_bars), len(py['ohlc']))
            for c, p in zip(c_bars, py['ohlc']):
                self.assertEqual((int(c[1]), int(c[6])), (p['second'], p['count']))
                for ci, key in zip((2, 3, 4, 5), ('open', 'high', 'low', 'close')):
                    assert_numeric(self, c[ci], p[key])

    def test_constant_equal_crossing_repeated_time_and_quantity(self):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / 'edge.csv'
            prices = [10, 10, 10, 10, 10, 10, 12, 14, 9, 8, 10, 11, 10, 9, 12, 8]
            times = [1_800_000_000_000_000 + (i // 3) * 1_000_000 for i in range(len(prices))]
            write_csv(path, prices, [0.25 + (i % 4) for i in range(len(prices))], times)
            result = compare_result(self, path, 3, 5, trace=True)
            self.assertTrue(result['signals'])
            constant = Path(td) / 'constant.csv'
            write_csv(constant, [4] * 8)
            equal_result = compare_result(self, constant, 3, 5, trace=True)
            self.assertEqual([], equal_result['signals'])

    def test_empty_and_small_inputs(self):
        with tempfile.TemporaryDirectory() as td:
            empty = Path(td) / 'empty.csv'
            empty.write_text('trade_id,price,qty,quoteQty,time,isBuyerMaker,isBestMatch\n', encoding='utf-8')
            py = market_python.compute_summary(empty)
            self.assertEqual(0, py['trade_count'])
            self.assertIsNone(py['final_vwap'])
            if NUMPY_AVAILABLE:
                npy = market_numpy.compute_summary(empty)
            else:
                npy = None
            if npy is not None:
                self.assertEqual(py, npy)
            small = Path(td) / 'small.csv'
            write_csv(small, [5, 6], [0.5, 1.5])
            py = compare_result(self, small, 3, 5, trace=True)
            if NUMPY_AVAILABLE:
                npy = market_numpy.compute_summary(small)
            else:
                npy = None
            if npy is not None:
                self.assertEqual(py, npy)
            self.assertEqual([False, False], [row['short_ready'] for row in py['rows']])


if __name__ == '__main__':
    unittest.main()
