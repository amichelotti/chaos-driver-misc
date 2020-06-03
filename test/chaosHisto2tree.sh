#!/bin/sh
## $0 chaost-cds01.chaos.lnf.infn.it:5000  "27-05-2020 19:00:00" "27-05-2020 21:00:00" "ALGO/WAVE/TEST/SINWAVE"
if [ $# -ne 4 ];then
    echo "$# you must specify server:port start end nodeid: es:$0 chaost-cds01.chaos.lnf.infn.it:5000  \"27-05-2020 19:00:00\" \"27-05-2020 21:00:00\" \"ALGO/WAVE/TEST/SINWAVE\"" 
    exit 1
fi 
source /usr/local/chaos/chaos-distrib/chaos_env.sh
chaosHisto2tree --notoutput 1 --log-max-size 200 --metadata-server "$1" --log-on-file 1 --start "$2" --end "$3" --page 1000 --log-file CHAOSHISTO.log --nodeid "$4" --direct-io-client-kv-param=ZMQ_RCVTIMEO:600000
