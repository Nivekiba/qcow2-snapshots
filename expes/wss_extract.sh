#!/bin/bash

QEMU_CMD_DIR=$1

<<<<<<< HEAD
snapshots_lengths=(1 50 100)
=======
snapshots_lengths=(1 50 100 150 200)
>>>>>>> bc9377f627eb54b0bbfdd2ac584714804c1db0f9


y=`cat checkpoint_memory_expe`
 

<<<<<<< HEAD
[[ $y -gt 2 ]] && exit 0
echo ${snapshots_lengths[$y]}

expect -c "
set timeout -1
spawn $QEMU_CMD_DIR/x86_64-softmmu/qemu-system-x86_64 -smp 4 -serial stdio --accel kvm -m 4G -drive file=disk/chain-100/snapshot-${snapshots_lengths[$y]},format=qcow2,cache=none,l2-cache-size=7M
=======
[[ $y -gt 4 ]] && exit 0
echo ${snapshots_lengths[$y]}


echo "$QEMU_CMD_DIR/x86_64-softmmu/qemu-system-x86_64 -smp 4 -serial stdio --accel kvm -m 4G -drive file=disk/snapshot-${snapshots_lengths[$y]},format=qcow2"

expect -c "
set timeout -1
spawn $QEMU_CMD_DIR/x86_64-softmmu/qemu-system-x86_64 -smp 4 -serial stdio --accel kvm -m 4G -drive file=disk/snapshot-${snapshots_lengths[$y]},format=qcow2
>>>>>>> bc9377f627eb54b0bbfdd2ac584714804c1db0f9

expect \"login: \"
send \"root\n\"

expect \"Password: \"
send \"a\n\"

expect \"# \"
send \"dd if=/dev/sda of=/dev/null bs=1M iflag=direct\r\n\"


expect \"# \"
send \"shutdown -h now\n\"
" &

pid=$!

sleep 4

sudo ./wss.pl `ps -A | grep qemu | awk '{print $1}'` 1 -C > es

wait $pid

sudo echo ${snapshots_lengths[$y]} >> memory_size
tail -1 es >> memory_size

let g="$y+1"
echo $g > checkpoint_memory_expe

./wss_extract.sh $QEMU_CMD_DIR