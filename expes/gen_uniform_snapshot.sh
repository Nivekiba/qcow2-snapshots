#!/bin/bash

SNAPSHOTS=$1
QEMU_CMD_DIR=$2
SNAPSHOTS_DIR=$3
<<<<<<< HEAD
DISK_DIR=$4
disk_size=$5
=======
disk_size=$4
>>>>>>> bc9377f627eb54b0bbfdd2ac584714804c1db0f9


let megas="(${disk_size:0:-1}-9)*1024/$SNAPSHOTS"



if [ -z "$SNAPSHOTS" ]; then
<<<<<<< HEAD
	echo "\nCommand usage:\n\t ./gen_uniform_snapshots.sh NB_SNAPSHOTS PATH_TO_QEMU SNAPSHOTS_DIR DISK_DIR disk_size\n"
=======
	echo "\nCommand usage:\n\t ./gen_uniform_snapshots.sh NB_SNAPSHOTS PATH_TO_QEMU SNAPSHOTS_DIR disk_size\n"
>>>>>>> bc9377f627eb54b0bbfdd2ac584714804c1db0f9
	exit
fi

SNAPSHOTS_DIR=`realpath $SNAPSHOTS_DIR`
<<<<<<< HEAD
DISK_DIR=`realpath $DISK_DIR`

sudo rm -f $SNAPSHOTS_DIR/snapshot-*

sudo $QEMU_CMD_DIR/qemu-img create $SNAPSHOTS_DIR/snapshot-1 -b $DISK_DIR/ub-18.04_$disk_size.qcow2 -f qcow2 -F qcow2
=======

sudo rm -f disk/snapshot-*

sudo $QEMU_CMD_DIR/qemu-img create $SNAPSHOTS_DIR/snapshot-1 -b $SNAPSHOTS_DIR/ub-18.04_$disk_size.qcow2 -f qcow2 -F qcow2
>>>>>>> bc9377f627eb54b0bbfdd2ac584714804c1db0f9
sudo ./write_data.exp $QEMU_CMD_DIR $SNAPSHOTS_DIR/snapshot-1 $megas

i=2
j=1

while true; do
	if [[ $i -gt $SNAPSHOTS ]]; then
		echo "Snapshots created"
		break
	fi

    sudo $QEMU_CMD_DIR/qemu-img create $SNAPSHOTS_DIR/snapshot-$i -b $SNAPSHOTS_DIR/snapshot-$j -f qcow2 -F qcow2	
    sudo ./write_data.exp $QEMU_CMD_DIR $SNAPSHOTS_DIR/snapshot-$i $megas

	sudo ./clear_ram.sh
	sync
	
	let j="$i"
	let i="$i+1"
done

echo "End creation of snapshots"