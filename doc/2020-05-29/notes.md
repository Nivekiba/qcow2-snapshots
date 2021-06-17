1) performance

The main issue is `bdrv_is_inserted` which execution time scales linerarly
(TODO confirm linear or factorial??) with the number of snapshots. It is called
in particular on the critical path of the IO, i.e for each request made by the
VM.

--> I confirmed that it is LINEAR not factorial.

I relaunched the experiment on a (much) slower storage medium --> external hdd
vs my ssd and although `brdv_is_inserted` took a smaller percentage of the
total profile, it was still dominant which is weird.

2) memory consumption

The big issue here is the l2 cache: there is one cache per snapshot! per
default it's about 2.5 MB per cache but administrators may set it higher to
support bigger disk sizes. Once again this again scales linearly with the
number of snapshots. 

1 snapshot, cache size 3MB, boot & full read && halt: 4200832 
300 snapshots, cache size 3MB, boot & full read && halt: 4828832 

The difference is very close to the cache sizes: we need exaclty 2.5 MB to
manage 20 GB of disk and 2.5 * 300 = 750.
