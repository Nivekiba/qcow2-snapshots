#!/bin/bash

SNAPSHOTS=$1
QEMU_CMD_DIR=$2
SNAPSHOTS_DIR=$3
BASE_DISK_PATH=$4
disk_size=$5
disk_format=$6


let megas="(${disk_size:0:-1}-1)*1024/$SNAPSHOTS"



if [ -z "$SNAPSHOTS" ]; then
	echo "\nCommand usage:\n\t ./gen_uniform_snapshots.sh NB_SNAPSHOTS PATH_TO_QEMU SNAPSHOTS_DIR BASE_DISK_PATH disk_size DISK_FORMAT\n"
	exit
fi

SNAPSHOTS_DIR=`realpath $SNAPSHOTS_DIR`
BASE_DISK_PATH=`realpath $BASE_DISK_PATH`

sudo rm -f $SNAPSHOTS_DIR/snapshot-*

sudo qemu-img create $SNAPSHOTS_DIR/snapshot-1 -f $disk_format $disk_size

sudo ./create_partition.exp ss $BASE_DISK_PATH -1 $SNAPSHOTS_DIR/snapshot-1 $disk_format

sudo ./write_data_ext.exp ss $BASE_DISK_PATH $megas $SNAPSHOTS_DIR/snapshot-1 $disk_format
sudo ./write_data_ext.exp ss $BASE_DISK_PATH $megas $SNAPSHOTS_DIR/snapshot-1 $disk_format

i=2
j=1

while true; do
	if [[ $i -gt $SNAPSHOTS ]]; then
		echo "Snapshots created"
		break
	fi

    sudo qemu-img create $SNAPSHOTS_DIR/snapshot-$i -b $SNAPSHOTS_DIR/snapshot-$j -f $disk_format	
	sudo ./write_data_ext.exp ss $BASE_DISK_PATH $megas $SNAPSHOTS_DIR/snapshot-$i $disk_format

	sudo ./clear_ram.sh
	sync
	
	let j="$i"
	let i="$i+1"
done

echo "End creation of snapshots"
