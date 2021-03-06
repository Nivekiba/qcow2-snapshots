#!/bin/bash

QEMU_DIR=$1
image_file=$2
cache=$3

QEMU=$QEMU_DIR/x86_64-softmmu/qemu-system-x86_64

sudo $QEMU -smp 4 --accel kvm -m 4G -drive file=$image_file,format=qcow2,cache=none,l2-cache-size=$cache,l2-cache-entry-size=512 -nographic -nic user,hostfwd=tcp::10022-:22 \
    -chardev socket,path=qemu-ga-socket,server,nowait,id=qga0 \
    -device virtio-serial \
    -device virtserialport,chardev=qga0,name=org.qemu.guest_agent.0 \
    -monitor unix:qemu-monitor-socket,server,nowait \
    -qmp unix:qemu-qmp-socket,server,nowait &

QEMU_ID=$!

sudo arp -d localhost
#sleep $3

bef=`date +%s`

while ! sshpass -p a ssh -i keys/id_rsa root@localhost -o StrictHostKeyChecking=no -p 10022 2> /dev/null true; do
    sleep 10
done;

let spend="`date +%s` - bef"
echo "time spend to gain ssh service: "$spend

sshpass -p a  ssh -o "UserKnownHostsFile=/dev/null" -o StrictHostKeyChecking=no \
    -i ./keys/id_rsa root@localhost -p 10022 'fio --filename=/dev/sda --direct=1 --percentage_random=70  --rw=randread --randrepeat=1 --ioengine=libaio --bs=4k --iodepth=32 --numjobs=1 --size=50G --name=randread.4k.out --runtime=180' >& dd

cat dd | tail -4 | head -1 >> dd_footprint

sshpass -p a ssh -o "UserKnownHostsFile=/dev/null" -o StrictHostKeyChecking=no \
    -i ./keys/id_rsa root@localhost -p 10022 'shutdown now'

#kill -9 `ps -A | grep qemu | awk '{print $1}'` || echo "nothing killed"
wait $QEMU_ID
#kill -9 $QEMU_ID || echo "nothing killed"

sleep 15
