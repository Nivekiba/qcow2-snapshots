
QEMU_CMD_DIR=./qemu-4.2

rm snapshot-* -rf

sudo $QEMU_CMD_DIR/qemu-img create snapshot-1 -b disk/ubuntu-18.04.qcow2 -f qcow2 -F qcow2
sudo ./test.exp snapshot-1
sudo $QEMU_CMD_DIR/qemu-img create snapshot-2 -b snapshot-1 -f qcow2 -F qcow2
sudo $QEMU_CMD_DIR/qemu-img create snapshot-3 -b snapshot-2 -f qcow2 -F qcow2
sudo $QEMU_CMD_DIR/qemu-img create snapshot-4 -b snapshot-3 -f qcow2 -F qcow2
sudo $QEMU_CMD_DIR/qemu-img create snapshot-5 -b snapshot-4 -f qcow2 -F qcow2
sudo ./test.exp snapshot-5
sudo $QEMU_CMD_DIR/qemu-img create snapshot-6 -b snapshot-5 -f qcow2 -F qcow2
sudo $QEMU_CMD_DIR/qemu-img create snapshot-7 -b snapshot-6 -f qcow2 -F qcow2
sudo $QEMU_CMD_DIR/qemu-img create snapshot-8 -b snapshot-7 -f qcow2 -F qcow2
sudo $QEMU_CMD_DIR/qemu-img create snapshot-9 -b snapshot-8 -f qcow2 -F qcow2
sudo ./test.exp snapshot-9
sudo $QEMU_CMD_DIR/qemu-img create snapshot-10 -b snapshot-9 -f qcow2 -F qcow2
sudo $QEMU_CMD_DIR/qemu-img create snapshot-11 -b snapshot-10 -f qcow2 -F qcow2
sudo cp snapshot-11 back-snapshot-11
