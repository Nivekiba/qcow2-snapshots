#!/bin/bash

chain_sizes="50 500"

echo "" >> dd_footprint

for chain in $chain_sizes; do
    sudo ./clear_ram.sh
    echo "vanilla $chain" >> dd_footprint
    cp /mnt/data/shareddir/chain-$chain-r-b/snapshot-$chain /mnt/data/shareddir/chain-$chain-r-b/snap
    cache_per=$((1792/$chain))
    sudo ./launch_rocksdb.sh ../qemu-4.2-vanilla/build /mnt/data/shareddir/chain-$chain-r-b/snap $cache_per"K" > dd
    cat dd | grep ", Average" >> dd_footprint
    cat dd | grep ", Throughput" >> dd_footprint
    rm /mnt/data/shareddir/chain-$chain-r-b/snap

    sudo ./clear_ram.sh



    echo "direct-access $chain" >> dd_footprint
    cp /mnt/data/shareddir/chain-$chain-r/snapshot-$chain /mnt/data/shareddir/chain-$chain-r/snap
    sudo ./launch_rocksdb.sh ../qemu-4.2/build /mnt/data/shareddir/chain-$chain-r/snap 1792K > dd
    cat dd | grep ", Average" >> dd_footprint
    cat dd | grep ", Throughput" >> dd_footprint
    rm /mnt/data/shareddir/chain-$chain-r/snap

    sudo ./clear_ram.sh
done
