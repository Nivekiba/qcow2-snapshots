#!/bin/bash

#
#    Objective: variation of l2 table cache, to show that our solution need less cache to have good performance
#

cache_sizes="1024K 2048K 3072K 4096K 5120K 6144K 7192K 112000K 224000K 448000K 896000K 1792000K 3584000K"
# 1M 2M 3M 4M 5M 6M 7M ~110M ~210M ~430M ...... ~3.5G (size required for max l2 table cache for 500 snapshots)
image_file_b=$1
image_file=$2
len_chain=$3

rm -- dd_footprint

#vanilla execution
for cache in $cache_sizes; do
    path=`realpath $image_file_b`
    dirpath=`dirname $path`

    echo "$cache vanilla" >> dd_footprint
    cp $path $dirpath/snap
    let cache_per_snap="${cache:0:-1}/$len_chain"
    sudo ./throughput_ssh.sh ../qemu-4.2-vanilla/build $dirpath/snap $cache
    rm $dirpath/snap

    sudo ./clear_ram.sh
done

#direct access
for cache in $cache_sizes; do
    path=`realpath $image_file`
    dirpath=`dirname $path`

    echo "$cache direct-access" >> dd_footprint
    cp $path $dirpath/snap
    sudo ./throughput_ssh.sh ../qemu-4.2/build $dirpath/snap $cache
    rm $dirpath/snap

    sudo ./clear_ram.sh
done