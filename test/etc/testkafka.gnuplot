set datafile separator ","
#set yrange [1:150000]
#set logscale y 2
set logscale x 2
set xlabel 'Bytes'
set xlabel 'MB/s'

set terminal png size 1024,768 enhanced font 'Verdana,10'
set output 'kafka_benchmark.png'

plot 'benchmark_write.csv' using 1:4 lc rgb "green" with lines title 'Write bandwith (KB/s)','benchmark_write.csv' using 1:5 lc rgb "orange" with lines title 'put (HZ)'
plot 'benchmark_read.csv' using 1:4 lc rgb "blue" with lines title 'Read bandwith (KB/s)','benchmark_read.csv' using 1:5 lc rgb "yellow" with lines title 'get (HZ)'


