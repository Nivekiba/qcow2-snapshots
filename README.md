Required packages:
```
sudo apt install qemu-system-x86 libguestfs-tools socat gnuplot \
    build-essential gnuplot libpixman-1-dev \
    libglib2.0-dev libpixman-1-dev libgtk-3-dev
```

Build qemu:
```
git submodule init
git submodule update
cd qemu
./configure --target-list=x86_64-softmmu --enable-debug-info
make -j`nproc`
```

Create guest image:
```
./create-vm.sh
```

Launch the VM:
```
./launch-vm.sh
```

Root password is `a`.

To launch the experiment, wait until the VM finishes its boot process then, on
the host:
```
./snapshot.sh
```

Note that you should relaunch the VM each time you start the experiment as qemu
pivot on a new live virtual disk file each time we take a snapshot and even if
we delete it, it will refuse to create a snapshot with the same name.

---
<!---
## Results

### Guest bandwith over time

The fio log is `results_bw.log". You can plot is as follows:
```
fio2gnuplot -b results -G raw
gnuplot mygraph
```

This creates PNG files plotting the bandwith over time.

### Qemu internals

The file `qemu-events.log` is a log containing the execution times of various
functions in the critical path of 2 operations:
- disk snapshotting
- streaming

Regarding the format, there is one line per function invocation:
```
timestamp (s):function name:execution time (s)
```

### Other host events
`host.log` is a log over time of the number of file decriptors opened by qemu,
as well as the totla size taken by the snapshot files (for now even after a
stream intermediate snapshots are not deleted)
the format is:
```
timestamp:number of fds:size of snapshots in bytes
```

## Misc info

Communicate with the guest agent:
```
sudo socat - UNIX-CONNECT:/tmp/qga.sock
```

Will need to send these commands before and after the snapshot:
```
guest-fsfreeze-freeze
guest-fsfreeze-thaw
```

Snapshot command itself, sent to the monitor through telnet:
```
echo snapshot_blkdev ide0-hd0 snapshot-filename qcow2 | telnet 127.0.0.1 55555
```

## System tap profiling

See `stap/Readme.md`

---

Questions for Outscale:
- are they using the guest agent to freeze and unfreeze the guest FS?
- need details to reproduce something more or less realistic
  - R/W traffic of a guest?
  - Frequency of snaphsots?
  - Frequency of streams? also what model, i.e. whcih snapshot do they merge
    and which they dont?
--->

Details information about Artifact evaluation are set in [expes directory](./expes)
