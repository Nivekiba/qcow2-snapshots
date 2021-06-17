# QCOW2 format

Sources used:
- https://github.com/qemu/qemu/blob/master/docs/interop/qcow2.txt
- https://people.igalia.com/berto/files/kvm-forum-2017-slides.pdf
- http://events17.linuxfoundation.org/sites/events/files/slides/How%20to%20Handle%20Globally%20Distributed%20QCOW2%20Chains_final_01.pdf
- https://www.linux-kvm.org/images/9/92/Qcow2-why-not.pdf


Virtual disk file format for QEMU. Very versatile as it supports on demand
growing, backing files, internal snapshots, compression and encryption. These
features come at the cost of a performance loss compared to the raw format --
but only in extreme cases when using features that anyway are not available for
raw.

QCOW2 disk images can be composed of several layers with the write oprations
being realized on the topmost (active) layer and read operations being served
by whatever layer has the valid version of the data. This can be used to create
snapshots and also share base images between several VMs. We have a chain of
layers and the first level below the active one is called the backing file.

The file is divided into units of the same size named clusters (defautl size:
64 KB). Used as allocation units at the host and guest level. First cluster is
the header and contain metadata.

## Header

First bytes of the image file, contains:
- General info such as qcow2 version (there are two versions of qcow2, 2 and
  3), encryption info, compression info, dirty/corrupted state info
- cluster size (seems 64 KB by default)
- Pointer to backing file, i.e. previous file in the snapshot chain. Also the
  "number of snapshots contained in the image", not clear what it is exactly
- optional bitmaps that can be used for various things, for now there is only
  one: the dirty bitmap tracking changes from some point in time

## Reference count for COW cluster management

There is a reference count (refcount) for each host cluster. A refcount of 0
means that the cluster is free, 1 means that it is used, and >=2 means that it
used and any write access must be down in a qcow2 fashion.

Refcounts are managed in a two-levels table. The first level table is called
refcount table. There is a pointer to it in the header. It must be contiguous
within the file. It has a variable size. It contains pointers to the
second-level structures, named  refcount blocks, each has a size of 1 cluster.

Into these table, the size of a L1 entry seems fixed to 64 bits. If it is 0 the
corresponding refcount block has not been allocated. Otherwise bits 9-63 are
the offset within the image file for the refount block. In the refcount block
table each entry contains the reference count for the cluster in question.

## Cluster mapping for data

There is a similar table with 2 levels named L1 table and L2 tables. L1 has a
variable size and can take multiple clusters but must be contiguous in the
image file. L2 tables are 1 cluster in size each.

An entry in the L1 table is 64 bits, the most important being (1)
an offset within the image file for the cluster containing the corresponding L2
table and (2) a status bit indicating if the corresponding L2 table is either
unused or require COW (bit is 0) or 1 if the refcount is exactly one. I assume
that by refcoun they mean the refcount of the L2 table itself -- not data.

An entry in the L2 table is also 64 bits. Most of these makes up the cluster
descriptor (described below) and there is also a status bit which is set to
0 for a cluster that is unused/compressed/requiring COW and it is 1 for
clusters that have a refcount of 1. The cluster descriptor is mostly composed
of the offset within the image file where the cluster data start and there is
also a bit used to indicate if the cluster should read all zeroes or not.

If a cluster is allocated the data is read and written from the current file.
If it is unallocated the data is read from the backing file (i.e. relaunch the
operation on the backing file). For a write operation on an unalocated cluster,
COW happens: the cluster is allocated in the current file and data is written
there. However, allocating a cluster for a write operation of size < to cluster
size requires bringing data to fill up the rest of the cluster. The concept of
"subcluster" was prototyped in 2017, not sure if it made it to the mainline.

## Snapshots

Qcow2 supports internal snapshots, i.e. several snapshots in the same file.
This seems to be a bit different from what we are doing i.e. external snapshots
that are made through a chain of backing files. I think we can safely focus on
external snapshots as they present many advantages, in fact red hat mentions
that internal shapshots are not actively developped and discourages their uses:
https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/7/html/virtualization_deployment_and_administration_guide/sect-troubleshooting-workaround_for_creating_external_snapshots_with_libvirt

The basic principle when creating a snapshot is: switch to a new L1 table which
is a copy of the old one and increase the refcount of the L2 tbales reachable
from this L1 table is increased.

There is a snapshot table in the image file, it contains one entry per
snapshot. This table is contiguous within the file. The entry contains various
info such as the offset where the corresponding L1 table starts, the snapshot
name, and also some info about the VM state -- it seems that the VM state can
(registers, etc.) can be saved alongside the disk data when making a snapshot.

There is on L1 table per snapshot and L2 tables are allocated on demand as the
image grows. The active L2 tables can be located in different layers. L1 table
is small and can be kept in RAM. L2 should too because a data lookup involves
going through both level however the total size of L2 can be large according
to the disk and cluster size: for example 128MB of RAM per TB of disk with the
default cluster size of 64 KB. So there is a cache for L2 tables.

## The L2 cache problem

L1 and L2 tables are on the file but we want to avoid 3 disk accesses to serve
one guest request. L1 table is small and can be cached in memory. However L2
is much bigger: for example with the default size of 64 KB clusters we need
128 MB of L2 tables per TB of disk. So L2 entries are cached in memory.

L2 entries are also cached in memory and this cache's size has a dramatic
impact on performance, for example going from 1 to 2.5 MB gives more than 10x
performance increase:
https://people.igalia.com/berto/files/kvm-forum-2017-slides.pdf 
L2 cache space
needed can be reduced by increasing the cluster size but it leads to slower
allocations and more wasted disk space. Default size is 1 MB, enough for 8 GB
disk. How to find the right default value is a bit complicated. Also, each
image file has its own cache! So with long snapshot chains there can be several
cache entries for the same cluster and thus memory wasted:
https://blogs.igalia.com/berto/2015/12/17/improving-disk-io-performance-in-qemu-2-5-with-the-qcow2-l2-cache/
The solution is to periodically drop entries for the cache that have not been
accessed -- there is an option for that.

This issue is also mentioned here:
https://www.linux-kvm.org/images/9/92/Qcow2-why-not.pdf

I made sure in my experiment to set enough L2 space in memory to cache the
entire set of entries for the 20 GB disk I use. I also reboot the VM between
each read test so I don't think there is cache thrashing due to old entries,
i.e. no need for dropping them.

## The COW problem

A new cluster is allocated when there is a write operation within a cluster
that is present on a backing file. If only a subset is written (and it is often
the case), the rest of the data must be read to fill the cluster (COW). This is
a costly operation:

1) before it consisted into up to two additional reads (before and after the
   written subset of the cluster) -> changed to a single read on the entire
   cluster. 
   This solution Was implemented long ago and thus the problem is probably not
   what makes the bad performance that I observe.

2) When doing sequential writes that spanned several clusters, but each request
   would be a subset of a cluster, lots of COW was done for nothing -> solved
   by implementing a cache with the newly written data and marking the cluster
   invalid. After that, if the newly written data must be read it is done from
   the cache and only if the old data needs to be read it triggers a COW
   operation. Another solution is to have subclusters, i.e. divide clusters
   into multiple pieces on which COW is done but it is not yet merged (last
   heard of 2017) because it requires changes of the on-disk format
   There was supposedly a patch for the cache on 2015 but I am not sure it is
   implemented in the version I have
   
   Could this be the issue? I don't think so, this issue should also be present
   with short snapshot chains anyway.

## The indexing with long chain problem

I am trying to understand how a read operation is done on a disk backed by a
long chain, assuming the data in question is far in the chain. Still looking in
the code but it takes time. THe only thing I can find outside of the code is
this:

http://events17.linuxfoundation.org/sites/events/files/slides/How%20to%20Handle%20Globally%20Distributed%20QCOW2%20Chains_final_01.pdf

They mention having to walk the chain to find L1 metadata in each file and put
it in RAM. Could it be a RAM issue for L1? 
