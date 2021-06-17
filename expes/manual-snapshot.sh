#!/bin/bash

ROOT=/home/nivek/Workspace/qcow2-snapshots/expes
QEMU_GA_SOCKET=$ROOT/qemu-ga-socket
QEMU_MONITOR_SOCKET=$ROOT/qemu-monitor-socket

if [[ "$1" == "" ]]; then
    echo "usage: $0 <output>"
    exit
fi

filename=$1
filepath=`realpath $filename`

echo '{"execute": "guest-fsfreeze-freeze"}' | sudo socat - UNIX-CLIENT:$QEMU_GA_SOCKET
# TODO loop over status until it becomes frozen
echo '{"execute": "guest-fsfreeze-status"}' | sudo socat - UNIX-CLIENT:$QEMU_GA_SOCKET
sudo socat -T.1 - UNIX-CONNECT:$QEMU_GA_SOCKET
sleep .2
echo "snapshot_blkdev ide0-hd0 `realpath $filepath` qcow2" | sudo socat - UNIX-CONNECT:$QEMU_MONITOR_SOCKET
sleep .2
echo '{"execute": "guest-fsfreeze-thaw"}' | sudo socat - UNIX-CONNECT:$QEMU_GA_SOCKET
# TODO loop over status until it becomes thawed
echo '{"execute": "guest-fsfreeze-status"}' | sudo socat - UNIX-CLIENT:$QEMU_GA_SOCKET
sudo socat -T.1 - UNIX-CONNECT:$QEMU_GA_SOCKET
sleep .2
