load "scripts/common.gp"
input_file = "results/bench/ladder_plot_input.csv"
summary_file = "results/logs/plot_ladder_summary.txt"
output_file = "results/plots/ladder.png"
set datafile separator comma
set key autotitle columnhead
stats input_file using (column("median_total_ns")) nooutput
row_count = STATS_records
xmin = 0
xmax = row_count - 1
raw_min = STATS_min
raw_max = STATS_max
ymargin = (raw_max - raw_min) * 0.05
if (ymargin <= 0) ymargin = abs(raw_max) * 0.05 + 0.01
ymin = raw_min - ymargin
ymax = raw_max + ymargin
set print summary_file
print sprintf("rows=%d", row_count)
print sprintf("points_median_total_ns=%d", row_count)
print sprintf("xmin=%.6f", xmin)
print sprintf("xmax=%.6f", xmax)
print sprintf("ymin=%.6f", ymin)
print sprintf("ymax=%.6f", ymax)
set print
set datafile separator comma
set key autotitle columnhead
set output output_file
set title "Tick-to-Trade Efficiency Ladder"
set xlabel "Engine version"
set ylabel "Median total time (ns)"
set xtics rotate by -45
set xrange [xmin:xmax]
set yrange [ymin:ymax]
plot input_file using 0:(column("median_total_ns")):xtic(column("version")) with linespoints ls 1 title "Median total time"
unset output
