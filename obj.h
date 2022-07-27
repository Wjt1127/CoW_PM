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


