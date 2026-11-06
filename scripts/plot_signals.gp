# Stage A signal plot template; expects a signals.dat export in the working directory.
set terminal pngcairo
set output "results/plots/signals.png"
set title "Price and moving-average signals"
set xlabel "Trade"
set ylabel "Price"
plot "results/plots/signals.dat" using 1:2 with lines title "Price", \
     "results/plots/signals.dat" using 1:3 with lines title "Short SMA", \
     "results/plots/signals.dat" using 1:4 with lines title "Long SMA"
