#!/bin/bash

ROOT=$1
QEMU_DIR=$2
QEMU=$ROOT/$QEMU_DIR/build/x86_64-softmmu/qemu-system-x86_64
file=$3

sudo $QEMU -smp 4 -m 4G \
    -drive format=qcow2,file=$3 \
    --accel kvm \
    -nographic \
    -nic user,hostfwd=tcp::10022-:22 \
    -chardev socket,path=qemu-ga-socket,server,nowait,id=qga0 \
    -device virtio-serial \
    -device virtserialport,chardev=qga0,name=org.qemu.guest_agent.0 \
    -monitor unix:qemu-monitor-socket,server,nowait \
    -qmp unix:qemu-qmp-socket,server,nowait
