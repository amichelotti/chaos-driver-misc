#!/bin/sh
csvdir=$1
base=`dirname $0`

listacsv=`ls $csvdir/*.csv`

for i in $listacsv;do
    basen=`basename $i`
    diren=`dirname $i`
    rm -f __report_bp__.png
    ln -sf $i __report_bp__
    if gnuplot < $base/benchmark.gnuplot;then
	if mv __report_bp__.png $diren/$basen.png;then
	    echo "* created $diren/$basen.png"
	fi
	
    fi
done
