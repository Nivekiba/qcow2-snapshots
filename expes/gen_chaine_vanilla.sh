#!/bin/bash
#$1 : nb of snapshots in the string
#$2 : disk size (xG)

QEMU_DIR=$(pwd)
QEMU_CMD_DIR=$QEMU_DIR/qemu-4.2-vanilla/build
SNAPSHOTS_DIR=$QEMU_DIR/snapshots/vanilla/string-$nb_snaps #$3

nb_snaps=$1
disk_size=$2

sudo $QEMU_DIR/expes/clear_ram.sh
#rm $SNAPSHOTS_DIR/snapshot-* -f

sudo $QEMU_CMD_DIR/qemu-img create $SNAPSHOTS_DIR/snapshot-1 -b $QEMU_DIR/disk/ub-18.04_$disk_size.qcow2 -f qcow2 -F qcow2

i=2
j=1

while true; do
	if [[ $i -gt $nb_snaps ]]; then
		echo "Done, existing"
		break
	fi

    sudo $QEMU_CMD_DIR/qemu-img create $SNAPSHOTS_DIR/snapshot-$i -b $SNAPSHOTS_DIR/snapshot-$j -f qcow2 -F qcow2
	let j="$i"
	let i="$i+1"
done