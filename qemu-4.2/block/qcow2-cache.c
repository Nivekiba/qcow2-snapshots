/*
 * L2/refcount table cache for the QCOW2 format
 *
 * Copyright (c) 2010 Kevin Wolf <kwolf@redhat.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "qemu/osdep.h"
#include "qcow2.h"
#include "trace.h"

typedef struct Qcow2CachedTable {
    int64_t  offset;
    unsigned int l1_index;
    unsigned int start_slice;
    uint64_t lru_counter;
    int      ref;
    bool     dirty;
    BlockDriverState       *last_bs_req; // last bs to do a request in this table
} Qcow2CachedTable;

struct Qcow2Cache {
    Qcow2CachedTable       *entries;
    struct Qcow2Cache      *depends;
    int                     size;
    int                     table_size;
    bool                    depends_on_flush;
    void                   *table_array;
    uint64_t                lru_counter;
    uint64_t                cache_clean_lru_counter;
};

int nb_ext_maxi = 0;

static inline void *qcow2_cache_get_table_addr(Qcow2Cache *c, int table)
{
    return (uint8_t *) c->table_array + (size_t) table * c->table_size;
}

static inline int qcow2_cache_get_table_idx(Qcow2Cache *c, void *table)
{
    ptrdiff_t table_offset = (uint8_t *) table - (uint8_t *) c->table_array;
    int idx = table_offset / c->table_size;
    assert(idx >= 0 && idx < c->size && table_offset % c->table_size == 0);
    return idx;
}

static inline const char *qcow2_cache_get_name(BDRVQcow2State *s, Qcow2Cache *c)
{
    if (c == s->refcount_block_cache) {
        return "refcount block";
    } else if (c == s->l2_table_cache) {
        return "L2 table";
    } else {
        /* Do not abort, because this is not critical */
        return "unknown";
    }
}

static void qcow2_cache_table_release(Qcow2Cache *c, int i, int num_tables)
{
    
    if(c->entries[i].last_bs_req != NULL){
        BDRVQcow2State *ss = c->entries[i].last_bs_req->opaque;
        if(ss->l2_table_cache == c){
            qcow2_cache_table_release(write_cache, i, num_tables);
        }
    }
/* Using MADV_DONTNEED to discard memory is a Linux-specific feature */
#ifdef CONFIG_LINUX
    void *t = qcow2_cache_get_table_addr(c, i);
    int align = qemu_real_host_page_size;
    size_t mem_size = (size_t) c->table_size * num_tables;
    size_t offset = QEMU_ALIGN_UP((uintptr_t) t, align) - (uintptr_t) t;
    size_t length = QEMU_ALIGN_DOWN(mem_size - offset, align);
    if (mem_size > offset && length > 0) {
        madvise((uint8_t *) t + offset, length, MADV_DONTNEED);
    }
#endif
}

static inline bool can_clean_entry(Qcow2Cache *c, int i)
{
    Qcow2CachedTable *t = &c->entries[i];
    return t->ref == 0 && !t->dirty && t->offset != 0 &&
        t->lru_counter <= c->cache_clean_lru_counter;
}

void qcow2_cache_clean_unused(Qcow2Cache *c)
{
    int i = 0;
    while (i < c->size) {
        int to_clean = 0;

        /* Skip the entries that we don't need to clean */
        while (i < c->size && !can_clean_entry(c, i)) {
            i++;
        }

        /* And count how many we can clean in a row */
        while (i < c->size && can_clean_entry(c, i)) {
            c->entries[i].offset = 0;
            c->entries[i].lru_counter = 0;
            c->entries[i].last_bs_req = NULL;
            i++;
            to_clean++;
        }

        if (to_clean > 0) {
            qcow2_cache_table_release(c, i - to_clean, to_clean);
        }
    }

    c->cache_clean_lru_counter = c->lru_counter;
}

Qcow2Cache *qcow2_cache_create(BlockDriverState *bs, int num_tables,
                               unsigned table_size)
{
    BDRVQcow2State *s = bs->opaque;
    Qcow2Cache *c;

    assert(num_tables > 0);
    assert(is_power_of_2(table_size));
    //assert(table_size >= (1 << MIN_CLUSTER_BITS));
    assert(table_size <= s->cluster_size);

    c = g_new0(Qcow2Cache, 1);
    c->size = num_tables;
    c->table_size = table_size;
    c->entries = g_try_new0(Qcow2CachedTable, num_tables);
    c->table_array = qemu_try_blockalign(bs->file->bs,
                                         (size_t) num_tables * c->table_size);

    if (!c->entries || !c->table_array) {
        qemu_vfree(c->table_array);
        g_free(c->entries);
        g_free(c);
        c = NULL;
    }

    return c;
}

int qcow2_cache_destroy(Qcow2Cache *c)
{
    int i;

    for (i = 0; i < c->size; i++) {
        assert(c->entries[i].ref == 0);
    }

    qemu_vfree(c->table_array);
    g_free(c->entries);
    g_free(c);

    return 0;
}

static int qcow2_cache_flush_dependency(BlockDriverState *bs, Qcow2Cache *c)
{
    int ret;

    ret = qcow2_cache_flush(bs, c->depends);
    if (ret < 0) {
        return ret;
    }

    c->depends = NULL;
    c->depends_on_flush = false;

    return 0;
}

static int qcow2_cache_entry_flush(BlockDriverState *bs, Qcow2Cache *c, int i)
{
    BDRVQcow2State *s = bs->opaque;
    int ret = 0;

    if (!c->entries || !c->entries[i].dirty || !c->entries[i].offset) {
        return 0;
    }

    trace_qcow2_cache_entry_flush(qemu_coroutine_self(),
                                  c == s->l2_table_cache, i);

    if (c->depends) {
        ret = qcow2_cache_flush_dependency(bs, c);
    } else if (c->depends_on_flush) {
        ret = bdrv_flush(bs->file->bs);
        if (ret >= 0) {
            c->depends_on_flush = false;
        }
    }

    if (ret < 0) {
        return ret;
    }

    if (c == s->refcount_block_cache) {
        ret = qcow2_pre_write_overlap_check(bs, QCOW2_OL_REFCOUNT_BLOCK,
                c->entries[i].offset, c->table_size, false);
    } else if (c == s->l2_table_cache) {
        ret = qcow2_pre_write_overlap_check(bs, QCOW2_OL_ACTIVE_L2,
                c->entries[i].offset, c->table_size, false);
    } else {
        ret = qcow2_pre_write_overlap_check(bs, 0,
                c->entries[i].offset, c->table_size, false);
    }

    if (ret < 0) {
        return ret;
    }

    if (c == s->refcount_block_cache) {
        BLKDBG_EVENT(bs->file, BLKDBG_REFBLOCK_UPDATE_PART);
    } else if (c == s->l2_table_cache) {
        BLKDBG_EVENT(bs->file, BLKDBG_L2_UPDATE);
    }

    if(c == s->refcount_block_cache){
        ret = bdrv_pwrite(bs->file, c->entries[i].offset,
                        qcow2_cache_get_table_addr(c, i), c->table_size);
        if (ret < 0) {
            return ret;
        }
    }
    
    if(c == s->l2_table_cache){

        if(write_cache->entries[i].offset == 0){
            ret = bdrv_pwrite(bs->file, c->entries[i].offset,
                        qcow2_cache_get_table_addr(c, i), c->table_size);
            if (ret < 0) {
                return ret;
            }
        } else {
            uint64_t* buf1 = qcow2_cache_get_table_addr(c, i);
            uint64_t* buf2 = qcow2_cache_get_table_addr(write_cache, i);
            bool modif = false;
            for(int k = 0; k < c->table_size/8; k++){
                uint64_t l1 = be64_to_cpu(buf1[k]);
                uint64_t l2 = be64_to_cpu(buf2[k]);
                if(l1 == l2)
                    continue;
                bool cond1 = (get_l2_entry_backing_idx(&l1) == nb_ext_maxi);
                bool cond2 = qcow2_get_cluster_type(bs, l1)==QCOW2_CLUSTER_UNALLOCATED && get_l2_entry_backing_idx(&l1) > get_l2_entry_backing_idx(&l2);
                if(
                    (!cond1 && cond2) 
                    ||
                    (cond1 && !cond2)
                    ||
                    l2 == 0
                )
                {
                    // printf("%4d- flushing %lx, %lx\n", k, l1, l2);
                    // FILE* f = fopen("logger", "a");
                    // fprintf(f, "flushing - %lx - %lx - %d - %d\n", l1, l2, k, i);
                    // fclose(f);
                    buf2[k] = buf1[k];
                    modif = true;
                }
            }
            if(modif){
                // printf("curr_nb: %d, cur_max: %d\n", get_external_nb_snapshot_from_incompat(s->incompatible_features), nb_ext_maxi);
                // raise(SIGINT);
                ret = bdrv_pwrite(bs->file, c->entries[i].offset,
                                buf2, c->table_size);

                if (ret < 0) {
                    return ret;
                }
            }
        }
    }

    c->entries[i].dirty = false;

    return 0;
}

int qcow2_cache_write(BlockDriverState *bs, Qcow2Cache *c)
{
    BDRVQcow2State *s = bs->opaque;
    int result = 0;
    int ret;
    int i;

    trace_qcow2_cache_flush(qemu_coroutine_self(), c == s->l2_table_cache);

    for (i = 0; i < c->size; i++) {
        ret = qcow2_cache_entry_flush(bs, c, i);
        if (ret < 0 && result != -ENOSPC) {
            result = ret;
        }
    }

    return result;
}

int qcow2_cache_flush(BlockDriverState *bs, Qcow2Cache *c)
{
    int result = qcow2_cache_write(bs, c);

    if (result == 0) {
        int ret = bdrv_flush(bs->file->bs);
        if (ret < 0) {
            result = ret;
        }
    }

    return result;
}

int qcow2_cache_set_dependency(BlockDriverState *bs, Qcow2Cache *c,
    Qcow2Cache *dependency)
{
    int ret;

    if (dependency->depends) {
        ret = qcow2_cache_flush_dependency(bs, dependency);
        if (ret < 0) {
            return ret;
        }
    }

    if (c->depends && (c->depends != dependency)) {
        ret = qcow2_cache_flush_dependency(bs, c);
        if (ret < 0) {
            return ret;
        }
    }

    c->depends = dependency;
    return 0;
}

void qcow2_cache_depends_on_flush(Qcow2Cache *c)
{
    c->depends_on_flush = true;
}

int qcow2_cache_empty(BlockDriverState *bs, Qcow2Cache *c)
{
    int ret, i;

    ret = qcow2_cache_flush(bs, c);
    if (ret < 0) {
        return ret;
    }

    for (i = 0; i < c->size; i++) {
        assert(c->entries[i].ref == 0);
        c->entries[i].offset = 0;
        c->entries[i].lru_counter = 0;
    }

    qcow2_cache_table_release(c, 0, c->size);

    c->lru_counter = 0;

    return 0;
}
// int nb_cached = 0;
int nb_missed = 0;
int nb_missed_common = 0;
// float time_missed = 0;
// float time_total = 0;

static int qcow2_cache_do_get(BlockDriverState *bs, Qcow2Cache *c,
    uint64_t offset, void **table, bool read_from_disk, unsigned int l1_index, unsigned int start_slice)
{

#ifdef DEBUG_TIME
    int time_missed = clock();
    int time_hit = time_missed;
#endif
    // clock_t uptime = clock();
    bool missed = false;
    // printf("\t\t\t======entering cache=====\n");

    // nb_cached++;
    BDRVQcow2State *s = bs->opaque;
    int curr_nb = get_external_nb_snapshot_from_incompat(s->incompatible_features);
    bool is_last_bs = curr_nb == nb_ext_maxi && c==s->l2_table_cache;
    nb_ext_maxi = MAX(nb_ext_maxi, curr_nb);
    int i;
    int ret;
    int lookup_index;
    uint64_t min_lru_counter = UINT64_MAX;
    int min_lru_index = -1;

    assert(offset != 0);

    trace_qcow2_cache_get(qemu_coroutine_self(), c == s->l2_table_cache,
                          offset, read_from_disk);

    if (!QEMU_IS_ALIGNED(offset, c->table_size)) {
        qcow2_signal_corruption(bs, true, -1, -1, "Cannot get entry from %s "
                                "cache: Offset %#" PRIx64 " is unaligned",
                                qcow2_cache_get_name(s, c), offset);
        return -EIO;
    }

    /* Check if the table is already cached */
    i = lookup_index = (offset / c->table_size * 4) % c->size;
    do {
        const Qcow2CachedTable *t = &c->entries[i];
        // if (t->offset == offset) {
        //     goto found;
        // }
        if(c == s->l2_table_cache){
            /* retirer la condition sur l'offset
             * et verifier le remplacement de slice
             */
            if(t->offset == offset){//} && t->l1_index == l1_index && t->start_slice == start_slice){
                goto found;
            }
        } else {
            if (t->offset == offset) {
                goto found;
            }
        }
        if (t->ref == 0 && t->lru_counter < min_lru_counter) {
            min_lru_counter = t->lru_counter;
            min_lru_index = i;
        }
        if (++i == c->size) {
            i = 0;
        }
    } while (i != lookup_index);

    if (min_lru_index == -1) {
        /* This can't happen in current synchronous code, but leave the check
         * here as a reminder for whoever starts using AIO with the cache */
        abort();
    }

    /* Cache miss: write a table back and replace it */
    missed = true;
    i = min_lru_index;
    trace_qcow2_cache_get_replace_entry(qemu_coroutine_self(),
                                        c == s->l2_table_cache, i);

    ret = qcow2_cache_entry_flush(bs, c, i);
    if (ret < 0) {
        return ret;
    }

    trace_qcow2_cache_get_read(qemu_coroutine_self(),
                               c == s->l2_table_cache, i);
    c->entries[i].offset = 0;

    if (read_from_disk) {
        nb_missed++;
        if (c == s->l2_table_cache) {
            BLKDBG_EVENT(bs->file, BLKDBG_L2_LOAD);
        }

        ret = bdrv_pread(bs->file, offset,
                         qcow2_cache_get_table_addr(c, i),
                         c->table_size);
        void* wr = qcow2_cache_get_table_addr(write_cache, i);
        void* cr = qcow2_cache_get_table_addr(c, i);
    
        if(is_last_bs){
            memcpy(
                wr,
                cr,
                c->table_size
            );
        }

        if (ret < 0) {
            return ret;
        }
    }

    c->entries[i].offset = offset;
    c->entries[i].l1_index = l1_index;
    c->entries[i].start_slice = start_slice;
    if(is_last_bs){
        write_cache->entries[i].offset = offset;
        write_cache->entries[i].l1_index = l1_index;
        write_cache->entries[i].start_slice = start_slice;
    }

    /* And return the right table */
    // printf("missed\n");


found:

#ifdef DEBUG_TIME
    time_missed = clock() - time_missed;
    if(!missed) time_missed = 0;
#endif
    // printf("end\n");
    c->entries[i].ref++;

    *table = qcow2_cache_get_table_addr(c, i);

    trace_qcow2_cache_get_done(qemu_coroutine_self(),
                               c == s->l2_table_cache, i);

    // float time = 1000000 * (float)(clock() - uptime)/CLOCKS_PER_SEC;
    // if(missed){
    //     printf("time spend: %f\tmissed\n", time);
    //     time_missed += time;
    // }else{
    //     printf("time spend: %f\n", time);
    //     time_total += time;
    // }
    // printf("%d/%d (missed/cached)\n", nb_missed, nb_cached);
    // printf("%f/%f (missed/cached)\n", time_missed, time_total);
    // if(c==s->l2_table_cache){
    //     printf("slice in RAM %p: %d, %ld\n", &(c->entries[i]), i, offset);
    //     printf("to change cache %d, id_snap: %d\n", to_change_cache, curr_nb);
    //     printf("l1_index: %d, l2_slice: %d\n", current_l1_index, current_l2_slice_index);
    // }
    
    // if(!!c->entries[i].last_bs_req){
    bool modif = false;
    // printf("%d, %d, %d\n", nb_missed_common, nb_missed, missed);  
    if(c == s->l2_table_cache && !missed){// && !!c->entries[i].last_bs_req){
        BDRVQcow2State* sn = c->entries[i].last_bs_req->opaque;
        int ind_curr_back = get_external_nb_snapshot_from_incompat(s->incompatible_features);
        int ind_prev_back = get_external_nb_snapshot_from_incompat(sn->incompatible_features);
        // printf("curr %d, prev %d\n", ind_curr_back, ind_prev_back);
        // printf("qcow2_cache: offset: %ld, %d, %d\n\n", offset, start_slice, l1_index);
        // printf("index cache: %d\n", i);
        if(ind_curr_back != ind_prev_back){
            // printf("last: %p, curr: %p\n", c->entries[i].last_bs_req, bs);
            // printf("checker: %p\n", c->entries[i].last_bs_req->backing);
            if(read_from_disk){
                nb_missed_common++;
                // printf("%d, %d, %d\n", nb_missed_common, nb_missed, missed);
                uint64_t* buf = qemu_try_blockalign(bs, (size_t)c->table_size);
#ifdef DEBUG_TIME
    time_missed = time_missed - clock();
#endif
                int ret2 = bdrv_pread(bs->file, offset,
                         buf,
                        //  qcow2_cache_get_table_addr(c, i),
                         c->table_size);
#ifdef DEBUG_TIME
    time_missed = time_missed + clock();
#endif

                if (ret2 < 0) {
                    return ret2;
                }
                int y=0;
                
                uint64_t* a = buf;
                uint64_t* b = *table;
                for(y = 0; y < c->table_size/8; y++){
                    uint64_t aa = be64_to_cpu(a[y]);
                    uint64_t bb = be64_to_cpu(b[y]);

                    if(aa == bb || aa == 0 || get_l2_entry_backing_idx(&aa) > nb_ext_max || get_l2_entry_backing_idx(&bb) > nb_ext_max)
                        continue;
                        
                    if(
                        (qcow2_get_cluster_type(bs, bb) == QCOW2_CLUSTER_NORMAL &&
                         qcow2_get_cluster_type(bs, aa) == QCOW2_CLUSTER_NORMAL &&
                         get_l2_entry_backing_idx(&aa) >= get_l2_entry_backing_idx(&bb))
                        ||
                        (qcow2_get_cluster_type(bs, bb) == QCOW2_CLUSTER_UNALLOCATED &&
                         (qcow2_get_cluster_type(bs, aa) == QCOW2_CLUSTER_UNALLOCATED || qcow2_get_cluster_type(bs, aa) == QCOW2_CLUSTER_NORMAL) &&
                         (aa & 1ULL<<63) == 1ULL<<63 &&
                         (bb & 1ULL<<63) == 1ULL<<63 &&
                         get_l2_entry_backing_idx(&aa) >= get_l2_entry_backing_idx(&bb))
                        ||
                        (qcow2_get_cluster_type(bs, bb) == QCOW2_CLUSTER_ZERO_PLAIN &&
                         qcow2_get_cluster_type(bs, aa) == QCOW2_CLUSTER_NORMAL &&
                         (aa & 1ULL<<63) == 1ULL<<63 &&
                         (bb & 1ULL<<63) == 1ULL<<63 &&
                         get_l2_entry_backing_idx(&aa) >= get_l2_entry_backing_idx(&bb))
                    ){
                        // printf(
                        //         "%4d- comparison: %lx, %lx(aatype: %d, bbtype: %d)\t %ld\n",
                        //         y,
                        //         aa,
                        //         bb,
                        //         qcow2_get_cluster_type(bs, aa),
                        //         qcow2_get_cluster_type(bs, bb)
                        //         , offset);
                        b[y] = a[y];
                        modif = true;
                    }
                    // else{
                    //     printf(
                    //             "??? %4d- comparison: %lx, %lx(aatype: %d, bbtype: %d)\t %ld\n",
                    //             y,
                    //             aa,
                    //             bb,
                    //             qcow2_get_cluster_type(bs, aa),
                    //             qcow2_get_cluster_type(bs, bb)
                    //             , offset);
                    //     // if(i == 29)
                    //     // fprintf(ff, 
                    //     //     "??? %4d- comparison: %lx, %lx(aatype: %d, bbtype: %d)\t %ld\n",
                    //     //     y,
                    //     //     aa,
                    //     //     bb,
                    //     //     qcow2_get_cluster_type(bs, aa),
                    //     //     qcow2_get_cluster_type(bs, bb)
                    //     //     , offset);
                    // }
                }
                   
                if(modif){
                    // printf("\nPrinting comparison\n");
                    //raise(SIGINT);
                }
                *table = b;
                qemu_vfree(buf);
            }
        }
    }

    if(c == s->l2_table_cache)
        c->entries[i].last_bs_req = bs;

#ifdef DEBUG_TIME
    if(c == s->l2_table_cache){
        time_hit = clock() - time_missed - time_hit;
        LogDataTime tmplog1 = {
            .snap_id = get_external_nb_snapshot_from_incompat(s->incompatible_features),
            .time = time_hit
        };
        strcpy(tmplog1.event, "HIT");
        log_datas[index_log] = tmplog1;
        index_log++;
        //fprintf(file_tim2, "HIT;-1;%d\n", time_hit);
        if(missed){
            // fprintf(file_tim2, "MISSED;-1;%d\n", time_missed);
            LogDataTime tmplog2 = {
                .snap_id = get_external_nb_snapshot_from_incompat(s->incompatible_features),
                .time = time_missed
            };
            strcpy(tmplog2.event, "MISSED");
            log_datas[index_log] = tmplog2;
            index_log++;
        }
    }
#endif

    return 0;
}

int qcow2_cache_get(BlockDriverState *bs, Qcow2Cache *c, uint64_t offset,
    void **table, unsigned int l1_index, unsigned int start_slice)
{
    return qcow2_cache_do_get(bs, c, offset, table, true, l1_index, start_slice);
}

int qcow2_cache_get_empty(BlockDriverState *bs, Qcow2Cache *c, uint64_t offset,
    void **table, unsigned int l1_index, unsigned int start_slice)
{
    return qcow2_cache_do_get(bs, c, offset, table, false, l1_index, start_slice);
}

void qcow2_cache_put(Qcow2Cache *c, void **table)
{
    int i = qcow2_cache_get_table_idx(c, *table);

    c->entries[i].ref--;
    *table = NULL;

    if (c->entries[i].ref == 0) {
        c->entries[i].lru_counter = ++c->lru_counter;
    }

    assert(c->entries[i].ref >= 0);
}

void qcow2_cache_entry_mark_dirty(Qcow2Cache *c, void *table)
{
    int i = qcow2_cache_get_table_idx(c, table);
    assert(c->entries[i].offset != 0);
    c->entries[i].dirty = true;
}

void *qcow2_cache_is_table_offset(Qcow2Cache *c, uint64_t offset)
{
    int i;

    for (i = 0; i < c->size; i++) {
        if (c->entries[i].offset == offset) {
            return qcow2_cache_get_table_addr(c, i);
        }
    }
    return NULL;
}

void qcow2_cache_discard(Qcow2Cache *c, void *table)
{
    int i = qcow2_cache_get_table_idx(c, table);

    assert(c->entries[i].ref == 0);

    c->entries[i].offset = 0;
    c->entries[i].lru_counter = 0;
    c->entries[i].dirty = false;

    qcow2_cache_table_release(c, i, 1);
}
