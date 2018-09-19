set datafile separator ","
#set yrange [1:150000]
set logscale y 10
#set logscale x 2
set xlabel 'KBytes'
set terminal png size 1024,768 enhanced font 'Verdana,10'
set output 'testDataSetIO.png'

plot 'testDataSetIO.csv' using 2:3 lc rgb "green" with lines title 'push rate (cycle/s)','testDataSetIO.csv' using 2:4 lc rgb "cyan" with lines title 'pull rate (cycle/s)','testDataSetIO.csv' using 2:8 lc rgb "pink" with lines title 'bandwith (MB/s)','testDataSetIO.csv' using 2:9 lc rgb "magenta" with lines title  'prep overhead(us)'


