/*
 * 实现omap()调用的相关函数
 * omap()需要为给定的obj分配一个虚拟地址 v
 * 参考mmap实现
 */

#include <obj.h>

/*
 * vma->vm_file = get_file(file)建立文件与vma的映射, 
 * 在其中调用 mmap_region() 创建虚拟内存区域
 */
unsigned long do_mmap(struct file *file, unsigned long addr,
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

	/*
	 * Does the application expect PROT_READ to imply PROT_EXEC?
	 *
	 * (the exception is when the underlying filesystem is noexec
	 *  mounted, in which case we dont add PROT_EXEC.)
	 */
	if ((prot & PROT_READ) && (current->personality & READ_IMPLIES_EXEC))
		if (!(file && path_noexec(&file->f_path)))
			prot |= PROT_EXEC;
	/* 假如没有设置MAP_FIXED标志，且addr小于mmap_min_addr, 因为可以修改addr, 
	   所以就需要将addr设为mmap_min_addr的页对齐后的地址 */
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