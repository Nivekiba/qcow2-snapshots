#!/bin/bash

QEMU_DIR=$1
image_file=$2
cache=$3

QEMU=$QEMU_DIR/x86_64-softmmu/qemu-system-x86_64

sudo $QEMU --accel kvm -smp 4 -m 4G -drive file=$image_file,format=qcow2,cache=none,l2-cache-size=$cache -nographic -nic user,hostfwd=tcp::10022-:22 \
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
    sleep 30
done;

let spend="`date +%s` - bef"
echo "time spend to gain ssh service: "$spend

sshpass -p a ssh -o "UserKnownHostsFile=/dev/null" -o StrictHostKeyChecking=no \
    -i ./keys/id_rsa root@localhost -p 10022 'dd if=/dev/sda of=/dev/null bs=1M status=progress && rm -rf ~/.ssh/known_hosts' >& dd

cat dd | tail -1 | cut -d "," -f 4 >> dd_footprint

sudo ./get_wss.sh >> memory_footprint

sshpass -p a ssh -o "UserKnownHostsFile=/dev/null" -o StrictHostKeyChecking=no \
    -i ./keys/id_rsa root@localhost -p 10022 'shutdown now'

wait $QEMU_ID
#kill -9 $QEMU_ID || echo "nothing killed"

sleep 15
