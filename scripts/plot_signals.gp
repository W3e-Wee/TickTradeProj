load "scripts/common.gp"
set datafile separator whitespace
stats "results/plots/signals.dat" using 1 nooutput
row_count = STATS_records
xmin = STATS_min
xmax = STATS_max
stats "results/plots/signals.dat" using 2 nooutput
price_min = STATS_min
price_max = STATS_max
stats "results/plots/signals.dat" using 3 nooutput
short_min = STATS_min
short_max = STATS_max
stats "results/plots/signals.dat" using 4 nooutput
long_min = STATS_min
long_max = STATS_max
raw_min = (price_min < short_min ? (price_min < long_min ? price_min : long_min) : (short_min < long_min ? short_min : long_min))
raw_max = (price_max > short_max ? (price_max > long_max ? price_max : long_max) : (short_max > long_max ? short_max : long_max))
ymargin = (raw_max - raw_min) * 0.05
if (ymargin <= 0) ymargin = abs(raw_max) * 0.05 + 0.01
ymin = raw_min - ymargin
ymax = raw_max + ymargin
stats "results/plots/signal_points.dat" using ($3 == 1 ? 1 : 1/0) nooutput
buy_count = STATS_records
stats "results/plots/signal_points.dat" using ($3 == -1 ? 1 : 1/0) nooutput
sell_count = STATS_records
stats "results/plots/signal_points.dat" using 1 nooutput
points_count = STATS_records
set print "results/logs/plot_signals_summary.txt"
print sprintf("rows=%d", row_count)
print sprintf("points_trade_price=%d", row_count)
print sprintf("points_short_sma=%d", row_count)
print sprintf("points_long_sma=%d", row_count)
print sprintf("points_buy=%d", buy_count)
print sprintf("points_sell=%d", sell_count)
print sprintf("xmin=%.6f", xmin)
print sprintf("xmax=%.6f", xmax)
print sprintf("ymin=%.6f", ymin)
print sprintf("ymax=%.6f", ymax)
set print
set output "results/plots/signals.png"
set datafile separator whitespace
set title "Trade Price and SMA Crossovers"
set xlabel "Trade number"
set ylabel "Price (USDT)"
set yrange [ymin:ymax]
set xrange [xmin:xmax]
plot "results/plots/signals.dat" using 1:2 with lines ls 3 lw 0.5 title "Trade price", \
     "results/plots/signals.dat" using 1:3 with lines ls 1 title "Short SMA", \
     "results/plots/signals.dat" using 1:4 with lines ls 2 title "Long SMA", \
     "results/plots/signal_points.dat" using ($3 == 1 ? $1 : 1/0):($3 == 1 ? $2 : 1/0) with points pt 9 ps 1.8 lc rgb "#009E73" title "BUY", \
     "results/plots/signal_points.dat" using ($3 == -1 ? $1 : 1/0):($3 == -1 ? $2 : 1/0) with points pt 11 ps 1.8 lc rgb "#D55E00" title "SELL"
unset output
