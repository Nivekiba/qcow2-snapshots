#!/bin/bash

SNAPSHOTS=$1
QEMU_CMD_DIR=$2
SNAPSHOTS_DIR=$3
disk_size=$4
DISPERSION_DEB=$5
DISPERSION_MID=$6
DISPERSION_END=$7

if [ -z "$SNAPSHOTS" ]; then
	echo "\nCommand usage:\n\t ./gen_snapshots.sh NB_SNAPSHOTS PATH_TO_QEMU SNAPSHOTS_DIR disk_size [x y z]\n\tx% clusters presented at the chain begin, y% in the chain mid, z% in the end"
fi

SNAPSHOTS_DIR=`realpath $SNAPSHOTS_DIR`

# the 3 values for dispersion have to be set
if ! ([ ! -z "$DISPERSION_DEB" ] && [ ! -z "$DISPERSION_MID" ] && [ ! -z "$DISPERSION_END" ]); then
	DISPERSION_DEB=100
	DISPERSION_MID=0
	DISPERSION_END=0
fi

sudo rm -f disk/snapshot-*

sudo $QEMU_CMD_DIR/qemu-img create $SNAPSHOTS_DIR/snapshot-1 -b $SNAPSHOTS_DIR/ub-18.04_$disk_size.qcow2 -f qcow2 -F qcow2

let megas="(${disk_size:0:-1}-12)*1024*($DISPERSION_DEB)/100"

# Write x% in the 1st snapshot
sudo ./write_data.exp $QEMU_CMD_DIR $SNAPSHOTS_DIR/snapshot-1 $megas
sudo ./start_reboot_vm.exp $QEMU_CMD_DIR $SNAPSHOTS_DIR/snapshot-1

i=2
j=1

mid_ind=$SNAPSHOTS/2

while true; do
	if [[ $i -gt $SNAPSHOTS ]]; then
		echo "Snapshots created"
		break
	fi

    sudo $QEMU_CMD_DIR/qemu-img create $SNAPSHOTS_DIR/snapshot-$i -b $SNAPSHOTS_DIR/snapshot-$j -f qcow2 -F qcow2
	sudo ./start_reboot_vm.exp $QEMU_CMD_DIR $SNAPSHOTS_DIR/snapshot-$i
	
	let j="$i"
	let i="$i+1"

	# Write y% in the mid snapshot
	if [[ $i -eq  $mid_ind ]]; then
		let megas="(${disk_size:0:-1}-12)*1024*($DISPERSION_MID)/100"
		sudo ./write_data.exp $QEMU_CMD_DIR $SNAPSHOTS_DIR/snapshot-$i $megas
	fi
done

# Write z% in the last snapshot
let megas="(${disk_size:0:-1}-12)*1024*$DISPERSION_END/100"
sudo ./write_data.exp $QEMU_CMD_DIR $SNAPSHOTS_DIR/snapshot-$SNAPSHOTS $megas
sudo ./start_reboot_vm.exp $QEMU_CMD_DIR $SNAPSHOTS_DIR/snapshot-$SNAPSHOTS

echo "End creation of snapshots"