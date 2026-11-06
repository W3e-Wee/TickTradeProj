"""Compare the optional Python reference with a selected C ladder binary."""
import argparse
import re
import subprocess

from reference_engine import summarize


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--file", required=True)
    parser.add_argument("--c-engine", required=True)
    parser.add_argument("--short", type=int, required=True)
    parser.add_argument("--long", type=int, required=True)
    args = parser.parse_args()
    command = [args.c_engine, "--file", args.file, "--short", str(args.short),
               "--long", str(args.long)]
    completed = subprocess.run(command, check=True, capture_output=True, text=True)
    match = re.search(r"^RESULT .* trades=(\d+).* vwap=([0-9.]+) bars=(\d+) buys=(\d+) sells=(\d+)",
                      completed.stdout, re.MULTILINE)
    if match is None:
        raise SystemExit("C engine did not emit a RESULT record")
    c_values = tuple(map(int, (match.group(1), match.group(3), match.group(4), match.group(5))))
    reference = summarize(args.file, args.short, args.long)
    py_values = tuple(reference[key] for key in ("trades", "bars", "buys", "sells"))
    ok = c_values == py_values and abs(float(match.group(2)) - reference["vwap"]) <= abs(reference["vwap"]) * 1e-6
    if not ok:
        print(f"C={c_values} vwap={match.group(2)} Python={py_values} vwap={reference['vwap']:.6f}")
    print("python-reference " + ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
