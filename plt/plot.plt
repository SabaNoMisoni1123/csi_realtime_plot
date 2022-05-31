set datafile separator ','
set terminal x11

set xrange [0:63]
set yrange [0:3000]
set style line 1 lw 3 lc 1
set nokey

set xlabel font"*,20"
set ylabel font"*,20"
set tics font"*,20"

set xlabel offset 0,0
set ylabel offset -2,0

set lmargin 12
# set bmargin 10

set xlabel "Subcarrier index"
set ylabel "CSI amplitude"

plot "./../build/out/csi_value.csv" using ($0):(sqrt($1**2 + $2**2)) ls 1 with lines
