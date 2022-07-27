#include <stddef.h>
#include <stdint.h>
#include <obj.h>

typedef struct OTE  //Object Table entry 记录obj到pm的映射
{    
     //使用 ino + offset
     PMEMoid obj; //其中包含了offset
     unsigned long i_ino;  //inode number

};
