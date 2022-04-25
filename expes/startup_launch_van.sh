#!/bin/bash
#$1 : nb of snapshots in the string
#$2 : disk size (xG)
#$3 : the bench to execute (dd or start)
#$x : snapshots directory/path -- this for distributed nodes (nfs) -- could be configured using a macro
#

nb_snaps=$1
disk_size=$2

QEMU_DIR=$(pwd)
QEMU_CMD_DIR=$QEMU_DIR/qemu-4.2-vanilla/build
SNAPSHOTS_DIR=$QEMU_DIR/snapshots/vanilla/string-$nb_snaps

echo "****Snapshots : $nb_snaps - Disk : $disk_size****" >> time_vanilla
sudo $QEMU_DIR/expes/micro_bench_$3.exp $QEMU_CMD_DIR $SNAPSHOTS_DIR/snapshot-$nb_snaps time_vanilla

sleep 15
