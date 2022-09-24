#define PMEM_DIR "/mnt/pmem0/buddy_test"

#include <stdio.h>
#include <string.h>
#include "pmem_buddy.h"
#define BUDDY_PAGE_SIZE 4096

int main()
{
    // size_t max_size = 1024 * 1024 ;
    // size_t size = 1024 * 512;

    size_t max_size = 1024 * 1024 * 1024; // 1GB
    size_t size = 1024 * 1024 * 1024;      // 512MB
    pbuddy_alloc_t *allocator = pbuddy_alloc_init(PMEM_DIR, NULL, max_size, size);
    size_t buffer_size = BUDDY_PAGE_SIZE * 4;
    void *ptr = pbuddy_malloc(allocator, buffer_size);
    strcpy(ptr, "Hello, World!");
    printf("%s\n", (char *)ptr); 
    buddy_dbg_print(allocator);
    // pbuddy_free(allocator, ptr, buffer_size);
    // pbuddy_alloc_destroy(&allocator);
    return 0;
}