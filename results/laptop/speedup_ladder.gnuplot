set terminal pngcairo size 1500,850 font 'Verdana,10'
set output 'C:/Users/siamu/Desktop/SIT Trimester 1/Mini Project/Grp05/results/laptop/speedup_ladder.png'
set title 'Stress-file speedup against frozen Version 0 (-O0)'
set xlabel 'Version / controlled rung'
set ylabel 'Median end-to-end speedup'
set style data boxes
set style fill solid 0.75
set grid ytics
set xtics rotate by -30
plot 'C:/Users/siamu/Desktop/SIT Trimester 1/Mini Project/Grp05/results/laptop/speedup_ladder.dat' using 1:2:xticlabels(3) notitle
