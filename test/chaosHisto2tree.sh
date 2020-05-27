#!/bin/sh
source /usr/local/chaos/chaos-distrib/chaos_env.sh
chaosHisto2tree --log-max-size 200 --metadata-server $1 --log-on-file 1 --start "27-05-2020 19:00:00" --end "27-05-2020 21:00:00" --page 1000 --log-file CHAOSHISTO.log --nodeid "ALGO/WAVE/TEST/SINWAVE"