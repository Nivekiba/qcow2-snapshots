The function that causes the performance slowdown is: `bdrv_is_inserted`. It
can be called from various places (apart from itself):

- `bdrv_check_byte_request`: `block/io.c:872`
  - Checking the parameter of an IO request received by the block driver,
    amongst other things verify that the driver is inserted. Definitely
    important, looks like it's on the IO critical path

- `blk_is_inserted`: `block/block-backend.c:1776`
  - basically a wrapper, called from several places, Important?

- `bdrv_co_flush`: `block/io:2781`
  - Looks like the driver is caching written data and this is the function to
    flush it. Important?

- `brdv_can_snapshot`: `block/snapshot.c:150`
  - check if we can snapshot, maybe important? 

- `bdrv_all_snapshots_includes_bs`: `block/snapshot.c:388`
  - Check if snapshos include block driver state? important?

- `bdrv_co_pdiscard`: `block/io.c:2898`
  - important?

- `backup_job_create`: `block/backup.c:340`
  - For block replication, probably not important in our case

Command to test read perfs:
```
dd if=/dev/sda of=/dev/null bs=4M && \
    echo 3 > /proc/sys/vm/drop_caches && \
    time dd if=/dev/sda of=/dev/null bs=4M
```

GDB: disable break on SIGUSR1:
```
handle SIGUSR1 noprint
```

GDB command for conditional breakpoint (replace 1 with breakpoint number):
```
cond 1 !$_caller_is("bdrv_check_byte_request") && \
    !$_caller_is("blk_is_inserted") && \
    !$_caller_is("bdrv_is_inserted")
```

After tracing just boot + halt, this is what we got:
```
^Cbdrv_check_byte_request: 35637864
blk_is_inserted: 238948
bdrv_co_flush: 1274
bdrv_can_snapshot: 0
bdrv_all_snapshots_includes_bs: 0
bdrv_co_pdiscard: 0
backup_job_create: 0
```
