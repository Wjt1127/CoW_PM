/**
 * @file    buddy_alloc.h
 * @brief   Buddy allocator
 *
 */

#ifndef _BUDDY_ALLOC_H
#define _BUDDY_ALLOC_H

#include <sys/types.h>
#include <pthread.h>
#include <stdint.h>
#include "list.h"

#define BUDDY_PAGE_SHIFT 12
#define BUDDY_MAX_SHIFT 30

#define BUDDY_PAGESIZE ((uint64_t)(1U << BUDDY_PAGE_SHIFT))     /* 4096 */
#define BUDDY_MAX_CHUNKSIZE ((uint64_t)(1U << BUDDY_MAX_SHIFT)) /* 1G */

#define BUDDY_BINS_CNT (BUDDY_MAX_SHIFT - BUDDY_PAGE_SHIFT + 1) /* buddy allocator的层数 */

#define MAX(a, b) (((a) >= (b)) ? (a) : (b))
#define MIN(a, b) (((a) <= (b)) ? (a) : (b))

typedef struct buddy_chunk_s buddy_chunk_t;
struct buddy_chunk_s
{
    list_link_t link;
};

/* bin #0: 4K-size chunk (BUDDY_PAGESIZE)
 * bin #1: 8K-size chunk
 * ...
 * bin #(max): 1G-size chunk (BUDDY_MAX_CHUNKSIZE)
 *
 * bitmap: 0 indicates free, 1 indicates allocated. If any chunk is free,
 * the bits corresponding to the subchunk that cut the chunk are all 1.
 *
 * For example, if there are a total of 10 pages and there are two free 
 * chunks as follows:
 *
 *            AAFF AAAF AA  (A: allocated, F: free)
 *
 * bitmap #0: 1111 1110 111 (last bit: sentinel)
 * bitmap #1: 1 0  1 1  1 1
 * bitmap #2: 1    1    1
 * bitmap #3: 1         1
 * bitmap #4: 1
 */
typedef struct pbuddy_alloc_s
{
    pthread_mutex_t mutex;

    char *file_fullpath;         // 文件的完整路径
    char *page_start;            // buddy chunk的起始地址
    void *pot_start;             // POT的起始地址
    uint64_t reserved;           // 元数据空间的大小
    uint64_t alloc_size;         // 
    uint64_t max_available_size; // 考虑到未来扩展可使用的总空间大小
    uint64_t available_size;     // buddy_malloc 当前可用的空间量
    uint64_t total_used;
    uint64_t periodic_total_used_max;

    list_t bins[BUDDY_BINS_CNT];

    char *bitmap[BUDDY_BINS_CNT];
    int bitmap_size[BUDDY_BINS_CNT];
} pbuddy_alloc_t;

pbuddy_alloc_t *buddy_allocator_new(void *base_ptr, uint64_t max_size, uint64_t size, char *file_fullpath);
void buddy_allocator_expand(pbuddy_alloc_t *alloc,
                            uint64_t old_size, uint64_t new_size);
void *buddy_malloc(pbuddy_alloc_t *alloc, uint64_t size);
void buddy_free(pbuddy_alloc_t *alloc, void *page, uint64_t size);

void buddy_dbg_print(pbuddy_alloc_t *alloc);
void get_buddy_alloc_state(pbuddy_alloc_t *alloc,
                           uint64_t *total_size, uint64_t *used_size);
uint64_t get_buddy_alloc_total_size(pbuddy_alloc_t *alloc);
uint64_t get_buddy_alloc_size(uint64_t size);
uint64_t get_buddy_alloc_size_rounddown(uint64_t size);

typedef struct region_s region_t;
struct region_s
{
    region_t *prev;
    region_t *next;
    int alloc_idx; /* allocator index: only for shared pool allocator */
    uint64_t size; /* size of this region, including region_t header */
};

#endif /* not _BUDDY_ALLOC_H */