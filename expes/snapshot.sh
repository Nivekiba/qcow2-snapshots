#!/bin/bash

QEMU_GA_SOCKET=./qemu-ga-socket
QEMU_MONITOR_SOCKET=./qemu-monitor-socket
ITERATIONS=4
SLEEP_BETWEEN_ITERATIONS_SEC=1
# Stream every X iterations, 0 disables streaming
STREAM=0

sudo ls &> /dev/null

# Cleanup snapshot and other result files
rm -rf snapshot-* &> /dev/null
rm -rf results_bw.log &> /dev/null
rm -rf host.log &> /dev/null

# setup permissions on key
chmod 600 ./keys/id_rsa

# Launch fio on the guest
ssh -o "UserKnownHostsFile=/dev/null" -o StrictHostKeyChecking=no \
    -i ./keys/id_rsa root@localhost -p 10022 fio fio.job &
REMOTE_FIO_ID=$!

i=1
while true; do

let i="$i+1"
if [[ $i -gt $ITERATIONS ]]; then
    echo "Done, exiting"
    break
fi

# Take a snapshot
filename=snapshot-$i
./scripts/manual-snapshot.sh $filename

# Stream if needed
if [ ! $i = "0" ] && [ ! $STREAM = "0" ]; then
    let modulo="$i%$STREAM"
    if [ $modulo = "0" ]; then
        ./scripts/streaming.sh
    fi
fi

# Log number of open fds and snapshot size
qemu_pid=`ps -A | grep qemu | awk '{print $1}'`
fds=`sudo ls /proc/$qemu_pid/fd | wc -l`
ts=`date +%s`
size=`du -c snapshot-* | grep total | cut -f 1`
echo "$ts:$fds:$size" >> host.log

sleep $SLEEP_BETWEEN_ITERATIONS_SEC

done

# Kill fio on guest
ssh -o "UserKnownHostsFile=/dev/null" -o StrictHostKeyChecking=no \
    -i ./keys/id_rsa root@localhost -p 10022 killall fio
kill -9 $REMOTE_FIO_ID

# Grab fio bandwidth log
scp -o "UserKnownHostsFile=/dev/null" -o StrictHostKeyChecking=no \
    -i ./keys/id_rsa -P 10022 root@localhost:/root/results_*.log .

echo "---------------------------------------------"
echo "Snapshots: $ITERATIONS, Total snapshots size (KB):"
du snapshot-* | cut -f 1 | paste -sd+ | bc
echo "----------------------------------------------"
