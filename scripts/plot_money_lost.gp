load "scripts/common.gp"
set datafile separator comma
set key autotitle columnhead
stats "results/rival/sweep.csv" using (column("reaction_us")) nooutput
row_count = STATS_records
xmin = STATS_min
xmax = STATS_max
stats "results/rival/sweep.csv" using (column("money_lost")) nooutput
raw_min = STATS_min
raw_max = STATS_max
ymargin = (raw_max - raw_min) * 0.05
if (ymargin <= 0) ymargin = abs(raw_max) * 0.05 + 0.01
ymin = raw_min - ymargin
ymax = raw_max + ymargin
set print "results/logs/plot_money_lost_summary.txt"
print sprintf("rows=%d", row_count)
print sprintf("points_money_lost=%d", row_count)
print sprintf("xmin=%.9g", xmin)
print sprintf("xmax=%.9g", xmax)
print sprintf("ymin=%.9g", ymin)
print sprintf("ymax=%.9g", ymax)
set print
set output "results/plots/money_lost.png"
set logscale x
set title "Money Lost to a Faster Rival"
set xlabel "Rival reaction time (microseconds)"
set ylabel "Money lost (USDT)"
set xrange [xmin:xmax]
set yrange [ymin:ymax]
plot "results/rival/sweep.csv" using (column("reaction_us")):(column("money_lost")) with linespoints ls 1 title "Money lost"
unset output
