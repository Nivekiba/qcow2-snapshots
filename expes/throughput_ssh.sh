#!/bin/bash

QEMU_DIR=$1
image_file=$2
QEMU=$QEMU_DIR/x86_64-softmmu/qemu-system-x86_64

sudo $QEMU --accel kvm -smp 4 -m 4G -drive file=$image_file,format=qcow2 -nic user,hostfwd=tcp::10022-:22 \
    -chardev socket,path=qemu-ga-socket,server,nowait,id=qga0 \
    -device virtio-serial \
    -device virtserialport,chardev=qga0,name=org.qemu.guest_agent.0 \
    -monitor unix:qemu-monitor-socket,server,nowait \
    -qmp unix:qemu-qmp-socket,server,nowait &

QEMU_ID=$!

sudo arp -d localhost
sleep 60

ssh -o "UserKnownHostsFile=/dev/null" -o StrictHostKeyChecking=no \
    -i ./keys/id_rsa root@localhost -p 10022 'dd if=/dev/sda of=/dev/null bs=1M status=progress' >& dd

cat dd | tail -1 | cut -d "," -f 4 >> dd_footprint

ssh -o "UserKnownHostsFile=/dev/null" -o StrictHostKeyChecking=no \
    -i ./keys/id_rsa root@localhost -p 10022 'shutdown now'

kill -9 `ps -A | grep qemu | awk '{print $1}'` || echo "nothing killed"
kill -9 $QEMU_ID || echo "nothing killed"

sleep 30