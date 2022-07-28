/*
 *   实现obj结构体的定义和相关函数的声明
 */

#include <stddef.h>
#include <stdint.h>

/*
 * Object handle
 */
typedef struct pmemoid {
	uint64_t objid;  //obj-id
	uint64_t off;   //offset
} PMEMoid;

/*
 * OTE:Object Table entry 
 * 记录obj到pm的映射
 */
typedef struct OTE
{    
     //使用 ino + offset
     PMEMoid obj; //其中包含了offset
     unsigned long i_ino;  //inode number
};

