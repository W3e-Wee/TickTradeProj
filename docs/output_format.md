# Output format

The required engine contract is:

1. A human-readable block (title, build label, parameters).
2. Exactly one machine-readable line of space-separated `key=value` fields:

```text
RESULT version=<id> file=<name> trades=<n> short=<n> long=<n> vwap=<%.6f> bars=<n> buys=<n> sells=<n> signal_hash=<16 hex> load_ns=<n> process_ns=<n> total_ns=<n> peak_mem_kb=<n>
```

3. A final `PASS` or `FAIL <reason>` line for built-in self-checks.

The signal hash is FNV-1a 64-bit over each trade index encoded as eight
little-endian bytes followed by one side byte. Cross-version equality checks
compare trade count, bars, buys, sells, and hash exactly; VWAP uses relative
tolerance `1e-6`.

Generated CSV files have one header row written once. Stage 0 rows are
`opt_level,mode,n,repeats,best_ns,median_ns,worst_ns,ns_per_add,sum` for the
loop benchmark and `opt_level,mode,n,reps,batches,iterative_median_ns,recursive_median_ns,ratio` for
the recursion benchmark. Parser comparison rows are
`file,rows,repeats,sscanf_median_ns,fast_median_ns,speedup`. Rival sweep rows are
`reaction_us,pnl,money_lost,trades`. Signal plot data has no header and stores
`trade_index,price,short_sma,long_sma,signal` for gnuplot.
