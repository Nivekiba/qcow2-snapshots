#!/bin/bash

QEMU_GA_SOCKET=./qemu-ga-socket
QEMU_MONITOR_SOCKET=./qemu-monitor-socket
ITERATIONS=35

# Cleanup snapshot and other result files
rm -rf snapshot-* &> /dev/null

# setup permissions on key
chmod 600 ./keys/id_rsa

# Launch command on host

SNAPSHOTS=$1
QEMU_CMD_DIR=$2
SNAPSHOTS_DIR=$3
disk_size=$4


let megas="(${disk_size:0:-1}-9)*1024/$SNAPSHOTS"



if [ -z "$SNAPSHOTS" ]; then
	echo "\nCommand usage:\n\t ./gen_uniform_snapshots_ssh.sh NB_SNAPSHOTS PATH_TO_QEMU SNAPSHOTS_DIR disk_size\n"
	exit
fi

SNAPSHOTS_DIR=`realpath $SNAPSHOTS_DIR`

sudo rm -f disk/snapshot-*
ssh -o "UserKnownHostsFile=/dev/null" -o StrictHostKeyChecking=no \
    -i ./keys/id_rsa root@localhost -p 10022 "rm -rf file*"



#sudo $QEMU_CMD_DIR/qemu-img create $SNAPSHOTS_DIR/snapshot-1 -b $SNAPSHOTS_DIR/ub-18.04_$disk_size.qcow2 -f qcow2 -F qcow2
./manual-snapshot.sh $SNAPSHOTS_DIR/snapshot-1
ssh -o "UserKnownHostsFile=/dev/null" -o StrictHostKeyChecking=no \
    -i ./keys/id_rsa root@localhost -p 10022 "dd if=/dev/zero  of=file_snapshot-1 count=$megas bs=1M"

i=2
j=1
sleep 15

while true; do
	if [[ $i -gt $SNAPSHOTS ]]; then
		echo "Snapshots created"
		break
	fi

    #sudo $QEMU_CMD_DIR/qemu-img create $SNAPSHOTS_DIR/snapshot-$i -b $SNAPSHOTS_DIR/snapshot-$j -f qcow2 -F qcow2	
    ./manual-snapshot.sh $SNAPSHOTS_DIR/snapshot-$i
    ssh -o "UserKnownHostsFile=/dev/null" -o StrictHostKeyChecking=no \
        -i ./keys/id_rsa root@localhost -p 10022 "dd if=/dev/zero  of=file_snapshot-$i count=$megas  bs=1M" 
sleep 15
	let j="$i"
	let i="$i+1"
done

echo "End creation of snapshots"