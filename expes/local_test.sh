#!/bin/bash


#./clear_ram.sh
#cp disk/chain-100-r/snapshot-10 disk/chain-100-r/bsnap
#./throughput_ssh.sh ../qemu-hack-direct-access/build disk/chain-100-r/bsnap 1M

./clear_ram.sh
cp disk/chain-100-r/snapshot-10 disk/chain-100-r/bsnap
./throughput_ssh.sh ../qemu-4.2/build disk/chain-100-r/bsnap 10M

#./clear_ram.sh
#cp disk/chain-100-r-b/snapshot-10 disk/chain-100-r-b/bsnap
#./throughput_ssh.sh ../qemu-4.2-vanilla/build disk/chain-100-r-b/bsnap 1M
