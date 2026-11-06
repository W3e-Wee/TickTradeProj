# Rival sweep plot template; expects results/rival/sweep.csv.
set terminal pngcairo
set output "results/plots/money_lost.png"
set logscale x
set xlabel "Rival reaction time (microseconds)"
set ylabel "Money lost"
plot "results/rival/sweep.csv" using 1:3 with linespoints title "Money lost"
