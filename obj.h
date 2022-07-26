/*
 *   实现obj结构体的定义和相关函数的声明
 */

#include <stddef.h>
#include <stdint.h>

/*
 * Object handle
 */
typedef struct pmemoid {
	uint64_t pool_uuid_lo;
	uint64_t off;
} PMEMoid;


