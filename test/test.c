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

int main()
{
     int fd;
     fd = open("/home/wujintian/nvm_malloc/hybrid-memory-allocator/test/POT", O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);

     if (ftruncate(fd, 1024 * 1024 * 1024)) // 设置文件大小
     {
          printf("ftruncate failed\n");
          exit(-1);
     }

     return 0;
}