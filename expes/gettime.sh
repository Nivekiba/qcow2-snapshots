#!/bin/bash


disk_dir=/mnt/data/shareddir


# Vanilla one
cp $disk_dir/qcow2-snapshots/expes/disk/ub-18.04_50G.qcow2 $disk_dir/snap
sudo ./footprint_ssh.sh ../qemu-4.2-vanilla/build $disk_dir/snap 1M
rm -- $disk_dir/snap
mv time.csv base_van.csv

# direct access
cp $disk_dir/qcow2-snapshots/expes/disk/ub-18.04_50G.qcow2 $disk_dir/snap
sudo ./footprint_ssh.sh ../qemu-4.2/build $disk_dir/snap 1M
rm -- $disk_dir/snap
mv time.csv base_direct.csv

################################

# Vanilla one
cp $disk_dir/chain-500-b/snapshot-100 $disk_dir/snap
sudo ./footprint_ssh.sh ../qemu-4.2-vanilla/build $disk_dir/snap 1M
rm -- $disk_dir/snap
mv time.csv time_van_100.csv

# direct access
cp $disk_dir/chain-500/snapshot-100 $disk_dir/snap
sudo ./footprint_ssh.sh ../qemu-4.2/build $disk_dir/snap 1M
rm -- $disk_dir/snap
mv time.csv time_direct_100.csv