# Artifact Evaluation Description

The main Goal of this files is to run our version of qemu ([sQemu](../qemu-4.2)) and the vanilla version ([vQemu](../qemu-4.2-vanilla))
under differents cases and compare results of the two.

So we have 3 cases of runs:
* **run to get metrics**: here we patch the 2 versions of qemu cited above to help us collect metrics such (cache lookup latency, number of cache hits, cache missed,...)
* **run default qemu**: here we generate chain of snapshots without considering usage in real life. This was for validating our solution effectiveness
* **run default qemu but on real application**: In this section, we re-run qemu but on real applications (rocksdb, redis) using YCSB to help simulate real life workloads.



## Metrics

### First create a root image
Our script create an ubuntu-based image, you can choose your virtual disk size
```
sudo ./create-vm.sh <size_of_disk>
```
_size_of_disk_ can be in octets (655643), in MB (567MB) or GB (40GB) but it's better to have a value higher than 20GB to contain the ubuntu OS.

### Then, generate the chain of snapshots for both qemu versions
Since for now we don't have a full written script to convert from the old format to our modified format, we generated each chain with each version

```
sudo ./gen_uniform_snapshot.sh <nb_snap> ../qemu-hack-direct-access/build <path_to_keep_snapshots_file> <size_of_disk>
```
_size_of_disk_ probably the same you use to create the root image
this script generates a chain of <nb_snap> snapshots and fills uniformly the virtual disk across all the snapshots so that all snapshots have approximatively the same size and the content of the virtual disk is uniformly distributed
data generated here are fully random files.

_qemu-hack-direct-access_ is a patch of ([sQemu](../qemu-4.2)) that helps us to create more fastly the chain. It didn't impact our new design.

```
sudo ./gen_uniform_snapshot.sh <nb_snap> ../qemu-4.2-vanilla/build <path_to_keep_snapshots_file> <path_of_dir_of_the_root_image> <size_of_disk>
# to create a chain for vQemu version
```

### Run the Metrics evaluation

```
sudo ./footprint_ssh.sh ../qemu-4.2-vanilla-metrics/build <path_to_keep_snapshots_file>/snapshot_<nb_snap> # vQemu
```
and
```
sudo ./footprint_ssh.sh ../qemu-4.2-metrics/build <path_to_keep_snapshots_file>/snapshot_<nb_snap> # sQemu
```

This scripts launch a VM on the latest snapshot file generated and run a single `dd` command on all the virtual disk the qemu-\*-metrics patchs collect data in file that will be processed in our jupyter notebook script.
Also, the `footprint_ssh.sh` gives on output 2 importants files:
* **dd_footprint**: which gives the Thoughput during the workload execution
* **memory_footprint**: which gives the memory consumption during all the workload execution

For the paper, we write a general script `memory_metrics.sh` which helps us gather metrics for both qemu versions for 4 differents size of snapshot's chain (0, 1, 50, 500, 100)


## sQemu effectiveness Validation

This section is similar to the metrics section
We first generates chains using the same scripts below.

### Evaluation on trivial workload (`dd`)

Then we run 
```
sudo ./footprint_ssh.sh ../qemu-4.2-vanilla/build <path_to_keep_snapshots_file_vqemu>/snapshot_<nb_snap> # vQemu
```
and
```
sudo ./footprint_ssh.sh ../qemu-4.2/build <path_to_keep_snapshots_file_sqemu>/snapshot_<nb_snap> # sQemu
```

without the metrics as you can see.
The general script we use to compute many differents executions here is `memory_grid.sh` since we did almost all of our experiments on grid5000, it is why the name is like this

### Evaluation of VM startup workload
Here we run just 

```
sudo ./startup_launch_dh.sh <nb_snap> <size_of_disk_in_G> start <path_to_keep_snapshots_file_sqemu> # for sQemu
```
and
```
sudo ./startup_launch_van.sh <nb_snap> <size_of_disk_in_G> start <path_to_keep_snapshots_file_vqemu> # for vQemu
```

### Evaluation of sQemu with variation of metadata cache
```
sudo ./cross_throughput_cache.sh <snapshot_path_vqemu> <snapshot_path_sqemu> <nb_snap>
```
The script `cross_throughput_cache.sh` execute the same `dd` workload on the same images but each time change the metadata allocation at VM startup

## Rocksdb/YCSB workload runs

Here we first want to generate a chain with data populated in the database and uniformly distributed on all the chain of snapshots

### Generate rocksdb chain for each qemu version
```
sudo ./gen_rocksdb_uniform_snapshot.sh <nb_snap> ../qemu-hack-direct-access/build <path_to_keep_snapshots_file_sqemu_rocks> <size_of_disk> # sQemu
```
and 
```
sudo ./gen_rocksdb_uniform_snapshot.sh <nb_snap> ../qemu-4.2-vanilla/build <path_to_keep_snapshots_file_vqemu_rocks> <size_of_disk> # vQemu
```

### Then launch experiments
For this section, i strongly recommended to copy the snapshot file and keep it at side for every execution.
While one execution on a snapshot file modify the snapshot content (due to VM startup), reexecute the same script on a modified file should not give the same results.

```
sudo ./launch_rocksdb.sh ../qemu-4.2/build <path_to_keep_snapshots_file_sqemu_rocks>/snapshot_<nb_snap> # sQemu
## and
sudo ./launch_rocksdb.sh ../qemu-4.2-vanilla/build <path_to_keep_snapshots_file_vqemu_rocks>/snapshot_<nb_snap> # vQemu
```
the natural output of `launch_rocksdb.sh` is the output of YCSB/rocksdb
The only problem you may have here is that you will have to set manually the output in the "jupyter notebook"

The generic script wrote for execution on grid5000 is: `mini_rocks.sh`


## The notebook file (Stats_and-graphs.ipynb)

Each section is almost well explain and they all use output files from differents scripts mentionned above except for `cross_throughput_cache.sh` and `launch_rocksdb.sh` scripts, i apologized for that
However, you will probably have some errors at first execution due to `paths` i enforced in scripts.
