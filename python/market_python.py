#!/usr/bin/env python3
"""Plain-Python Stage A engine; CSV rows are processed in file order."""
from __future__ import annotations

import argparse
import csv
import json
import math
import time
from collections import deque
from pathlib import Path


def parse_bool(value: str) -> bool:
    value = value.strip().lower()
    if value in {'true', '1', 'yes'}:
        return True
    if value in {'false', '0', 'no'}:
        return False
    raise ValueError(f'invalid boolean value: {value!r}')


def iter_csv(path):
    """Yield validated Binance records without reordering or sorting them."""
    with Path(path).open(newline='', encoding='utf-8-sig') as handle:
        reader = csv.reader(handle)
        header_seen = False
        for line_no, row in enumerate(reader, 1):
            if not row or not any(field.strip() for field in row):
                continue
            if not header_seen and row[0].strip().lower() == 'trade_id':
                header_seen = True
                continue
            header_seen = True
            if len(row) != 7:
                raise ValueError(f'{path}:{line_no}: expected 7 CSV fields, got {len(row)}')
            try:
                maker = parse_bool(row[5])
                record = {
                    'trade_id': int(row[0]), 'price': float(row[1]), 'qty': float(row[2]),
                    'quoteQty': float(row[3]), 'time': int(row[4]),
                    # Binance isBuyerMaker means the buyer was maker; that trade is sell-side.
                    'is_buy': not maker, 'is_best_match': parse_bool(row[6]),
                }
            except (ValueError, OverflowError) as exc:
                raise ValueError(f'{path}:{line_no}: invalid trade row: {exc}') from exc
            if not all(math.isfinite(record[k]) for k in ('price', 'qty', 'quoteQty')):
                raise ValueError(f'{path}:{line_no}: non-finite numeric value')
            yield record


def compute_records(records, short_window=3, long_window=5, *, include_rows=True):
    if short_window <= 0 or long_window <= 0 or short_window > long_window:
        raise ValueError('windows must be positive and short_window <= long_window')
    short_ring, long_ring = [0.0] * short_window, [0.0] * long_window
    short_sum = long_sum = vwap_num = vwap_den = 0.0
    count = buys = sells = 0
    prev_diff = None
    signals, bars, rows = [], [], []
    current_bar = None
    for trade in records:
        count += 1
        price, qty = float(trade['price']), float(trade['qty'])
        short_i, long_i = (count - 1) % short_window, (count - 1) % long_window
        # Keep the C sequence of double operations: add the new item, then
        # subtract the expired ring value once the window is full.
        short_sum += price
        long_sum += price
        if count > short_window:
            short_sum -= short_ring[short_i]
        if count > long_window:
            long_sum -= long_ring[long_i]
        short_ring[short_i], long_ring[long_i] = price, price
        vwap_num += price * qty
        vwap_den += qty
        short_ready, long_ready = count >= short_window, count >= long_window
        short_sma = short_sum / short_window if short_ready else None
        long_sma = long_sum / long_window if long_ready else None
        signal = None
        if short_ready and long_ready:
            diff = short_sma - long_sma
            if prev_diff is not None:
                if prev_diff <= 0.0 and diff > 0.0:
                    buys += 1
                    signal = 'BUY'
                elif prev_diff >= 0.0 and diff < 0.0:
                    sells += 1
                    signal = 'SELL'
                if signal:
                    signals.append({'trade': count, 'type': signal,
                                    'short_sma': short_sma, 'long_sma': long_sma})
            prev_diff = diff
        second = int(trade['time']) // 1_000_000
        if current_bar is None or current_bar['second'] != second:
            current_bar = {'second': second, 'open': price, 'high': price, 'low': price,
                           'close': price, 'count': 0}
            bars.append(current_bar)
        current_bar['high'] = max(current_bar['high'], price)
        current_bar['low'] = min(current_bar['low'], price)
        current_bar['close'] = price
        current_bar['count'] += 1
        if include_rows:
            rows.append({'trade': count, 'trade_id': trade['trade_id'], 'price': price,
                         'qty': qty, 'is_buy': bool(trade['is_buy']), 'vwap':
                         (vwap_num / vwap_den if vwap_den else None),
                         'short_sma': short_sma, 'long_sma': long_sma,
                         'short_ready': short_ready, 'long_ready': long_ready, 'signal': signal})
    return {'trade_count': count, 'final_vwap': vwap_num / vwap_den if vwap_den else None,
            'buy_signals': buys, 'sell_signals': sells, 'signals': signals,
            'ohlc': bars, 'rows': rows}


def compute_summary(path, short_window=3, long_window=5, *, include_rows=True):
    return compute_records(iter_csv(path), short_window, long_window, include_rows=include_rows)


def timed_summary(path, short_window, long_window, engine='python'):
    start = time.perf_counter_ns()
    if engine == 'python':
        # Parsing and calculation are deliberately interleaved for a bounded-memory
        # stream. Separate parse/compute phases are not meaningful for this path.
        result = compute_records(iter_csv(path), short_window, long_window, include_rows=False)
        end = time.perf_counter_ns()
        result.pop('rows', None)
        compact = {k: result[k] for k in ('trade_count', 'final_vwap', 'buy_signals', 'sell_signals')}
        compact['ohlc_bars'] = len(result['ohlc'])
        return {'load_parse_ns': None, 'compute_ns': None,
                'engine_total_ns': end - start, 'result': compact}
    else:
        from market_numpy import parse_numpy, compute_numpy
        records = parse_numpy(path)
        parsed = time.perf_counter_ns()
        result = compute_numpy(records, short_window, long_window, include_rows=False)
    end = time.perf_counter_ns()
    result.pop('rows', None)
    compact = {k: result[k] for k in ('trade_count', 'final_vwap', 'buy_signals', 'sell_signals')}
    compact['ohlc_bars'] = len(result['ohlc'])
    return {'load_parse_ns': parsed - start, 'compute_ns': end - parsed,
            'engine_total_ns': end - start, 'result': compact}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('dataset')
    ap.add_argument('--short', type=int, default=3)
    ap.add_argument('--long', type=int, default=5)
    ap.add_argument('--json', action='store_true', help='emit the full result as JSON')
    ap.add_argument('--benchmark-summary', action='store_true', help=argparse.SUPPRESS)
    args = ap.parse_args()
    if args.benchmark_summary:
        print(json.dumps(timed_summary(args.dataset, args.short, args.long), separators=(',', ':')))
    else:
        result = compute_summary(args.dataset, args.short, args.long)
        if args.json:
            print(json.dumps(result, separators=(',', ':')))
        else:
            print(json.dumps({k: v for k, v in result.items() if k != 'rows'}, indent=2))


if __name__ == '__main__':
    main()
