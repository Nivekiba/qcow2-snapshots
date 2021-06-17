#!/bin/bash
#$1 : nb of snapshots in the string
#$2 : disk size (xG)
#$3 : the bench to execute (dd or start)
#$x : snapshots directory/path -- this for distributed nodes (nfs) -- could be configured using a macro
#

# tcc = $x

#sudo ./create-vm.sh $disk_size #download iso/images of each size and keep them in order to avoid this at each bench

QEMU_DIR=$(pwd)
QEMU_CMD_DIR=$QEMU_DIR/qemu-hack-direct-access/build
SNAPSHOTS_DIR=$QEMU_DIR/snapshots/dir-hack/string-$nb_snaps #$3

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