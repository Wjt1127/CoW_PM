#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/mman.h>
#include <linux/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>

#include "pmem_buddy.h"
#include "buddy_alloc.h"

/**
 * @brief Initialize pmem allocator.
 *
 * @param[in] file_fullpath
 * @param base_ptr Base address of the allocator. If NULL, it will be allocated.
 * @param max_size Size of the pool.
 * @param size
 * @return pbuddy_alloc_t* Pointer to the allocator.
 */
pbuddy_alloc_t *pbuddy_alloc_init(char *file_fullpath, void *base_ptr, size_t max_size, size_t size)
{
    pbuddy_alloc_t *allocator;
    int fd;
    void *addr;
    int file_fullpath_len;

    file_fullpath_len = strlen(file_fullpath);

    if (file_fullpath_len > PATH_MAX)
    {
        printf("Could not create file: too long path.");
        return NULL;
    }

    fd = open(file_fullpath, O_RDWR | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);

    if (ftruncate(fd, max_size)) // 设置文件大小
    {
        printf("ftruncate failed\n");
        goto exit;
    }

    // 将文件映射到内存。
    addr = mmap(base_ptr, max_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
    if (addr == MAP_FAILED)
    {
        printf("mmap failed\n");
        goto exit;
    }
    close(fd);

    //初始化分配器
    allocator = buddy_allocator_new(addr, max_size, size, file_fullpath);
    if (allocator == NULL)
    {
        printf("buddy_allocator_new failed\n");
        goto exit;
    }

    return allocator;

exit:
    if (fd != -1)
        (void)close(fd);
    if (file_fullpath != NULL)
        (void)munmap(file_fullpath, file_fullpath_len);
    return NULL;
}

/**
 * @brief 进行内存unmap 并删除文件。
 *
 * @param alloc_ptr pmem_alloc结构的地址。
 * @return int 成功 0, 失败 -1.
 */
int pbuddy_alloc_destroy(pbuddy_alloc_t **alloc_ptr)
{
    if (munmap((void *)((*alloc_ptr)->page_start), (*alloc_ptr)->alloc_size))
    {
        printf("munmap failed\n");
        return -1;
    }
    if (unlink((*alloc_ptr)->file_fullpath))
    {
        printf("unlink failed\n");
        return -1;
    }
    if (munmap((void *)(*alloc_ptr)->file_fullpath, strlen((*alloc_ptr)->file_fullpath) + 1))
    {
        printf("munmap failed\n");
        return -1;
    }
    if (munmap((void *)(*alloc_ptr), (*alloc_ptr)->reserved))
    {
        printf("munmap failed\n");
        return -1;
    }
    return 0;
}

void *pbuddy_malloc(pbuddy_alloc_t *allocator, size_t size)
{
    return buddy_malloc(allocator, size);
}

void pbuddy_free(pbuddy_alloc_t *allocator, void *ptr, size_t size)
{
    buddy_free(allocator, ptr, size);
}