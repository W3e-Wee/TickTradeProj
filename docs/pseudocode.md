# Pseudocode

## Stage 0 sum

```
read N
if N > safe recursion threshold:
    print honest message and continue with iterative sum
else:
    recursively compute sum(1..N)
print iterative_sum and recursive status
```

## Stage A market baseline

```
open CSV
for each record in file:
    parse trade_id, price, qty, quoteQty, time, isBuyerMaker
    validate fields and quantity > 0
    update cumulative numerator and denominator
    update short and long circular price windows
    if both windows are full:
        compute short_sma and long_sma
        delta = short_sma - long_sma
        if previous_delta <= 0 and delta > 0 => BUY
        if previous_delta >= 0 and delta < 0 => SELL
        store previous_delta
    update OHLC bar for the timestamp second
flush final bar at EOF
print final VWAP and signal counts
```
