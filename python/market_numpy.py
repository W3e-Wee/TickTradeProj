#!/usr/bin/env python3
"""Batch/offline NumPy-assisted Stage A engine (not replay-latency code)."""
from __future__ import annotations

import math
from pathlib import Path
import argparse
import json
import time


def _empty_arrays(np):
    return {'trade_id': np.empty(0, dtype=np.int64), 'price': np.empty(0),
            'qty': np.empty(0), 'quoteQty': np.empty(0), 'time': np.empty(0, dtype=np.int64),
            'is_buy': np.empty(0, dtype=np.bool_), 'is_best_match': np.empty(0, dtype=np.bool_)}


def parse_numpy(path):
    """Load Binance CSV rows in file order, with optional-header detection."""
    import numpy as np
    import csv
    path = Path(path)
    first_line = None
    first_line_number = 0
    has_following_data = False
    try:
        with path.open('r', encoding='utf-8-sig', newline='') as handle:
            for line_number, line in enumerate(handle, 1):
                if line.strip():
                    first_line, first_line_number = line, line_number
                    break
            if first_line is None:
                return _empty_arrays(np)
            fields = next(csv.reader([first_line]))
            normalized = [''.join(ch.lower() for ch in field if ch.isalnum()) for field in fields]
            expected_header = ['tradeid', 'price', 'qty', 'quoteqty', 'time',
                               'isbuyermaker', 'isbestmatch']
            has_header = normalized == expected_header
            if has_header:
                has_following_data = any(line.strip() for line in handle)
    except (OSError, UnicodeError, csv.Error) as exc:
        raise ValueError(f'{path}: cannot inspect Binance CSV: {exc}') from exc
    # Avoid NumPy's empty-input warning for a genuinely empty file or a
    # header-only CSV. Blank lines after a header do not make it nonempty.
    if has_header and not has_following_data:
        return _empty_arrays(np)

    def bool_conv(value):
        text = (value.decode('utf-8-sig') if isinstance(value, bytes) else str(value)).strip().lower()
        if text in ('true', '1', 'yes'):
            return True
        if text in ('false', '0', 'no'):
            return False
        raise ValueError(f'invalid boolean value: {text!r}')
    # NumPy's native parser keeps row order. skiprows is nonzero only after
    # recognizing the complete expected header; headerless official files keep
    # their first trade. Blank lines are ignored as in market_python.iter_csv.
    try:
        data = np.loadtxt(path, delimiter=',', skiprows=first_line_number if has_header else 0,
            comments=None, dtype=[
            ('trade_id', np.int64), ('price', np.float64), ('qty', np.float64),
            ('quoteQty', np.float64), ('time', np.int64), ('maker', np.bool_),
            ('best', np.bool_)], converters={5: bool_conv, 6: bool_conv}, ndmin=1,
            encoding='utf-8-sig')
    except (ValueError, OSError) as exc:
        raise ValueError(f'{path}: cannot parse Binance CSV with NumPy: {exc}') from exc
    if not all(np.isfinite(data[name]).all() for name in ('price', 'qty', 'quoteQty')):
        raise ValueError(f'{path}: non-finite numeric value')
    return {'trade_id': data['trade_id'], 'price': data['price'], 'qty': data['qty'],
            'quoteQty': data['quoteQty'], 'time': data['time'], 'is_buy': ~data['maker'],
            'is_best_match': data['best']}


def compute_numpy(data, short_window=3, long_window=5, *, include_rows=True):
    import numpy as np
    if short_window <= 0 or long_window <= 0 or short_window > long_window:
        raise ValueError('windows must be positive and short_window <= long_window')
    prices, qty, timestamps = data['price'], data['qty'], data['time']
    n = len(prices)
    # Native NumPy bulk operation for cumulative VWAP; SMA/crossover remain an
    # ordered Python recurrence to preserve the C running-sum crossover semantics.
    quote = np.cumsum(prices * qty, dtype=np.float64)
    quantity = np.cumsum(qty, dtype=np.float64)
    vwaps = np.divide(quote, quantity, out=np.full(n, np.nan), where=quantity != 0)
    short_ring, long_ring = [0.0] * short_window, [0.0] * long_window
    short_sum = long_sum = 0.0
    prev_diff = None
    signals, rows = [], []
    buys = sells = 0
    for i in range(n):
        price = float(prices[i])
        short_i, long_i = i % short_window, i % long_window
        short_sum += price
        long_sum += price
        if i >= short_window:
            short_sum -= short_ring[short_i]
        if i >= long_window:
            long_sum -= long_ring[long_i]
        short_ring[short_i], long_ring[long_i] = price, price
        short_sma = short_sum / short_window if i + 1 >= short_window else None
        long_sma = long_sum / long_window if i + 1 >= long_window else None
        signal = None
        if short_sma is not None and long_sma is not None:
            diff = short_sma - long_sma
            if prev_diff is not None:
                if prev_diff <= 0.0 and diff > 0.0:
                    signal, buys = 'BUY', buys + 1
                elif prev_diff >= 0.0 and diff < 0.0:
                    signal, sells = 'SELL', sells + 1
                if signal:
                    signals.append({'trade': i + 1, 'type': signal,
                                    'short_sma': short_sma, 'long_sma': long_sma})
            prev_diff = diff
        if include_rows:
            rows.append({'trade': i + 1, 'trade_id': int(data['trade_id'][i]),
                         'price': price, 'qty': float(qty[i]),
                         'is_buy': bool(data['is_buy'][i]),
                         'vwap': float(vwaps[i]) if math.isfinite(vwaps[i]) else None,
                         'short_sma': short_sma, 'long_sma': long_sma,
                         'short_ready': short_sma is not None,
                         'long_ready': long_sma is not None, 'signal': signal})
    # Bars are contiguous timestamp runs, matching the C engine's file-order rule.
    seconds = timestamps // 1_000_000
    if n:
        starts = np.r_[0, np.flatnonzero(seconds[1:] != seconds[:-1]) + 1]
        ends = np.r_[starts[1:], n]
        bars = [{'second': int(seconds[a]), 'open': float(prices[a]),
                 'high': float(np.max(prices[a:b])), 'low': float(np.min(prices[a:b])),
                 'close': float(prices[b - 1]), 'count': int(b - a)}
                for a, b in zip(starts.tolist(), ends.tolist())]
    else:
        bars = []
    total_qty = float(quantity[-1]) if n else 0.0
    final_vwap = float(vwaps[-1]) if n and total_qty != 0 else None
    return {'trade_count': n, 'final_vwap': final_vwap, 'buy_signals': buys,
            'sell_signals': sells, 'signals': signals, 'ohlc': bars, 'rows': rows}


def compute_summary(path, short_window=3, long_window=5, *, include_rows=True):
    return compute_numpy(parse_numpy(path), short_window, long_window, include_rows=include_rows)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('dataset')
    ap.add_argument('--short', type=int, default=3)
    ap.add_argument('--long', type=int, default=5)
    ap.add_argument('--json', action='store_true')
    ap.add_argument('--benchmark-summary', action='store_true', help=argparse.SUPPRESS)
    args = ap.parse_args()
    if args.benchmark_summary:
        start = time.perf_counter_ns()
        data = parse_numpy(args.dataset)
        parsed = time.perf_counter_ns()
        result = compute_numpy(data, args.short, args.long, include_rows=False)
        end = time.perf_counter_ns()
        result.pop('rows', None)
        compact = {k: result[k] for k in ('trade_count', 'final_vwap', 'buy_signals', 'sell_signals')}
        compact['ohlc_bars'] = len(result['ohlc'])
        print(json.dumps({'load_parse_ns': parsed-start, 'compute_ns': end-parsed,
                          'engine_total_ns': end-start, 'result': compact}, separators=(',', ':')))
    else:
        result = compute_summary(args.dataset, args.short, args.long)
        print(json.dumps(result if args.json else {k: v for k, v in result.items() if k != 'rows'},
                         separators=(',', ':') if args.json else None, indent=None if args.json else 2))


if __name__ == '__main__':
    main()
