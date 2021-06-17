#!/bin/bash

ROOT=~/Desktop/qcow2-snapshots
QEMU_QMP_SOCKET=$ROOT/qemu-qmp-socket

echo "{'execute':'qmp_capabilities'} " \
    "{ 'execute': 'block-stream', 'arguments': { 'device': 'ide0-hd0'} }" \
    | sudo socat - UNIX-CONNECT:$QEMU_QMP_SOCKET


