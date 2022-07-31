/*
 * 实现omap()调用的相关函数
 * omap()需要为给定的obj分配一个虚拟地址 v
 * 参考mmap实现
 */

#include <obj.h>



/*
 * 原本是使用file对应的get_unmapped_area的 ：
 *         get_area = file->f_op->get_unmapped_area;
 * 这里应该要改成obj的 应该在obj的结构体中加一个类似file的方法就行
 */
unsigned long
get_unmapped_area(struct file *file, unsigned long addr, unsigned long len,
		unsigned long pgoff, unsigned long flags)
{
	unsigned long (*get_area)(struct file *, unsigned long,
				  unsigned long, unsigned long, unsigned long);
	/* 针对特定平台的检查，目前arm64中arch_mmap_check 是一个空函数 */
	unsigned long error = arch_mmap_check(addr, len, flags);
	if (error)
		return error;

	/* 申请虚拟空间的地址不能超过最大值。这里可以知道虚拟空间size 的最大值就是TASK_SIZE */
	/* Careful about overflows.. */
	if (len > TASK_SIZE)
		return -ENOMEM;
	//指向当前进程的unmap空间分配函数
	get_area = current->mm->get_unmapped_area;
	/* file 不为空的话，则 unmap空间分配函数 执行file中指定的函数 */
	if (file) {
		if (file->f_op->get_unmapped_area)
			get_area = file->f_op->get_unmapped_area;
	} else if (flags & MAP_SHARED) {
		/* 如果file为空，说明可能申请的是匿名空间，这里检查
		   如果是共享内存的话，则分配函数执行共享内存的分配函数 */
		/*
		 * mmap_region() will call shmem_zero_setup() to create a file,
		 * so use shmem's get_unmapped_area in case it can be huge.
		 * do_mmap_pgoff() will clear pgoff, so match alignment.
		 */
		pgoff = 0;
		 //如果是共享内存的话 选择使用share_memory的 unmap空间分配函数
		get_area = shmem_get_unmapped_area;
	}

	/* 前面都是根据参数或一些判断 来选择使用哪种get_area函数
	   (进程地址空间中没有被分配的空间) */
	addr = get_area(file, addr, len, pgoff, flags);
	if (IS_ERR_VALUE(addr))
		return addr;

	/* addr +len 不能大于TASK_SIZE */
	if (addr > TASK_SIZE - len)
		return -ENOMEM;

	/* 检查分配到的地址是否已经被映射，
	   如果已经被映射则返回error，毕竟这里要分配的是进程未映射的空间。*/
	if (offset_in_page(addr))
		return -EINVAL;

	/* secure检查 */
	error = security_mmap_addr(addr);
	return error ? error : addr;
}


/*
 * vma->vm_file = get_file(file)建立文件与vma的映射, 
 * 在其中调用 mmap_region() 创建虚拟内存区域
 */
unsigned long do_omap(struct file *file, unsigned long addr,
			unsigned long len, unsigned long prot,
			unsigned long flags, vm_flags_t vm_flags,
			unsigned long pgoff, unsigned long *populate,
			struct list_head *uf)
{
	// current 表示当进程
	struct mm_struct *mm = current->mm; //获取该进程的memory descriptor
	int pkey = 0;

	*populate = 0;

	/* 函数对传入的参数进行一系列检查, 假如任一参数出错，都会返回一个errno */
	if (!len)
		return -EINVAL;

	if (!(flags & MAP_FIXED))
		addr = round_hint_to_min(addr);

	/* Careful about overflows.. */
	/* PAGE_ALIGN(addr)  就是  (((addr)+PAGE_SIZE-1)&PAGE_MASK) */
	len = PAGE_ALIGN(len);   // 进行页大小对齐
	if (!len)
		return -ENOMEM;

	/* offset overflow? */
	/* 正常应该是 大与等于的 如果小于说明溢出了 */
	if ((pgoff + (len >> PAGE_SHIFT)) < pgoff)
		return -EOVERFLOW;

	/* Too many mappings? */
	/* 判断该进程的地址空间的虚拟区间数量是否超过了限制 */
	if (mm->map_count > sysctl_max_map_count)
		return -ENOMEM;

	/* Obtain the address to map to. we verify (or select) it and ensure
	 * that it represents a valid section of the address space.
	 */
	/* get_unmapped_area从当前进程的用户空间获取一个未被映射区间的起始地址 */
	addr = get_unmapped_area(file, addr, len, pgoff, flags);
	if (offset_in_page(addr))
		return addr;

	/* 如果是保护执行的话 则生成一个pkey */
	if (prot == PROT_EXEC) {
		pkey = execute_only_pkey(mm);
		if (pkey < 0)
			pkey = 0;
	}

	/* Do simple checking here so the lower-level routines won't have
	 * to. we assume access permissions have been handled by the open
	 * of the memory object, so we don't do any here.
	 */
	vm_flags |= calc_vm_prot_bits(prot, pkey) | calc_vm_flag_bits(flags) |
			mm->def_flags | VM_MAYREAD | VM_MAYWRITE | VM_MAYEXEC;

	if (flags & MAP_LOCKED)
		if (!can_do_mlock())  //检查是否可以锁
			return -EPERM;

	if (mlock_future_check(mm, vm_flags, len))
		return -EAGAIN;

	/* file指针不为nullptr, 即从文件到虚拟空间的映射 */
	if (file) {
		struct inode *inode = file_inode(file);
		/*
           根据标志指定的map种类，把为文件设置的访问权考虑进去。
		 如果所请求的内存映射是共享可写的，就要检查要映射的文件是为写入而打开的，
		 而不是以追加模式打开的，还要检查文件上没有上强制锁。
		 对于任何种类的内存映射，都要检查文件是否为读操作而打开的。
		*/	
		switch (flags & MAP_TYPE) {
		case MAP_SHARED:
			if ((prot&PROT_WRITE) && !(file->f_mode&FMODE_WRITE))
				return -EACCES;

			/*
			 * Make sure we don't allow writing to an append-only
			 * file..
			 */
			if (IS_APPEND(inode) && (file->f_mode & FMODE_WRITE))
				return -EACCES;

			/*
			 * Make sure there are no mandatory locks on the file.
			 */
			if (locks_verify_locked(file))
				return -EAGAIN;

			vm_flags |= VM_SHARED | VM_MAYSHARE;
			if (!(file->f_mode & FMODE_WRITE))
				vm_flags &= ~(VM_MAYWRITE | VM_SHARED);

			/* fall through */
		case MAP_PRIVATE:
			if (!(file->f_mode & FMODE_READ))
				return -EACCES;
			if (path_noexec(&file->f_path)) {
				if (vm_flags & VM_EXEC)
					return -EPERM;
				vm_flags &= ~VM_MAYEXEC;
			}

			if (!file->f_op->mmap)
				return -ENODEV;
			if (vm_flags & (VM_GROWSDOWN|VM_GROWSUP))
				return -EINVAL;
			break;

		default:
			return -EINVAL;
		}
	} else {
		switch (flags & MAP_TYPE) {
		case MAP_SHARED:
			if (vm_flags & (VM_GROWSDOWN|VM_GROWSUP))
				return -EINVAL;
			/*
			 * Ignore pgoff.
			 */
			pgoff = 0;
			vm_flags |= VM_SHARED | VM_MAYSHARE;
			break;
		case MAP_PRIVATE:
			/*
			 * Set pgoff according to addr for anon_vma.
			 */
			pgoff = addr >> PAGE_SHIFT;
			break;
		default:
			return -EINVAL;
		}
	}

	/*
	 * Set 'VM_NORESERVE' if we should not account for the
	 * memory use of this mapping.
	 */
	if (flags & MAP_NORESERVE) {
		/* We honor MAP_NORESERVE if allowed to overcommit */
		if (sysctl_overcommit_memory != OVERCOMMIT_NEVER)
			vm_flags |= VM_NORESERVE;

		/* hugetlb applies strict overcommit unless MAP_NORESERVE */
		if (file && is_file_hugepages(file))
			vm_flags |= VM_NORESERVE;
	}
	// mmap_region()负责创建虚拟内存区域
	addr = mmap_region(file, addr, len, vm_flags, pgoff, uf);
	if (!IS_ERR_VALUE(addr) &&
	    ((vm_flags & VM_LOCKED) ||
	     (flags & (MAP_POPULATE | MAP_NONBLOCK)) == MAP_POPULATE))
		*populate = len;
	return addr;
}