#!/bin/bash
#$1 : nb of snapshots in the string
#$2 : disk size (xG)
#$3 : the bench to execute (dd or start)
#$x : snapshots directory/path -- this for distributed nodes (nfs) -- could be configured using a macro
#

# tcc = $x

#sudo ./create-vm.sh $disk_size #download iso/images of each size and keep them in order to avoid this at each bench

nb_snaps=$1
disk_size=$2

QEMU_DIR=$(pwd)
QEMU_CMD_DIR=$QEMU_DIR/qemu-hack-direct-access/build
SNAPSHOTS_DIR=$QEMU_DIR/snapshots/dir-hack/string-$nb_snaps
#   l2_cache_size = disk_size_GB * 131072
#	tcc : compute and place it as an input for micro_bench_dd.exp

echo "****Snapshots : $nb_snaps - Disk : $disk_size****" >> time_direct_hack
sudo $QEMU_DIR/expes/micro_bench_$3.exp $QEMU_CMD_DIR $SNAPSHOTS_DIR/snapshot-$nb_snaps time_direct_hack
#  | tail -3 >> logging
# echo "Throughput $SNAPSHOTS" >> looggin.g
# echo "Finshing calculing throughput"
# echo "" 
sleep 15