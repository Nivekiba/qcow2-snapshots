rm memory_footprint
rm dd_footprint

# sudo cp disk/chain-100-b/back-snapshot-300 disk/chain-100-b/snapshot-300
# sudo cp disk/chain-100-b/back-snapshot-200 disk/chain-100-b/snapshot-200
# sudo cp disk/chain-100-b/back-snapshot-100 disk/chain-100-b/snapshot-100
# sudo cp disk/chain-100-b/back-snapshot-1 disk/chain-100-b/snapshot-1

echo "\n\r300 vanilla\n\r" >> memory_footprint
sudo ./footprint_ssh.sh ../qemu-4.2-vanilla/build disk/chain-100-b/snapshot-300

echo "\n\r200 vanilla\n\r" >> memory_footprint
sudo ./footprint_ssh.sh ../qemu-4.2-vanilla/build disk/chain-100-b/snapshot-200

echo "\n\r100 vanilla\n\r" >> memory_footprint
sudo ./footprint_ssh.sh ../qemu-4.2-vanilla/build disk/chain-100-b/snapshot-100

echo "\n\r1 vanilla\n\r" >> memory_footprint
sudo ./footprint_ssh.sh ../qemu-4.2-vanilla/build disk/chain-100-b/snapshot-1

echo "\n\rbase vanilla\n\r" >> memory_footprint
sudo ./footprint_ssh.sh ../qemu-4.2-vanilla/build disk/ub-18.04_50G.qcow2

## Full memory common
echo "\n\n" >> memory_footprint
echo "\n\n" >> dd_footprint

sudo cp disk/chain-100/back-snapshot-200 disk/chain-100/snapshot-200
sudo cp disk/chain-100/back-snapshot-100 disk/chain-100/snapshot-100
sudo cp disk/chain-100/back-snapshot-1 disk/chain-100/snapshot-1

echo "\n\r300 hack_direct_common_cache\n\r" >> memory_footprint
sudo ./footprint_ssh.sh ../qemu-4.2/build disk/chain-100/snapshot-300
sleep 5

echo "\n\r200 hack_direct_common_cache\n\r" >> memory_footprint
sudo ./footprint_ssh.sh ../qemu-4.2/build disk/chain-100/snapshot-200
sleep 5

echo "\n\r100 hack_direct_common_cache\n\r" >> memory_footprint
sudo ./footprint_ssh.sh ../qemu-4.2/build disk/chain-100/snapshot-100
sleep 5

echo "\n\r1 hack_direct_common_cache\n\r" >> memory_footprint
sudo ./footprint_ssh.sh ../qemu-4.2/build disk/chain-100/snapshot-1
sleep 5

echo "\n\rbase hack_direct_common_cache\n\r" >> memory_footprint
sudo ./footprint_ssh.sh ../qemu-4.2/build disk/ub-18.04_50G.qcow2
sleep 5


## only direct access
echo "\n\n" >> memory_footprint
echo "\n\n" >> dd_footprint

sudo cp disk/chain-100/back-snapshot-200 disk/chain-100/snapshot-200
sudo cp disk/chain-100/back-snapshot-100 disk/chain-100/snapshot-100
sudo cp disk/chain-100/back-snapshot-1 disk/chain-100/snapshot-1

echo "\n\r300 hack_direct\n\r" >> memory_footprint
sudo ./footprint_ssh.sh ../qemu-hack-direct-access/build disk/chain-100/snapshot-300
sleep 5

echo "\n\r200 hack_direct\n\r" >> memory_footprint
sudo ./footprint_ssh.sh ../qemu-hack-direct-access/build disk/chain-100/snapshot-200
sleep 5

echo "\n\r100 hack_direct\n\r" >> memory_footprint
sudo ./footprint_ssh.sh ../qemu-hack-direct-access/build disk/chain-100/snapshot-100
sleep 5

echo "\n\r1 hack_direct\n\r" >> memory_footprint
sudo ./footprint_ssh.sh ../qemu-hack-direct-access/build disk/chain-100/snapshot-1
sleep 5

echo "\n\rbase hack_direct\n\r" >> memory_footprint
sudo ./footprint_ssh.sh ../qemu-hack-direct-access/build disk/ub-18.04_50G.qcow2
sleep 5