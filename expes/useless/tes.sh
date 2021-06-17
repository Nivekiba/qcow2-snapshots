qemu='gdb --args ./qemu-4.2'


sudo $qemu/qemu-img create snapshot-a -b disk/ubuntu-18.04.qcow2 -f qcow2 -F qcow2
sudo $qemu/qemu-io snapshot-a -c "write 0x870000 1k"
sudo $qemu/qemu-io snapshot-a -c "write 0x870040 -z 64"
sudo $qemu/qemu-io snapshot-a -c "read -v 0x870000 1k"
rm snapshot-b snapshot-c snapshot-d -f
sudo $qemu/qemu-img create snapshot-b -b snapshot-a -f qcow2 -F qcow2
sudo $qemu/qemu-io snapshot-b -c "write 0x870200 -z 64"
sudo $qemu/qemu-io snapshot-b -c "read -v 0x870000 1k"
sudo $qemu/qemu-img create snapshot-c -b snapshot-b -f qcow2 -F qcow2
sudo $qemu/qemu-img create snapshot-d -b snapshot-c -f qcow2 -F qcow2
echo " "
#sudo $qemu/qemu-io snapshot-d -c "read -v 0x870000 1k"
