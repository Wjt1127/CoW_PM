## CoW on PM
### 一次简单访问的流程:
     1.调用creatobj()实现obj到offset的映射。
     2.调用omap()由obj得到一个虚拟地址v。
     3. cpu拿这个虚拟地址访问:
        3.1 第一次访问页表没有这个虚拟地址的映射信息，出现page fault。
        3.2 转到page fault里面，通过VtoO()得到obj后，在pm的表里面得到obj到offset再到p的映射转化，最后返回一个p。
     4.最后根据v和p修改相应的页表项。

### Todo
     1.实现createobj()函数，输入fd和offset创建一个obj，同时在PM的表中添加这条映射信息，并返回该obj(id或者句柄之类的)。改写mmap实现。
     2.实现omap()函数，输入对象obj的标识和一些其他的标志位信息，返回一个虚拟地址指针，该指针指向该对象。改写mmap实现。
     3.createobj()和omap()实际上就是讲mmap的流程分为两步，在其中添加一层新的obj映射。
     4.实现VtoO()函数，输入虚拟地址可以返回和它对应的obj标识。可以在obj的某个结构体中添加一个成员变量，表示映射的虚拟地址。

### VtoO()函数的实现难点
     我们需要实现一个v到obj的表，并且这个表跟页表一样需要随着进程一起切换。
     方法：或许可以开辟一个大的表，使用pid来标识每个进程的v到obj的转换。可以使用TLB的ASID技术来实现。