#!/usr/bin/gnuplot
set datafile separator ","

set logscale y 10
set logscale x 2
set xlabel 'bytes'
set terminal pngcairo size 1024,768 enhanced font 'Verdana,10'
set output '__report_bp__.png'

plot '__report_bp__' using 1:2 lc rgb "green" with lines title 'UI Acquire (us)','__report_bp__' using 1:5 lc rgb "red" with lines title 'CU cycle (us)','__report_bp__' using 1:6:7 lc rgb "black" with yerrorbars title 'CU TRX (us)','__report_bp__' using 1:8 lc rgb "cyan" with lines title 'Bandwith (KB/s)','__report_bp__' using 1:9 lc rgb "pink" title 'UI lost','__report_bp__' using 1:10:11 lc rgb "gray" with yerrorbars title 'ui-cu time shift (us)','__report_bp__' using 1:12 lc rgb "magenta" title  'command latency (us)','__report_bp__' using 1:13 lc rgb "orange" with lines title 'Cycles/s'
set xlabel 'ui delay (us)'


