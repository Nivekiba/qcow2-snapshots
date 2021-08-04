#!/bin/bash

QEMU_DIR=$1
ITERATIONS=$2

QEMU=$QEMU_DIR/x86_64-softmmu/qemu-system-x86_64

sudo $QEMU --accel kvm -smp 4 -m 4G -drive file=`realpath snapshot-tests/disk/ub-18.04_50G.qcow2`,format=qcow2,cache=none,l2-cache-size=7M -nographic -nic user,hostfwd=tcp::10022-:22 \
    -chardev socket,path=qemu-ga-socket,server,nowait,id=qga0 \
    -device virtio-serial \
    -device virtserialport,chardev=qga0,name=org.qemu.guest_agent.0 \
    -monitor unix:qemu-monitor-socket,server,nowait \
    -qmp unix:qemu-qmp-socket,server,nowait &

QEMU_ID=$!

sudo arp -d localhost
#sleep $3

bef=`date +%s`

while ! ssh -i keys/id_rsa root@localhost -o StrictHostKeyChecking=no -p 10022 2> /dev/null true; do
    sleep 30
done;

let spend="`date +%s` - bef"
echo "time spend to gain ssh service: "$spend

ssh -o "UserKnownHostsFile=/dev/null" -o StrictHostKeyChecking=no \
    -i ./keys/id_rsa root@localhost -p 10022 'dpkg --configure -a && apt --fix-broken install -y && apt-get update -y && apt-get install maven git -y'

ssh -o "UserKnownHostsFile=/dev/null" -o StrictHostKeyChecking=no \
    -i ./keys/id_rsa root@localhost -p 10022 "rm -rf -- ~/YCSB && git clone https://github.com/brianfrankcooper/YCSB.git && cd ~/YCSB && mvn -pl site.ycsb:rocksdb-binding -am clean package"

echo "
recordcount=1200000
rocksdb.dir=/root/ycsb-db
fieldcount=150
fieldlength=150
target=1000
operationcount=100000
measurementtype=timeseries
timeseries.granularity=2000
" > rocksdb.dat

scp -o "UserKnownHostsFile=/dev/null" -o StrictHostKeyChecking=no \
    -i ./keys/id_rsa -P 10022 rocksdb.dat root@localhost:~


ssh -o "UserKnownHostsFile=/dev/null" -o StrictHostKeyChecking=no \
    -i ./keys/id_rsa root@localhost -p 10022 \
    "rm -rf -- ~/ycsb-db || echo Deletion DB"


ssh -o "UserKnownHostsFile=/dev/null" -o StrictHostKeyChecking=no \
    -i ./keys/id_rsa root@localhost -p 10022 \
    "cd ~/YCSB && ./bin/ycsb load rocksdb -s -P workloads/workloada -P ~/rocksdb.dat && fsync" &

REMOTE_YCSB_PID=$!

# Create snapshot while writing
rm -rf -- ./snapshot-tests/snapshot-*
mkdir -p ./snapshot-tests

i=0
SLEEP_BETWEEN_ITERATIONS_SEC=$(echo "1000000.0 / (1000*$ITERATIONS)" | bc -l)

while true; do
    let i="$i+1"
    if [[ $i -gt $ITERATIONS ]]; then
        echo "Done, exiting"
        break
    fi

    sleep $SLEEP_BETWEEN_ITERATIONS_SEC

    filename=./snapshot-tests/snapshot-$i
    filename=`realpath $filename`
    sudo ./manual-snapshot.sh $filename
done

wait $REMOTE_YCSB_PID
# End of script

ssh -o "UserKnownHostsFile=/dev/null" -o StrictHostKeyChecking=no \
    -i ./keys/id_rsa root@localhost -p 10022 'shutdown now'

rm rocksdb.dat
wait $QEMU_ID

sleep 15
