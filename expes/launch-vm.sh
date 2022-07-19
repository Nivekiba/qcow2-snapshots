#!/bin/bash

size=$1

ROOT=/home/nivek/Workspace/qcow2-snapshots
QEMU=$ROOT/qemu-hack-direct-access/build/x86_64-softmmu/qemu-system-x86_64

sudo $QEMU -smp 4 -m 4G \
    -drive format=qcow2,file=disk/ub-18.04_20G.qcow2,cache=none \
    --accel kvm \
    -nographic \
    -nic user,hostfwd=tcp::10022-:22 \
    -chardev socket,path=qemu-ga-socket,server,nowait,id=qga0 \
    -device virtio-serial \
    -device virtserialport,chardev=qga0,name=org.qemu.guest_agent.0 \
    -monitor unix:qemu-monitor-socket,server,nowait \
    -qmp unix:qemu-qmp-socket,server,nowait
