/*
 * 先跳过PM的分配问题，在用户态建立一个文件来记录POT(persisten-obj-table)。
 */
#include <obj.h>

/* 通过以下函数获得uuid,并以此标识obj */
uint64_t Oid_generate()
{
     uuid_t uu; // uuid_t类型就是 char [16]
     uuid_generate_time(uu);

     int i;
     if (ID_BYTES == 8) //返回64位ID
     {
          uint64_t Oid = 0; //需要返回的Oid
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
               uint64_t Oid[2] = {0}; //需要返回的Oid
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


/*
 * 根据输入的 fd和 offset返回对应的对象句柄
 * 并在POT中建立 该对象到 offset的映射条目
 */
PMEMoid CreateObj(int fd,off_t offset)
{
     struct file *file;
     PMEMoid obj;
     OTE add_ote;

     /* 后期需要去除打开pot文件这段 实现一个函数获得POT的基址 */
     FILE* fd_pot;
     if((fd_pot = fopen("POT","ab+")) == NULL){  /* POT是 Persistent Object Table
               为了简化前期的工作 使用POT文件来记录object 到 offset的映射*/
          perror("fopen");
		exit(1);
     }

     /* 获得 fd 对应的inode numeber */
     struct files_struct *files = current->files;  //获取本进程的files结构体
     file = fcheck_files(files, fd); //根据files_struct结构获取file结构体
     struct inode *finode = file->f_inode;
     unsigned long ino = finode->i_ino;  //获得 inode number
     
     /* 初始化对象句柄 */
     obj.objid = Oid_generate();
     obj.off = offset;

     /* 测试输出 */
     printf("id of the obj:%lX\n",obj.objid);
     printf("ino of the obj:%ld\n",ino);
     printf("offset of the obj:%ld\n",obj.off);

     /* 在POT中添加这条映射 */
     add_ote.obj = obj;
     add_ote.i_ino = ino;
     fwrite(&add_ote,sizeof(add_ote),1,fd_pot);  //没有配好qemu 后续再修改

     return obj;
}