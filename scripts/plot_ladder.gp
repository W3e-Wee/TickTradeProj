# Ladder benchmark chart; input and output are set by make plot-ladder.
set datafile separator comma
set datafile commentschars "#"
set terminal pngcairo
set title "Tick-to-Trade efficiency ladder"
set xlabel "Engine version"
set ylabel "Median total time (ns)"
set xtics rotate by -45
set key off
plot input every ::1 using 0:7:xtic(3) with linespoints pointtype 7
