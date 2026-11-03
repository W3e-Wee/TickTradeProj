set terminal pngcairo size 1200,700 font 'Verdana,10'
set output 'signals.png'
set title 'Tick-to-Trade signals'
set xlabel 'ordinal trade'
set ylabel 'price'
plot 'signals.dat' using 1:2 with lines title 'price', \
            'signals.dat' using 1:3 with lines title 'short SMA', \
            'signals.dat' using 1:4 with lines title 'long SMA', \
            'signals.dat' using 1:($5==1 ? $2 : 1/0) with points pt 7 ps 1.2 lc rgb 'green' title 'BUY', \
            'signals.dat' using 1:($5==-1 ? $2 : 1/0) with points pt 7 ps 1.2 lc rgb 'red' title 'SELL'
