"""Deterministic coursework rival execution and reaction-time analysis."""

from __future__ import annotations

import csv
import math
import sys
from itertools import zip_longest
from dataclasses import dataclass
from decimal import Decimal
from pathlib import Path
from typing import Iterable


REACTION_SWEEP_US = (0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0, 20.0, 50.0, 100.0)


def decimal_text(value: Decimal) -> str:
    return format(value, 'f')


@dataclass(frozen=True)
class Signal:
    trade: int
    side: int
    price: Decimal | float
    processing_duration_us: float


@dataclass
class SimulationResult:
    pnl: Decimal = Decimal(0)
    holding: int = 0
    open_position_price: Decimal | None = None
    buy_count: int = 0
    sell_count: int = 0
    ignored_duplicate_count: int = 0
    unfilled_orders: int = 0
    trigger_price_count: int = 0
    next_price_count: int = 0
    decisions: list[dict] | None = None


def simulate(signals: Iterable[Signal], prices: list[Decimal | float], rival_reaction_us: float) -> SimulationResult:
    """Apply one-BTC cash-flow rules; equality is too slow, final signal is unfilled."""
    if not math.isfinite(rival_reaction_us) or rival_reaction_us <= 0:
        raise ValueError('rival reaction time must be a finite positive number of microseconds')
    result = SimulationResult(decisions=[])
    ordered = list(signals)
    for signal in ordered:
        row = {
            'trade': signal.trade,
            'signal': 'BUY' if signal.side > 0 else 'SELL',
            'processing_duration_us': signal.processing_duration_us,
            'trigger_price': signal.price,
            'execution_price': '',
            'decision': '',
            'holding_before': result.holding,
            'holding_after': result.holding,
            'cash_pnl_after': result.pnl,
        }
        actionable = (signal.side > 0 and result.holding == 0) or (signal.side < 0 and result.holding == 1)
        if not actionable:
            result.ignored_duplicate_count += 1
            row['decision'] = 'ignored_position_rule'
            result.decisions.append(row)
            continue
        if signal.trade < 1 or signal.trade > len(prices):
            raise ValueError(f'signal trade ordinal {signal.trade} is outside the price data')
        if signal.trade == len(prices):
            result.unfilled_orders += 1
            row['decision'] = 'unfilled_no_next_trade'
            result.decisions.append(row)
            continue
        # The trigger price is attainable only when processing beats reaction;
        # equality and slower decisions execute at the following trade price.
        if signal.processing_duration_us < rival_reaction_us:
            execution_price = Decimal(str(signal.price))
            result.trigger_price_count += 1
            row['decision'] = 'filled_at_trigger_price'
        else:
            execution_price = Decimal(str(prices[signal.trade]))
            result.next_price_count += 1
            row['decision'] = 'filled_at_next_trade_price'
        row['execution_price'] = execution_price
        if signal.side > 0:
            result.pnl -= execution_price
            result.holding = 1
            result.open_position_price = execution_price
            result.buy_count += 1
        else:
            result.pnl += execution_price
            result.holding = 0
            result.open_position_price = None
            result.sell_count += 1
        row['holding_after'] = result.holding
        row['cash_pnl_after'] = result.pnl
        result.decisions.append(row)
    return result


def read_prices(dataset: Path) -> list[Decimal]:
    prices: list[Decimal] = []
    with dataset.open(newline='', encoding='utf-8-sig') as handle:
        rows = csv.reader(handle)
        first = next(rows, None)
        if first is None:
            raise ValueError(f'Dataset is empty: {dataset}')
        if any(field.strip().lower() == 'price' for field in first):
            price_column = next(i for i, field in enumerate(first) if field.strip().lower() == 'price')
        else:
            price_column = 1
            if len(first) <= price_column:
                raise ValueError(f'Dataset has no price column: {dataset}')
            prices.append(Decimal(first[price_column].strip()))
        for row in rows:
            if len(row) > price_column and row[price_column].strip():
                prices.append(Decimal(row[price_column].strip()))
    return prices


def read_replay_signals(dataset: Path, decisions_csv: Path, latency_csv: Path) -> tuple[list[Signal], list[float], list[int]]:
    prices = read_prices(dataset)
    with decisions_csv.open(newline='', encoding='utf-8') as handle:
        decisions = list(csv.DictReader(handle))
    with latency_csv.open(newline='', encoding='utf-8') as handle:
        latencies = list(csv.DictReader(handle))
    if len(decisions) != len(latencies):
        raise ValueError(f'Decision/latency row counts differ: {len(decisions)} vs {len(latencies)}')
    if len(decisions) > len(prices):
        raise ValueError('Replay logs contain more trades than the input dataset')
    signals: list[Signal] = []
    sequence: list[int] = []
    for index, (decision, latency) in enumerate(zip(decisions, latencies), 1):
        trade = int(decision['trade'])
        if trade != index or int(latency['trade']) != trade:
            raise ValueError(f'Replay rows are not aligned at ordinal {index}')
        value = int(decision['effective_decision'])
        sequence.append(value)
        if value:
            signals.append(Signal(trade, value, prices[trade - 1],
                                  int(latency['processing_duration_ns']) / 1000.0))
    return signals, prices, sequence


@dataclass(frozen=True)
class CompactSignal:
    trade: int
    side: int
    trigger_price: Decimal
    next_price: Decimal | None
    baseline_duration_us: float
    fast_duration_us: float


def _iter_dataset_prices(dataset: Path):
    with dataset.open(newline='', encoding='utf-8-sig') as handle:
        rows = csv.reader(handle)
        first = next(rows, None)
        if first is None:
            raise ValueError(f'Dataset is empty: {dataset}')
        if any(field.strip().lower() == 'price' for field in first):
            index = next(i for i, field in enumerate(first) if field.strip().lower() == 'price')
            for row in rows:
                if len(row) > index and row[index].strip():
                    yield Decimal(row[index].strip())
        else:
            if len(first) <= 1:
                raise ValueError(f'Dataset has no price column: {dataset}')
            yield Decimal(first[1].strip())
            for row in rows:
                if len(row) > 1 and row[1].strip():
                    yield Decimal(row[1].strip())


def stream_replay_pair(dataset: Path, baseline_decisions: Path, baseline_latency: Path,
                       fast_decisions: Path, fast_latency: Path, progress_log: Path,
                       progress_interval: int = 250_000) -> tuple[list[CompactSignal], int, int]:
    """Validate replay files in lockstep and retain actionable signals only."""
    sentry = object()
    records: list[CompactSignal] = []
    holding = 0
    processed = 0
    ignored_duplicates = 0
    with (baseline_decisions.open(newline='', encoding='utf-8') as bd, \
          baseline_latency.open(newline='', encoding='utf-8') as bl, \
          fast_decisions.open(newline='', encoding='utf-8') as fd, \
          fast_latency.open(newline='', encoding='utf-8') as fl, \
          progress_log.open('a', encoding='utf-8', buffering=1) as log):
        rows = [csv.DictReader(stream) for stream in (bd, bl, fd, fl)]
        prices = iter(_iter_dataset_prices(dataset))
        current_price = next(prices, None)
        next_price = next(prices, None)
        for ordinal, bundle in enumerate(zip_longest(*rows, fillvalue=sentry), 1):
            if any(row is sentry for row in bundle):
                raise ValueError(f'Replay file row counts differ at ordinal {ordinal}')
            bdec, blat, fdec, flat = bundle
            if current_price is None:
                raise ValueError('Replay logs contain more trades than the input dataset')
            if (int(bdec['trade']) != ordinal or int(fdec['trade']) != ordinal or
                    int(blat['trade']) != ordinal or int(flat['trade']) != ordinal):
                raise ValueError(f'Replay rows are not aligned at ordinal {ordinal}')
            bsignal, fsignal = int(bdec['effective_decision']), int(fdec['effective_decision'])
            if bsignal != fsignal:
                raise ValueError(f'Baseline and fast signal sequences differ at trade {ordinal}')
            if bsignal not in (-1, 0, 1):
                raise ValueError(f'Invalid effective decision at trade {ordinal}: {bsignal}')
            if bsignal and ((bsignal > 0 and holding == 0) or (bsignal < 0 and holding == 1)):
                records.append(CompactSignal(
                    ordinal, bsignal, current_price, next_price,
                    int(blat['processing_duration_ns']) / 1000.0,
                    int(flat['processing_duration_ns']) / 1000.0))
                holding = 1 if bsignal > 0 else 0
            elif bsignal:
                ignored_duplicates += 1
            processed = ordinal
            if ordinal % progress_interval == 0:
                record_size = sys.getsizeof(records[-1]) if records else 0
                estimate = sys.getsizeof(records) + len(records) * (record_size + 2 * sys.getsizeof(current_price))
                message = (f'progress baseline_rows={ordinal} fast_rows={ordinal} '
                           f'actionable_signals={len(records)} retained_memory_estimate_bytes={estimate}')
                print(message, file=sys.stderr, flush=True)
                log.write(message + '\n')
                log.flush()
                import os
                os.fsync(log.fileno())
            current_price = next_price
            next_price = next(prices, None)
        last_size = sys.getsizeof(records[-1]) if records else 0
        estimate = sys.getsizeof(records) + len(records) * (last_size + 2 * sys.getsizeof(current_price))
        log.write(f'complete baseline_rows={processed} fast_rows={processed} '
                  f'actionable_signals={len(records)} retained_memory_estimate_bytes={estimate}\n')
        log.flush()
        import os
        os.fsync(log.fileno())
    return records, processed, ignored_duplicates


def compact_simulate(records: list[CompactSignal], version: str, reaction_us: float,
                     ignored_duplicate_count: int = 0) -> dict:
    if not math.isfinite(reaction_us) or reaction_us <= 0:
        raise ValueError('rival reaction time must be a finite positive number of microseconds')
    duration_name = 'baseline_duration_us' if version == 'baseline' else 'fast_duration_us'
    pnl = Decimal(0)
    holding = 0
    opened = None
    buys = sells = trigger_count = next_count = unfilled = 0
    for signal in records:
        duration = getattr(signal, duration_name)
        if signal.next_price is None:
            unfilled += 1
            decision, execution = 'unfilled_no_next_trade', ''
        else:
            execution = signal.trigger_price if duration < reaction_us else signal.next_price
            decision = ('filled_at_trigger_price' if duration < reaction_us
                        else 'filled_at_next_trade_price')
            if decision == 'filled_at_trigger_price':
                trigger_count += 1
            else:
                next_count += 1
            if signal.side > 0:
                pnl -= execution
                opened = execution
                holding = 1
                buys += 1
            else:
                pnl += execution
                opened = None
                holding = 0
                sells += 1
    return {'pnl': pnl, 'holding': holding, 'open_position_price': opened,
            'buy_count': buys, 'sell_count': sells,
            'ignored_duplicate_count': ignored_duplicate_count,
            'unfilled_orders': unfilled, 'trigger_price_count': trigger_count,
            'next_price_count': next_count}


def write_compact_decisions(records: list[CompactSignal], path: Path, reaction_us: float) -> None:
    fields = ['version', 'trade', 'signal', 'processing_duration_us', 'trigger_price',
              'next_trade_price', 'execution_price', 'decision', 'holding_before',
              'holding_after', 'cash_pnl_after']
    with path.open('w', newline='', encoding='utf-8') as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        for version in ('baseline', 'fast'):
            holding, pnl = 0, Decimal(0)
            duration_name = f'{version}_duration_us'
            for signal in records:
                before = holding
                duration = getattr(signal, duration_name)
                if signal.next_price is None:
                    decision, execution = 'unfilled_no_next_trade', ''
                else:
                    execution = signal.trigger_price if duration < reaction_us else signal.next_price
                    decision = ('filled_at_trigger_price' if duration < reaction_us
                                else 'filled_at_next_trade_price')
                    if signal.side > 0:
                        pnl -= execution
                        holding = 1
                    else:
                        pnl += execution
                        holding = 0
                writer.writerow({'version': version, 'trade': signal.trade,
                                 'signal': 'BUY' if signal.side > 0 else 'SELL',
                                 'processing_duration_us': duration,
                                 'trigger_price': signal.trigger_price,
                                 'next_trade_price': signal.next_price if signal.next_price is not None else '',
                                 'execution_price': execution, 'decision': decision,
                                 'holding_before': before, 'holding_after': holding,
                                 'cash_pnl_after': pnl})
        handle.flush()
        import os
        os.fsync(handle.fileno())


def compact_sweep(records: list[CompactSignal], ignored_duplicate_count: int = 0) -> list[dict]:
    rows = []
    for reaction_us in REACTION_SWEEP_US:
        base = compact_simulate(records, 'baseline', reaction_us, ignored_duplicate_count)
        fast = compact_simulate(records, 'fast', reaction_us, ignored_duplicate_count)
        rows.append({'rival_reaction_us': reaction_us,
                     'baseline_pnl': base['pnl'], 'fast_pnl': fast['pnl'],
                     'money_lost': fast['pnl'] - base['pnl'],
                     'baseline_trigger_price_count': base['trigger_price_count'],
                     'baseline_next_price_count': base['next_price_count'],
                     'fast_trigger_price_count': fast['trigger_price_count'],
                     'fast_next_price_count': fast['next_price_count'],
                     'baseline_unfilled_orders': base['unfilled_orders'],
                     'fast_unfilled_orders': fast['unfilled_orders']})
    return rows


def sweep(signals: list[Signal], prices: list[Decimal | float]) -> list[dict]:
    rows = []
    for reaction_us in REACTION_SWEEP_US:
        result = simulate(signals, prices, reaction_us)
        rows.append({
            'rival_reaction_us': reaction_us,
            'pnl': result.pnl,
            'trigger_price_count': result.trigger_price_count,
            'next_price_count': result.next_price_count,
            'unfilled_orders': result.unfilled_orders,
            'ignored_duplicate_count': result.ignored_duplicate_count,
            'open_position': result.holding,
        })
    return rows


def write_sweep_svg(path: Path, rows: list[dict], baseline_name: str, fast_name: str) -> None:
    """Write a dependency-free SVG reaction/P&L graph."""
    width, height = 900, 540
    left, right, top, bottom = 90, 35, 50, 85
    plot_w, plot_h = width - left - right, height - top - bottom
    values = [float(row[key]) for row in rows for key in ('baseline_pnl', 'fast_pnl', 'money_lost')]
    low, high = min(values, default=0.0), max(values, default=0.0)
    if high == low:
        high, low = high + 1.0, low - 1.0
    x0, x1 = math.log10(rows[0]['rival_reaction_us']), math.log10(rows[-1]['rival_reaction_us'])

    def xy(x: float, y: float) -> tuple[float, float]:
        y = float(y)
        px = left + (math.log10(x) - x0) / (x1 - x0) * plot_w
        py = top + (high - y) / (high - low) * plot_h
        return px, py

    elements = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
                '<rect width="100%" height="100%" fill="white"/>',
                f'<text x="{width/2}" y="25" text-anchor="middle" font-family="sans-serif" font-size="18">Cash P&amp;L by rival reaction time</text>',
                f'<path d="M {left} {top} V {height-bottom} H {width-right}" fill="none" stroke="#333"/>',
                f'<text x="{width/2}" y="{height-25}" text-anchor="middle" font-family="sans-serif">Rival reaction time (microseconds, log scale)</text>',
                f'<text x="22" y="{height/2}" transform="rotate(-90 22 {height/2})" text-anchor="middle" font-family="sans-serif">Cash P&amp;L (price units)</text>']
    for reaction in (0.1, 1.0, 10.0, 100.0):
        x, _ = xy(reaction, low)
        elements.append(f'<path d="M {x:.1f} {top} V {height-bottom}" stroke="#ddd"/>')
        elements.append(f'<text x="{x:.1f}" y="{height-bottom+22}" text-anchor="middle" font-family="sans-serif">{reaction:g}</text>')
    for key, color, label in (('baseline_pnl', '#c0392b', baseline_name),
                              ('fast_pnl', '#2471a3', fast_name),
                              ('money_lost', '#239b56', 'money lost (fast - baseline)')):
        points = ' '.join(f'{xy(row["rival_reaction_us"], row[key])[0]:.1f},{xy(row["rival_reaction_us"], row[key])[1]:.1f}' for row in rows)
        elements.append(f'<polyline points="{points}" fill="none" stroke="{color}" stroke-width="3"/>')
        elements.append(f'<text x="{left+12 if key == "baseline_pnl" else left+260}" y="{top+20}" fill="{color}" font-family="sans-serif">{label}</text>')
    elements.append('</svg>')
    path.write_text('\n'.join(elements) + '\n', encoding='utf-8')
