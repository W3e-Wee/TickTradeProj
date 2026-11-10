# Shared headless PNG plot style and locale settings.
if (GPVAL_VERSION < 5.0) {
    set print "|echo ERROR: gnuplot 5.0 or newer required >&2"
    print ""
    set print
    exit
}
set decimalsign '.'
set datafile separator whitespace
set datafile missing "NaN"
set style line 1 lc rgb "#0072B2" lw 2
set style line 2 lc rgb "#D55E00" lw 2
set style line 3 lc rgb "#009E73" lw 2
set grid lc rgb "#D0D0D0"
set key outside
set border lc rgb "#444444"
set tics textcolor rgb "#333333"
if (strstrt(GPVAL_TERMINALS, "pngcairo") > 0) {
    set terminal pngcairo size 1400,700 font ",10"
} else if (strstrt(GPVAL_TERMINALS, " png ") > 0 || strstrt(GPVAL_TERMINALS, " png") > 0) {
    set terminal png size 1400,700
} else {
    set print "|echo ERROR: no PNG terminal available >&2"
    print ""
    set print
    exit
}
