#!/bin/bash

rocks_config="
redis.host=127.0.0.1
redis.port=6379
operationcount=3000000
measurementtype=timeseries
timeseries.granularity=2000
"

QEMU_CMD_DIR=$1
snapshot=$2
cache=$3

sudo ./run_redis.exp $QEMU_CMD_DIR $snapshot workloadc $cache "$rocks_config"
