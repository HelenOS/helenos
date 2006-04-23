/*
 * Copyright (C) 2006 Jakub Jermar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <ddi/ddi.h>
#include <ddi/ddi_arg.h>
#include <proc/task.h>
#include <security/cap.h>
#include <mm/frame.h>
#include <mm/page.h>
#include <mm/as.h>
#include <synch/spinlock.h>
#include <arch.h>
#include <align.h>
#include <errno.h>

/** Map piece of physical memory into virtual address space of specified task.
 *
 * @param id Task ID of the destination task.
 * @param pf Physical frame address of the starting frame.
 * @param vp Virtual page address of the starting page.
 * @param pages Number of pages to map.
 * @param writable If true, the mapping will be created writable.
 *
 * @return 0 on success, EPERM if the caller lacks capabilities to use this syscall,
 *	   ENOENT if there is no task matching the specified ID and ENOMEM if
 *	   there was a problem in creating address space area.
 */
static int ddi_physmem_map(task_id_t id, __address pf, __address vp, count_t pages, bool writable)
{
	ipl_t ipl;
	cap_t caps;
	task_t *t;
	int flags;
	count_t i;
	
	/*
	 * Make sure the caller is authorised to make this syscall.
	 */
	caps = cap_get(TASK);
	if (!(caps & CAP_MEM_MANAGER))
		return EPERM;
	
	ipl = interrupts_disable();
	spinlock_lock(&tasks_lock);
	
	t = task_find_by_id(id);
	
	if (!t) {
		/*
		 * There is no task with the specified ID.
		 */
		spinlock_unlock(&tasks_lock);
		interrupts_restore(ipl);
		return ENOENT;
	}

	/*
	 * TODO: We are currently lacking support for task destroying.
	 * Once it is added to the kernel, we must take care to
	 * synchronize in a way that prevents race conditions here.
	 */
	
	/* Lock the task and release the lock protecting tasks_btree. */
	spinlock_lock(&t->lock);
	spinlock_unlock(&tasks_lock);
	
	flags = AS_AREA_DEVICE | AS_AREA_READ;
	if (writable)
		flags |= AS_AREA_WRITE;
	if (!as_area_create(t->as, flags, pages * PAGE_SIZE, vp)) {
		/*
		 * The address space area could not have been created.
		 * We report it using ENOMEM.
		 */
		spinlock_unlock(&t->lock);
		interrupts_restore(ipl);
		return ENOMEM;
	}
	
	/* Initialize page tables. */
	for (i = 0; i < pages; i++)
		as_set_mapping(t->as, vp + i * PAGE_SIZE, pf + i * FRAME_SIZE);

	spinlock_unlock(&t->lock);
	interrupts_restore(ipl);
	return 0;
}

/** Enable range of I/O space for task.
 *
 * @param id Task ID of the destination task.
 * @param ioaddr Starting I/O address.
 * @param size Size of the enabled I/O space..
 *
 * @return 0 on success, EPERM if the caller lacks capabilities to use this syscall,
 *	   ENOENT if there is no task matching the specified ID.
 */
static int ddi_iospace_enable(task_id_t id, __address ioaddr, size_t size)
{
	ipl_t ipl;
	cap_t caps;
	task_t *t;
	int rc;
	
	/*
	 * Make sure the caller is authorised to make this syscall.
	 */
	caps = cap_get(TASK);
	if (!(caps & CAP_IO_MANAGER))
		return EPERM;
	
	ipl = interrupts_disable();
	spinlock_lock(&tasks_lock);
	
	t = task_find_by_id(id);
	
	if (!t) {
		/*
		 * There is no task with the specified ID.
		 */
		spinlock_unlock(&tasks_lock);
		interrupts_restore(ipl);
		return ENOENT;
	}

	/*
	 * TODO: We are currently lacking support for task destroying.
	 * Once it is added to the kernel, we must take care to
	 * synchronize in a way that prevents race conditions here.
	 */
	
	/* Lock the task and release the lock protecting tasks_btree. */
	spinlock_lock(&t->lock);
	spinlock_unlock(&tasks_lock);

	rc = ddi_iospace_enable_arch(t, ioaddr, size);
	
	spinlock_unlock(&t->lock);
	interrupts_restore(ipl);
	return rc;
}

/** Wrapper for SYS_MAP_PHYSMEM syscall.
 *
 * @param User space address of memory DDI argument structure.
 *
 * @return 0 on success, otherwise it returns error code found in errno.h
 */ 
__native sys_physmem_map(ddi_memarg_t *uspace_mem_arg)
{
	ddi_memarg_t arg;
	
	copy_from_uspace(&arg, uspace_mem_arg, sizeof(ddi_memarg_t));
	return (__native) ddi_physmem_map((task_id_t) arg.task_id, ALIGN_DOWN((__address) arg.phys_base, FRAME_SIZE),
					  ALIGN_DOWN((__address) arg.virt_base, PAGE_SIZE), (count_t) arg.pages,
					  (bool) arg.writable);
}

/** Wrapper for SYS_ENABLE_IOSPACE syscall.
 *
 * @param User space address of DDI argument structure.
 *
 * @return 0 on success, otherwise it returns error code found in errno.h
 */ 
__native sys_iospace_enable(ddi_ioarg_t *uspace_io_arg)
{
	ddi_ioarg_t arg;
	
	copy_from_uspace(&arg, uspace_io_arg, sizeof(ddi_ioarg_t));
	return (__native) ddi_iospace_enable((task_id_t) arg.task_id, (__address) arg.ioaddr, (size_t) arg.size);
}

__native ddi_int_control(__native enable, __native *flags)
{
	if (! cap_get(TASK) & CAP_INT_CONTROL)
		return EPERM;
	return ddi_int_control_arch(enable, flags);
}
