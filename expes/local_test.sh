#!/bin/bash


#./clear_ram.sh
#cp disk/chain-100-r/snapshot-10 disk/chain-100-r/bsnap
#./throughput_ssh.sh ../qemu-hack-direct-access/build disk/chain-100-r/bsnap 1M

./clear_ram.sh
cp disk/chain-100-f/snapshot-26 disk/chain-100-f/bsnap
./throughput_ssh.sh ../qemu-4.2/build disk/chain-100-f/bsnap 1M
#./launch_rocksdb.sh ../qemu-4.2/build/ disk/chain-100-r/bsnap
#./rocksdb_footprint.sh ../qemu-4.2/build disk/chain-100-r/bsnap 1792K

#./clear_ram.sh
#cp disk/chain-100-r-b/snapshot-10 disk/chain-100-r-b/bsnap
#./throughput_ssh.sh ../qemu-4.2-vanilla/build disk/chain-100-r-b/bsnap 1M
