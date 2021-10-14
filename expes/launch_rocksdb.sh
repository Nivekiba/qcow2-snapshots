#!/bin/bash

rocks_config="
rocksdb.dir=/root/ycsb-db
operationcount=500000
measurementtype=timeseries
timeseries.granularity=2000
"

QEMU_CMD_DIR=$1
snapshot=$2

sudo ./run_rocksdb.exp $QEMU_CMD_DIR $snapshot workloadc 1792K "$rocks_config"