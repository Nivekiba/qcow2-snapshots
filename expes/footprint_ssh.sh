QEMU_DIR=$1
image_file=$2
QEMU=$QEMU_DIR/x86_64-softmmu/qemu-system-x86_64

sudo $QEMU --accel kvm -smp 4 -m 4G -drive file=$image_file,format=qcow2 -nic user,hostfwd=tcp::10022-:22 \
    -chardev socket,path=qemu-ga-socket,server,nowait,id=qga0 \
    -device virtio-serial \
    -device virtserialport,chardev=qga0,name=org.qemu.guest_agent.0 \
    -monitor unix:qemu-monitor-socket,server,nowait \
    -qmp unix:qemu-qmp-socket,server,nowait &

sleep 120

sshpass -p 'a' ssh -o StrictHostKeyChecking=no root@localhost -p 10022 'dd if=/dev/sda of=/dev/null bs=1M status=progress'

sudo ./get_wss.sh >> memory_footprint

sshpass -p 'a' ssh -o StrictHostKeyChecking=no root@localhost -p 10022 'shutdown now'

sleep 10