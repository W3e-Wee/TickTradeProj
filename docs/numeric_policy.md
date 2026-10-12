# Numeric policy

- The market engine uses IEEE-754 `double` arithmetic for VWAP and SMA output.
- Quantities are validated for positivity before any balance update.
- The assignment expects cumulative VWAP as `sum(price * qty) / sum(qty)` and this is implemented directly in streaming form.
- The Stage 0 arithmetic intentionally avoids `volatile` or other tricks that would distort compiler optimisations.
- The recursive arithmetic is not forced to complete when the recursion depth exceeds the safe stack budget; the program reports that condition honestly.
