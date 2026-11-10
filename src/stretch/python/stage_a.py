#!/usr/bin/env python3
"""Pure Python Stage A reference. Usage: python stage_a.py --file CSV [options]. Exit codes: 0 success, 2 bad input, 3 out of range, 4 mismatch, 6 I/O error, 7 unsupported."""
import argparse
import csv
import math
import os
import re
import statistics
import sys
import time

SCALE = 100_000_000
MAX_INT64 = (1 << 63) - 1
MASK64 = (1 << 64) - 1
FNV_OFFSET = 14695981039346656037
FNV_PRIME = 1099511628211
EXIT_BAD_INPUT = 2
EXIT_OUT_OF_RANGE = 3
EXIT_MISMATCH = 4
EXIT_IO_ERROR = 6
RESULT_PATTERN = re.compile(r"([A-Za-z_]+)=([^ ]+)")
CSV_HEADER = "platform,member,implementation,file,trades,short,long,limit,repeats,median_total_ns,c_total_ns,times_slower_than_c,matches_c,python_version,numpy_version"
# Group 05 defaults; Make targets pass explicit windows when configured.
DEFAULT_SHORT_WINDOW = 30
DEFAULT_LONG_WINDOW = 60


def parse_scaled(text):
    """Convert a decimal field to an exact integer scaled by 10^8."""
    text = text.strip()
    if not re.fullmatch(r"\d+(?:\.\d{1,8})?", text):
        raise ValueError("invalid nonnegative decimal: " + text)
    whole, dot, fraction = text.partition(".")
    value = int(whole) * SCALE + int((fraction + "00000000")[:8] if dot else 0)
    if value > MAX_INT64:
        raise OverflowError("scaled decimal exceeds signed 64-bit range")
    return value


def fnv_signal(hash_value, index, side):
    """Hash a one-based trade index and signed BUY/SELL byte like C."""
    for byte in range(8):
        hash_value ^= (index >> (8 * byte)) & 0xFF
        hash_value = (hash_value * FNV_PRIME) & MASK64
    hash_value ^= side & 0xFF
    return (hash_value * FNV_PRIME) & MASK64


def run_engine(path, short_window, long_window, limit):
    """Stream rows and return Stage A totals, exact signals, bars, and timings."""
    started = time.perf_counter_ns()
    prices_short = [0] * short_window
    prices_long = [0] * long_window
    short_sum = 0
    long_sum = 0
    qty_total = 0.0
    quote_total = 0.0
    hash_value = FNV_OFFSET
    buys = 0
    sells = 0
    bars = 0
    previous_sign = 0
    signal_ready = False
    previous_second = None
    trades = 0
    bar_open = 0
    bar_high = 0
    bar_low = 0
    bar_close = 0
    with open(path, "r", newline="", encoding="utf-8") as input_file:
        rows = csv.reader(input_file)
        for row in rows:
            if not row:
                continue
            if not row[0].strip().isdigit():
                if trades == 0:
                    continue  # The provided CSV may have one nonnumeric header.
                raise ValueError("nonnumeric trade row after CSV header")
            if len(row) < 5:
                raise ValueError("trade row has fewer than five columns")
            price_scaled = parse_scaled(row[1])
            qty_scaled = parse_scaled(row[2])
            timestamp = int(row[4].strip())
            if price_scaled <= 0 or qty_scaled <= 0 or timestamp < 0:
                raise ValueError("price, quantity, and timestamp must be positive")
            price_text = row[1].strip()
            qty_text = row[2].strip()
            price_value = float(price_text)
            qty_value = float(qty_text)
            trades += 1
            short_slot = (trades - 1) % short_window
            long_slot = (trades - 1) % long_window
            if trades > short_window:
                short_sum -= prices_short[short_slot]
            if trades > long_window:
                long_sum -= prices_long[long_slot]
            prices_short[short_slot] = price_scaled
            prices_long[long_slot] = price_scaled
            short_sum += price_scaled
            long_sum += price_scaled
            quote_total += price_value * qty_value
            qty_total += qty_value
            if trades >= long_window:
                # Integer operands preserve the C cross-multiplication decision exactly.
                difference = short_sum * long_window - long_sum * short_window
                sign = (difference > 0) - (difference < 0)
                if signal_ready:
                    side = 1 if previous_sign <= 0 and sign > 0 else -1 if previous_sign >= 0 and sign < 0 else 0
                    if side > 0:
                        buys += 1
                        hash_value = fnv_signal(hash_value, trades, side)
                    elif side < 0:
                        sells += 1
                        hash_value = fnv_signal(hash_value, trades, side)
                previous_sign = sign
                signal_ready = True
            second = timestamp // 1_000_000
            if previous_second is None:
                bar_open = bar_high = bar_low = bar_close = price_scaled
                previous_second = second
            elif second != previous_second:
                bars += 1  # Close the previous second's OHLC bar.
                bar_open = bar_high = bar_low = bar_close = price_scaled
                previous_second = second
            else:
                bar_high = max(bar_high, price_scaled)
                bar_low = min(bar_low, price_scaled)
                bar_close = price_scaled
            if limit and trades >= limit:
                break
    if previous_second is not None:
        bars += 1  # Flush the final open bar, as Stage A does.
    elapsed = time.perf_counter_ns() - started
    if trades == 0:
        raise ValueError("input contains no trade rows")
    return {
        "trades": trades,
        "vwap": quote_total / qty_total if qty_total else 0.0,
        "bars": bars,
        "buys": buys,
        "sells": sells,
        "signal_hash": "%016x" % hash_value,
        "process_ns": elapsed,
        "load_ns": 0,
        "total_ns": elapsed,
    }


def parse_result(path):
    """Read and parse the RESULT key/value record from captured C output."""
    with open(path, "r", encoding="utf-8") as captured:
        for line in captured:
            if line.startswith("RESULT "):
                return dict(RESULT_PATTERN.findall(line))
    raise ValueError("comparison file has no RESULT line")


def compare_result(python_result, c_result):
    """Return the first field that differs from the captured C result."""
    for field in ("trades", "bars", "buys", "sells", "signal_hash"):
        if str(python_result[field]) != c_result.get(field):
            return field
    c_vwap = float(c_result["vwap"])
    if not math.isclose(python_result["vwap"], c_vwap, rel_tol=1e-6, abs_tol=0.0):
        return "vwap"
    return None


def append_csv(path, row):
    """Append one measurement row, writing the header on a new file."""
    new_file = not os.path.exists(path) or os.path.getsize(path) == 0
    with open(path, "a", newline="", encoding="utf-8") as output_file:
        writer = csv.writer(output_file)
        if new_file:
            writer.writerow(CSV_HEADER.split(","))
        writer.writerow(row)


def arguments(argv):
    """Define and validate the shared command-line arguments."""
    parser = argparse.ArgumentParser(description="Pure Python streaming Stage A reference")
    parser.add_argument("--file", required=True)
    parser.add_argument("--short", type=int, default=DEFAULT_SHORT_WINDOW)
    parser.add_argument("--long", type=int, default=DEFAULT_LONG_WINDOW)
    parser.add_argument("--limit", type=int, default=0)
    parser.add_argument("--repeat", type=int, default=3)
    parser.add_argument("--compare-c")
    parser.add_argument("--csv", default="results/bench/c_vs_python.csv")
    parser.add_argument("--member", default="member")
    parser.add_argument("--platform", default="unknown")
    options = parser.parse_args(argv)
    if options.limit < 0 or options.repeat <= 0:
        parser.error("limit must be nonnegative and repeat must be positive")
    return options


def main(argv=None):
    """Run repeats, compare optionally with C, print RESULT, and append CSV data."""
    try:
        options = arguments(argv)
        if options.short <= 0 or options.long < options.short or options.long > 100000:
            print("Window sizes must satisfy 0 < short <= long <= 100000", file=sys.stderr)
            return EXIT_OUT_OF_RANGE
        if not os.path.isfile(options.file):
            print("ERROR: missing dataset: " + options.file, file=sys.stderr)
            return EXIT_IO_ERROR
        results = [run_engine(options.file, options.short, options.long, options.limit) for _ in range(options.repeat)]
        result = results[-1]
        median_total = int(statistics.median(item["total_ns"] for item in results))
        result["process_ns"] = int(statistics.median(item["process_ns"] for item in results))
        result["total_ns"] = median_total
        c_record = parse_result(options.compare_c) if options.compare_c else None
        mismatch = compare_result(result, c_record) if c_record and not options.limit else None
        print("Tick-to-Trade Python Stage A (pure Python)")
        print("file=%s short=%d long=%d repeat=%d" % (options.file, options.short, options.long, options.repeat))
        print("RESULT version=py_pure file=%s trades=%d short=%d long=%d vwap=%.6f bars=%d buys=%d sells=%d signal_hash=%s load_ns=%d process_ns=%d total_ns=%d peak_mem_kb=NA" % (
            options.file, result["trades"], options.short, options.long, result["vwap"], result["bars"], result["buys"], result["sells"], result["signal_hash"], result["load_ns"], result["process_ns"], result["total_ns"]))
        if options.limit and options.compare_c:
            print("compare=SKIPPED(limit)")
        if mismatch:
            print("FAIL mismatch: " + mismatch)
            matches = "0"
        else:
            print("PASS")
            matches = "1" if c_record and not options.limit else "NA"
        c_total = int(c_record["total_ns"]) if c_record else None
        ratio = median_total / c_total if c_total else "NA"
        if c_record:
            c_row = [options.platform, options.member, "c", options.file, c_record.get("trades", "NA"), options.short, options.long, options.limit or "", 1, c_total, c_total, 1.0, "1" if not mismatch else "0", sys.version.split()[0], "NA"]
            append_csv(options.csv, c_row)
        py_row = [options.platform, options.member, "py_pure", options.file, result["trades"], options.short, options.long, options.limit or "", options.repeat, median_total, c_total or "NA", ratio, matches, sys.version.split()[0], "NA"]
        append_csv(options.csv, py_row)
        return EXIT_MISMATCH if mismatch else 0
    except FileNotFoundError as error:
        print("I/O error: " + str(error), file=sys.stderr)
        return EXIT_IO_ERROR
    except (OSError, csv.Error) as error:
        print("I/O error: " + str(error), file=sys.stderr)
        return EXIT_IO_ERROR
    except OverflowError as error:
        print("Out of range: " + str(error), file=sys.stderr)
        return EXIT_OUT_OF_RANGE
    except ValueError as error:
        print("Bad input: " + str(error), file=sys.stderr)
        return EXIT_BAD_INPUT


if __name__ == "__main__":
    sys.exit(main())
