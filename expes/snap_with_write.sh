#!/bin/bash

QEMU_GA_SOCKET=./qemu-ga-socket
QEMU_MONITOR_SOCKET=./qemu-monitor-socket
ITERATIONS=$1
SLEEP_BETWEEN_ITERATIONS_SEC=1
data_size_megas=$2
snap_dir=$3

mkdir -p $snap_dir
rm $snap_dir/snapshot-* -rf

# setup permissions on key
chmod 600 ./keys/id_rsa

i=0
while true; do

let i="$i+1"
if [[ $i -gt $ITERATIONS ]]; then
    echo "Done, exiting"
    break
fi

# Take a snapshot
filename=$snap_dir/snapshot-$i
./scripts/manual-snapshot.sh $filename

ssh -o "UserKnownHostsFile=/dev/null" -o StrictHostKeyChecking=no \
    -i ./keys/id_rsa root@localhost -p 10022 "dd if=/dev/sda  of=file_`basename $filename` count=$data_size_megas  bs=1M  status=progress iflag=direct"

sleep $SLEEP_BETWEEN_ITERATIONS_SEC

done

ssh -o "UserKnownHostsFile=/dev/null" -o StrictHostKeyChecking=no \
    -i ./keys/id_rsa root@localhost -p 10022 "shutdown now"
