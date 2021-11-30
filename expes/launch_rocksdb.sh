#!/bin/bash

rocks_config="
rocksdb.dir=/root/ycsb-db
operationcount=500000
measurementtype=timeseries
timeseries.granularity=2000
"

QEMU_CMD_DIR=$1
snapshot=$2

sudo ./run_rocksdb.exp $QEMU_CMD_DIR $snapshot workloadc 1792K "$rocks_config" &

QEMU_ID=$!

rm /tmp/mem

while true; do
	ps aux | grep $QEMU_ID > /tmp/dd
	c=`wc -l /tmp/dd | cut -f 1 -d" "`
	[[ $c -eq 2 ]] || break
	sudo ./get_wss.sh >> /tmp/mem
done

tail -3 /tmp/mem >> memory_footprint
