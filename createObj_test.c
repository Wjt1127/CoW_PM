/*
 * 实现在PM中记录 obj -> offset 的映射，我们需要在PM的固定地址处分配一个空间
 * 为了前期的简化操作，创建一个文件来代替PM的固定地址空间
 * 并测试、了解使用 ino + offset 打开文件的流程
 */

#include <stddef.h>
#include <stdint.h>
//#include <obj.h>
#include <string.h>
#include <sys/time.h>
#include <uuid/uuid.h>
#define ID_BYTES 8 //如果需要修改oid为128位，则改为16

/* 通过以下函数获得uuid,并以此标识obj */
unsigned long Oid_generate()
{
     uuid_t uu; // uuid_t类型就是 char [16]
     uuid_generate_time(uu);

     int i;
     if (ID_BYTES == 8) //返回64位ID
     {
          unsigned long Oid = 0; //需要返回的Oid
          for (i = 0; i < 8; i++)
          {
               Oid = (Oid << 8) + uu[i];
          }
          return Oid;
     }
     if (0) //如果需要128位将函数返回类型修改
     {
          if (ID_BYTES == 16) //返回128位ID
          {
               unsigned long Oid[2] = {0}; //需要返回的Oid
               for (i = 0; i < 8; i++)
               {
                    Oid[0] = (Oid[0] << 8) + uu[i];
               }
               for (i = 8; i < 16; i++)
               {
                    Oid[1] = (Oid[1] << 8) + uu[i];
               }
               return Oid;
          }
     }
}

int main()
{
     printf("%lX\n", Oid_generate());
     return 0;
}