/*
 * Copyright (c) 2006 Jakub Jermar
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

/** @addtogroup genericddi
 * @{
 */

/**
 * @file
 * @brief Device Driver Interface functions.
 *
 * This file contains functions that comprise the Device Driver Interface.
 * These are the functions for mapping physical memory and enabling I/O
 * space to tasks.
 */

#include <ddi/ddi.h>
#include <ddi/ddi_arg.h>
#include <proc/task.h>
#include <security/cap.h>
#include <mm/frame.h>
#include <mm/as.h>
#include <synch/mutex.h>
#include <syscall/copy.h>
#include <adt/btree.h>
#include <arch.h>
#include <align.h>
#include <errno.h>

/** This lock protects the parea_btree. */
static mutex_t parea_lock;

/** B+tree with enabled physical memory areas. */
static btree_t parea_btree;

/** Initialize DDI. */
void ddi_init(void)
{
	btree_create(&parea_btree);
	mutex_initialize(&parea_lock, MUTEX_PASSIVE);
}

/** Enable piece of physical memory for mapping by physmem_map().
 *
 * @param parea Pointer to physical area structure.
 *
 */
void ddi_parea_register(parea_t *parea)
{
	mutex_lock(&parea_lock);
	
	/*
	 * We don't check for overlaps here as the kernel is pretty sane.
	 */
	btree_insert(&parea_btree, (btree_key_t) parea->pbase, parea, NULL);
	
	mutex_unlock(&parea_lock);
}

/** Map piece of physical memory into virtual address space of current task.
 *
 * @param pf    Physical address of the starting frame.
 * @param vp    Virtual address of the starting page.
 * @param pages Number of pages to map.
 * @param flags Address space area flags for the mapping.
 *
 * @return 0 on success, EPERM if the caller lacks capabilities to use this
 *         syscall, EBADMEM if pf or vf is not page aligned, ENOENT if there
 *         is no task matching the specified ID or the physical address space
 *         is not enabled for mapping and ENOMEM if there was a problem in
 *         creating address space area.
 *
 */
static int ddi_physmem_map(uintptr_t pf, uintptr_t vp, size_t pages, int flags)
{
	ASSERT(TASK);
	ASSERT((pf % FRAME_SIZE) == 0);
	ASSERT((vp % PAGE_SIZE) == 0);
	
	/*
	 * Make sure the caller is authorised to make this syscall.
	 */
	cap_t caps = cap_get(TASK);
	if (!(caps & CAP_MEM_MANAGER))
		return EPERM;
	
	mem_backend_data_t backend_data;
	backend_data.base = pf;
	backend_data.frames = pages;
	
	ipl_t ipl = interrupts_disable();
	
	/* Find the zone of the physical memory */
	spinlock_lock(&zones.lock);
	size_t znum = find_zone(ADDR2PFN(pf), pages, 0);
	
	if (znum == (size_t) -1) {
		/* Frames not found in any zones
		 * -> assume it is hardware device and allow mapping
		 */
		spinlock_unlock(&zones.lock);
		goto map;
	}
	
	if (zones.info[znum].flags & ZONE_FIRMWARE) {
		/* Frames are part of firmware */
		spinlock_unlock(&zones.lock);
		goto map;
	}
	
	if (zone_flags_available(zones.info[znum].flags)) {
		/* Frames are part of physical memory, check if the memory
		 * region is enabled for mapping.
		 */
		spinlock_unlock(&zones.lock);
		
		mutex_lock(&parea_lock);
		btree_node_t *nodep;
		parea_t *parea = (parea_t *) btree_search(&parea_btree,
		    (btree_key_t) pf, &nodep);
		
		if ((!parea) || (parea->frames < pages)) {
			mutex_unlock(&parea_lock);
			goto err;
		}
		
		mutex_unlock(&parea_lock);
		goto map;
	}
	
	spinlock_unlock(&zones.lock);
err:
	interrupts_restore(ipl);
	return ENOENT;
	
map:
	spinlock_lock(&TASK->lock);
	
	if (!as_area_create(TASK->as, flags, pages * PAGE_SIZE, vp,
	    AS_AREA_ATTR_NONE, &phys_backend, &backend_data)) {
		/*
		 * The address space area could not have been created.
		 * We report it using ENOMEM.
		 */
		spinlock_unlock(&TASK->lock);
		interrupts_restore(ipl);
		return ENOMEM;
	}
	
	/*
	 * Mapping is created on-demand during page fault.
	 */
	
	spinlock_unlock(&TASK->lock);
	interrupts_restore(ipl);
	return 0;
}

/** Enable range of I/O space for task.
 *
 * @param id Task ID of the destination task.
 * @param ioaddr Starting I/O address.
 * @param size Size of the enabled I/O space..
 *
 * @return 0 on success, EPERM if the caller lacks capabilities to use this
 *           syscall, ENOENT if there is no task matching the specified ID.
 *
 */
static int ddi_iospace_enable(task_id_t id, uintptr_t ioaddr, size_t size)
{
	/*
	 * Make sure the caller is authorised to make this syscall.
	 */
	cap_t caps = cap_get(TASK);
	if (!(caps & CAP_IO_MANAGER))
		return EPERM;
	
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&tasks_lock);
	
	task_t *task = task_find_by_id(id);
	
	if ((!task) || (!context_check(CONTEXT, task->context))) {
		/*
		 * There is no task with the specified ID
		 * or the task belongs to a different security
		 * context.
		 */
		spinlock_unlock(&tasks_lock);
		interrupts_restore(ipl);
		return ENOENT;
	}
	
	/* Lock the task and release the lock protecting tasks_btree. */
	spinlock_lock(&task->lock);
	spinlock_unlock(&tasks_lock);
	
	int rc = ddi_iospace_enable_arch(task, ioaddr, size);
	
	spinlock_unlock(&task->lock);
	interrupts_restore(ipl);
	
	return rc;
}

/** Wrapper for SYS_PHYSMEM_MAP syscall.
 *
 * @param phys_base Physical base address to map
 * @param virt_base Destination virtual address
 * @param pages Number of pages
 * @param flags Flags of newly mapped pages
 *
 * @return 0 on success, otherwise it returns error code found in errno.h
 *
 */
unative_t sys_physmem_map(unative_t phys_base, unative_t virt_base,
    unative_t pages, unative_t flags)
{
	return (unative_t) ddi_physmem_map(ALIGN_DOWN((uintptr_t) phys_base,
	    FRAME_SIZE), ALIGN_DOWN((uintptr_t) virt_base, PAGE_SIZE),
	    (size_t) pages, (int) flags);
}

/** Wrapper for SYS_ENABLE_IOSPACE syscall.
 *
 * @param uspace_io_arg User space address of DDI argument structure.
 *
 * @return 0 on success, otherwise it returns error code found in errno.h
 *
 */
unative_t sys_iospace_enable(ddi_ioarg_t *uspace_io_arg)
{
	ddi_ioarg_t arg;
	int rc = copy_from_uspace(&arg, uspace_io_arg, sizeof(ddi_ioarg_t));
	if (rc != 0)
		return (unative_t) rc;
	
	return (unative_t) ddi_iospace_enable((task_id_t) arg.task_id,
	    (uintptr_t) arg.ioaddr, (size_t) arg.size);
}

/** Disable or enable preemption.
 *
 * @param enable If non-zero, the preemption counter will be decremented,
 *               leading to potential enabling of preemption. Otherwise
 *               the preemption counter will be incremented, preventing
 *               preemption from occurring.
 *
 * @return Zero on success or EPERM if callers capabilities are not sufficient.
 *
 */
unative_t sys_preempt_control(int enable)
{
	if (!(cap_get(TASK) & CAP_PREEMPT_CONTROL))
		return EPERM;
	
	if (enable)
		preemption_enable();
	else
		preemption_disable();
	
	return 0;
}

/** @}
 */
