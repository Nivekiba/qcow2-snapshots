rm memory_footprint
rm dd_footprint

disk_dir=/mnt/data/shareddir

# sudo cp disk/chain-100-b/back-snapshot-300 disk/chain-100-b/snapshot-300
# sudo cp disk/chain-100-b/back-snapshot-200 disk/chain-100-b/snapshot-200
# sudo cp disk/chain-100-b/back-snapshot-100 disk/chain-100-b/snapshot-100
# sudo cp disk/chain-100-b/back-snapshot-1 disk/chain-100-b/snapshot-1
rm stats_events_vanilla.csv

echo "\n\rbase vanilla\n\r" >> memory_footprint
sudo ./footprint_ssh.sh ../qemu-4.2-vanilla-metrics/build $disk_dir/ub-18.04_50G.qcow2
mv stats_events_vanilla.csv vanilla0.csv

echo "\n\r1 vanilla\n\r" >> memory_footprint
sudo ./footprint_ssh.sh ../qemu-4.2-vanilla-metrics/build $disk_dir/chain-1-b/snapshot-1
mv stats_events_vanilla.csv vanilla1.csv

echo "\n\r50 vanilla\n\r" >> memory_footprint
sudo ./footprint_ssh.sh ../qemu-4.2-vanilla-metrics/build $disk_dir/chain-50-b/snapshot-50
mv stats_events_vanilla.csv vanilla50.csv

echo "\n\r500 vanilla\n\r" >> memory_footprint
sudo ./footprint_ssh.sh ../qemu-4.2-vanilla-metrics/build $disk_dir/chain-500-b/snapshot-500
mv stats_events_vanilla.csv vanilla500.csv

echo "\n\r1000 vanilla\n\r" >> memory_footprint
sudo ./footprint_ssh.sh ../qemu-4.2-vanilla-metrics/build $disk_dir/chain-1000-b/snapshot-970
mv stats_events_vanilla.csv vanilla1000.csv

## Full memory common
echo "\n\n" >> memory_footprint
echo "\n\n" >> dd_footprint

echo "\n\rbase hack_direct_common_cache\n\r" >> memory_footprint
sudo ./footprint_ssh.sh ../qemu-4.2-metrics/build $disk_dir/ub-18.04_50G.qcow2
sleep 5
mv stats_events.csv direct0.csv

echo "\n\r1 hack_direct_common_cache\n\r" >> memory_footprint
sudo ./footprint_ssh.sh ../qemu-4.2-metrics/build $disk_dir/chain-1/snapshot-1
sleep 5
mv stats_events.csv direct1.csv

echo "\n\r50 hack_direct_common_cache\n\r" >> memory_footprint
sudo ./footprint_ssh.sh ../qemu-4.2-metrics/build $disk_dir/chain-50/snapshot-50
sleep 5
mv stats_events.csv direct50.csv

echo "\n\r500 hack_direct_common_cache\n\r" >> memory_footprint
sudo ./footprint_ssh.sh ../qemu-4.2-metrics/build $disk_dir/chain-500/snapshot-500
sleep 5
mv stats_events.csv direct500.csv

echo "\n\r1000 hack_direct_common_cache\n\r" >> memory_footprint
sudo ./footprint_ssh.sh ../qemu-4.2-metrics/build $disk_dir/chain-1000/snapshot-1000
sleep 5
mv stats_events.csv direct1000.csv
