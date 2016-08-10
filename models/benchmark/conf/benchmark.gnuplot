#!/usr/bin/gnuplot
set datafile separator ","

set logscale y 10
set logscale x 2
set xlabel 'bytes'
set terminal pngcairo size 1024,768 enhanced font 'Verdana,10'
set output '__report_bp__.png'
plot '__report_bp__' using 1:2 with lines title 'UI Acquire (us)','__report_bp__' using 1:5 with lines title 'CU cycle (us)','__report_bp__' using 1:6:7 with yerrorbars title 'CU TRX (us)','__report_bp__' using 1:8 with lines title 'Bandwith (KB/s)','__report_bp__' using 1:9 title 'UI lost','__report_bp__' using 1:10 title 'ui-cu time shift (us)','__report_bp__' using 1:11 title 'command latency (us)','__report_bp__' using 1:12 with lines title 'Cycles/s'
set xlabel 'ui delay (us)'
set output '__report_rt__.png'
plot '__report_rt__' using 1:2:3 with yerrorbars title 'Round Trip (us)','__report_rt__' using 1:4:5 title 'Command Latency (us)','__report_rt__' using 1:6 title '#errors'

