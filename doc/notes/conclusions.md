## Taking a snapshot

- The action of taking a snapshot is relatively fast because it simply
  corresponds to creating a new layer upon which future writes will be directed.
  It execution time seems somhow affected by the snapshot chain length but
  this time is overall negligible.

- When taking a snapshot the guest performance drop. Currently I use the agent
  to have the guest stop doing I/O during snapshot and it is likely that the
  drop would be less important if I do not (outscale does not use the agent).
  However, given that snapsotting is such a fast process, the performance drop
  during that operation is not a big issue.

## Snapshot chain lenght impact on guest performance

- Past experiment was wrong, the base image was on ramfs but the snapshot on
  disk so the performance drop was mostly due to data being more and more on
  disk and less and less on ramfs! redoing the experiment with disk only

## Streaming

- Streaming time is long and guest performance drop significantly during it.

- What are the parameters that impact streaming time?
  - Snapshot chain length? or is it total snapshot size? -> CONFIRM THIS
  - Early experiment suggest some "base cost" -> CONFIRM THIS

## Sharing images

- What happens when a base image is shared and 1 VM streams?
