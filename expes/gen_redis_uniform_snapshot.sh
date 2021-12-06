#!/bin/bash

SNAPSHOTS=$1
QEMU_CMD_DIR=$2
SNAPSHOTS_DIR=$3
DISK_DIR=$4
disk_size=$5

if [ -z "$SNAPSHOTS" ]; then
	echo "\nCommand usage:\n\t ./gen_uniform_snapshots.sh NB_SNAPSHOTS PATH_TO_QEMU SNAPSHOTS_DIR DISK_DIR disk_size\n"
	exit
fi

SNAPSHOTS_DIR=`realpath $SNAPSHOTS_DIR`
DISK_DIR=`realpath $DISK_DIR`

sudo rm -f $SNAPSHOTS_DIR/snapshot-*

sudo $QEMU_CMD_DIR/qemu-img create $SNAPSHOTS_DIR/snapshot-1 -b $DISK_DIR/ub-18.04_$disk_size.qcow2 -f qcow2 -F qcow2
sudo ./setup_redis.exp $QEMU_CMD_DIR $SNAPSHOTS_DIR/snapshot-1
# sudo ./setup_redis.exp $QEMU_CMD_DIR $SNAPSHOTS_DIR/snapshot-1

i=2
j=1

recordcount=3000000
target=500
index_current_snap=0
count_per_snap=$((recordcount/SNAPSHOTS))

while true; do
	if [[ $i -gt $SNAPSHOTS ]]; then
		echo "Snapshots created"
		break
	fi

	if [[ $count_per_snap -ge 1000000 ]]; then
		count_per_snap=1000000
	fi

    sudo $QEMU_CMD_DIR/qemu-img create $SNAPSHOTS_DIR/snapshot-$i -b $SNAPSHOTS_DIR/snapshot-$j -f qcow2 -F qcow2	

	rocks_config="
redis.host=127.0.0.1
redis.port=6379
recordcount=$recordcount
threadcount=15
fieldcount=50
fieldlength=50
insertstart=$index_current_snap
insertcount=$count_per_snap
measurementtype=timeseries
timeseries.granularity=2000
    "
	
	sudo ./write_redis.exp $QEMU_CMD_DIR $SNAPSHOTS_DIR/snapshot-$i "$rocks_config"
	
    let index_current_snap="$index_current_snap+$count_per_snap"

	sudo ./clear_ram.sh
	sync
	
	let j="$i"
	let i="$i+1"
done

echo "End creation of snapshots"