#!/usr/bin/env python3
"""NumPy-vectorised Stage A. Usage: python stage_a_numpy.py --file CSV [options]. Exit codes: 0 success, 2 bad input, 3 out of range, 4 mismatch, 6 I/O error; missing NumPy exits 0 with SKIPPED."""
import argparse
import csv
import math
import os
import re
import statistics
import sys
import time

try:
    import numpy as np
except ImportError:
    print("SKIPPED: numpy not installed")
    sys.exit(0)

SCALE = 100_000_000
# Group 05 defaults; Make targets pass explicit windows when configured.
DEFAULT_SHORT_WINDOW = 30
DEFAULT_LONG_WINDOW = 60
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


def parse_scaled(text):
    """Convert decimal bytes from the CSV into an exact integer scaled by 10^8."""
    if isinstance(text, bytes):
        text = text.decode("ascii")
    text = text.strip()
    if not re.fullmatch(r"\d+(?:\.\d{1,8})?", text):
        raise ValueError("invalid nonnegative decimal: " + text)
    whole, dot, fraction = text.partition(".")
    value = int(whole) * SCALE + int((fraction + "00000000")[:8] if dot else 0)
    if value > MAX_INT64:
        raise OverflowError("scaled decimal exceeds signed 64-bit range")
    return value


def fnv_signal(hash_value, index, side):
    """Hash the one-based trade index and C-compatible signed side byte."""
    for byte in range(8):
        hash_value ^= (index >> (8 * byte)) & 0xFF
        hash_value = (hash_value * FNV_PRIME) & MASK64
    hash_value ^= side & 0xFF
    return (hash_value * FNV_PRIME) & MASK64


def load_columns(path, limit):
    """Load exact scaled price, quantity, and timestamp columns with NumPy."""
    with open(path, "rb") as input_file:
        first = input_file.readline()
    skiprows = 0 if first and first.lstrip()[:1].isdigit() else 1
    columns = np.loadtxt(path, delimiter=",", usecols=(1, 2, 4), dtype=np.int64,
                         skiprows=skiprows,
                         converters={1: parse_scaled, 2: parse_scaled, 4: int},
                         max_rows=limit if limit else None, ndmin=2)
    if columns.shape[0] == 0:
        raise ValueError("input contains no trade rows")
    prices, quantities, timestamps = columns[:, 0], columns[:, 1], columns[:, 2]
    if np.any(prices <= 0) or np.any(quantities <= 0) or np.any(timestamps < 0):
        raise ValueError("price, quantity, and timestamp must be positive")
    return prices, quantities, timestamps


def rolling_sums(values, window):
    """Return exact window sums, using object integers if int64 could overflow."""
    # Wrapped int64 prefixes cancel modulo 2^64 when each actual window fits.
    if int(values.max()) > np.iinfo(np.int64).max // window:
        prefix = np.cumsum(values.astype(object), dtype=object)
        zero = np.zeros(1, dtype=object)
    else:
        prefix = np.cumsum(values, dtype=np.int64)
        zero = np.zeros(1, dtype=np.int64)
    prior = np.concatenate((zero, prefix[:-window]))
    return prefix[window - 1:] - prior


def run_engine(path, short_window, long_window, limit):
    """Vectorise rolling signals and bars, returning Stage A-compatible totals."""
    total_started = time.perf_counter_ns()
    load_started = time.perf_counter_ns()
    prices, quantities, timestamps = load_columns(path, limit)
    load_ns = time.perf_counter_ns() - load_started
    process_started = time.perf_counter_ns()
    short_sums = rolling_sums(prices, short_window)
    long_sums = rolling_sums(prices, long_window)
    aligned_short = short_sums[long_window - short_window:]
    cross_bound = int(prices.max()) * short_window * long_window
    if cross_bound <= np.iinfo(np.int64).max:
        differences = aligned_short * long_window - long_sums * short_window
    else:
        # Exact object arithmetic is the safe fallback for unusually large windows.
        differences = aligned_short.astype(object) * long_window - long_sums.astype(object) * short_window
    signs = np.sign(differences)
    signal_indexes = []
    signal_sides = []
    for offset in range(1, len(signs)):
        if signs[offset - 1] <= 0 and signs[offset] > 0:
            signal_indexes.append(long_window + offset)
            signal_sides.append(1)
        elif signs[offset - 1] >= 0 and signs[offset] < 0:
            signal_indexes.append(long_window + offset)
            signal_sides.append(-1)
    hash_value = FNV_OFFSET
    buys = sum(side > 0 for side in signal_sides)
    sells = sum(side < 0 for side in signal_sides)
    # Only signal events need this small loop; arithmetic over all trades is vectorised.
    for trade_index, side in zip(signal_indexes, signal_sides):
        hash_value = fnv_signal(hash_value, trade_index, side)
    seconds = timestamps // 1_000_000
    bar_starts = np.concatenate((np.zeros(1, dtype=np.int64),
                                 np.flatnonzero(seconds[1:] != seconds[:-1]) + 1))
    bar_ends = np.concatenate((bar_starts[1:], np.array([timestamps.size])))
    # Calculate every OHLC bar with array operations; the final group is flushed.
    price_float = prices.astype(np.float64) / SCALE
    qty_float = quantities.astype(np.float64) / SCALE
    opens = price_float[bar_starts]
    closes = price_float[bar_ends - 1]
    highs = np.maximum.reduceat(price_float, bar_starts)
    lows = np.minimum.reduceat(price_float, bar_starts)
    bars = int(opens.size)
    _ = highs, lows, closes  # Kept as the computed OHLC series for Stage A semantics.
    # VWAP uses double reporting arithmetic after integer-only signal decisions.
    vwap = float(np.sum(price_float * qty_float, dtype=np.float64) /
                 np.sum(qty_float, dtype=np.float64))
    process_ns = time.perf_counter_ns() - process_started
    total_ns = time.perf_counter_ns() - total_started
    return {"trades": int(prices.size), "vwap": vwap, "bars": bars,
            "buys": buys, "sells": sells, "signal_hash": "%016x" % hash_value,
            "process_ns": process_ns, "load_ns": load_ns, "total_ns": total_ns}


def parse_result(path):
    """Parse key/value fields from the first RESULT line in C output."""
    with open(path, "r", encoding="utf-8") as captured:
        for line in captured:
            if line.startswith("RESULT "):
                return dict(RESULT_PATTERN.findall(line))
    raise ValueError("comparison file has no RESULT line")


def compare_result(python_result, c_result):
    """Return the name of the first C output field that differs."""
    for field in ("trades", "bars", "buys", "sells", "signal_hash"):
        if str(python_result[field]) != c_result.get(field):
            return field
    if not math.isclose(python_result["vwap"], float(c_result["vwap"]), rel_tol=1e-6):
        return "vwap"
    return None


def append_csv(path, row):
    """Append one NumPy measurement and create the schema header if needed."""
    new_file = not os.path.exists(path) or os.path.getsize(path) == 0
    with open(path, "a", newline="", encoding="utf-8") as output_file:
        writer = csv.writer(output_file)
        if new_file:
            writer.writerow(CSV_HEADER.split(","))
        writer.writerow(row)


def arguments(argv):
    """Define shared options and reject invalid window or repeat values."""
    parser = argparse.ArgumentParser(description="NumPy-vectorised Stage A")
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
    if options.short <= 0 or options.long < options.short or options.long > 100000:
        parser.error("require 0 < short <= long <= 100000")
    if options.limit < 0 or options.repeat <= 0:
        parser.error("limit must be nonnegative and repeat must be positive")
    return options


def main(argv=None):
    """Run repeated vector calculations, compare optionally, and print RESULT."""
    try:
        options = arguments(argv)
        if options.short <= 0 or options.long < options.short or options.long > 100000:
            print("Window sizes must satisfy 0 < short <= long <= 100000", file=sys.stderr)
            return EXIT_OUT_OF_RANGE
        if not os.path.isfile(options.file):
            print("ERROR: missing dataset: " + options.file, file=sys.stderr)
            return EXIT_IO_ERROR
        results = [run_engine(options.file, options.short, options.long, options.limit)
                   for _ in range(options.repeat)]
        result = results[-1]
        median_total = int(statistics.median(item["total_ns"] for item in results))
        result["process_ns"] = int(statistics.median(item["process_ns"] for item in results))
        result["load_ns"] = int(statistics.median(item["load_ns"] for item in results))
        result["total_ns"] = median_total
        c_record = parse_result(options.compare_c) if options.compare_c else None
        mismatch = compare_result(result, c_record) if c_record and not options.limit else None
        print("Tick-to-Trade Python Stage A (NumPy)")
        print("file=%s short=%d long=%d repeat=%d numpy=%s" % (
            options.file, options.short, options.long, options.repeat, np.__version__))
        print("RESULT version=py_numpy file=%s trades=%d short=%d long=%d vwap=%.6f bars=%d buys=%d sells=%d signal_hash=%s load_ns=%d process_ns=%d total_ns=%d peak_mem_kb=NA" % (
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
        row = [options.platform, options.member, "py_numpy", options.file, result["trades"], options.short, options.long, options.limit or "", options.repeat, median_total, c_total or "NA", ratio, matches, sys.version.split()[0], np.__version__]
        append_csv(options.csv, row)
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
