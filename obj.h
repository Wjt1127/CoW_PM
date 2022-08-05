/*
 *   实现obj结构体的定义和相关函数的声明
 */

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <uuid/uuid.h>
#include <linux/sched.h>
#include <asm/current.h>

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
typedef struct __OTE
{    
     //使用 ino + offset
     PMEMoid obj; //其中包含了offset
     unsigned long i_ino;  //inode number
} OTE;


#define ID_BYTES 8 //如果需要修改oid为128位，则改为16

uint64_t Oid_generate(); /* 通过以下函数获得uuid,并以此标识obj */

/*
 * 根据输入的 fd和 offset返回对应的对象句柄
 * 并在POT中建立 该对象到 offset的映射条目
 */
PMEMoid CreateObj(int fd,off_t offset);