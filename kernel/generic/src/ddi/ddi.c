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
#include <trace.h>

/** This lock protects the parea_btree. */
static mutex_t parea_lock;

/** B+tree with enabled physical memory areas. */
static btree_t parea_btree;

/** Initialize DDI.
 *
 */
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
NO_TRACE static int ddi_physmem_map(uintptr_t pf, uintptr_t vp, size_t pages,
    unsigned int flags)
{
	ASSERT(TASK);
	
	if ((pf % FRAME_SIZE) != 0)
		return EBADMEM;
	
	if ((vp % PAGE_SIZE) != 0)
		return EBADMEM;
	
	/*
	 * Unprivileged tasks are only allowed to map pareas
	 * which are explicitly marked as such.
	 */
	bool priv =
	    ((cap_get(TASK) & CAP_MEM_MANAGER) == CAP_MEM_MANAGER);
	
	mem_backend_data_t backend_data;
	backend_data.base = pf;
	backend_data.frames = pages;
	
	/*
	 * Check if the memory region is explicitly enabled
	 * for mapping by any parea structure.
	 */
	
	mutex_lock(&parea_lock);
	btree_node_t *nodep;
	parea_t *parea = (parea_t *) btree_search(&parea_btree,
	    (btree_key_t) pf, &nodep);
	
	if ((parea != NULL) && (parea->frames >= pages)) {
		if ((!priv) && (!parea->unpriv)) {
			mutex_unlock(&parea_lock);
			return EPERM;
		}
		
		goto map;
	}
	
	parea = NULL;
	mutex_unlock(&parea_lock);
	
	/*
	 * Check if the memory region is part of physical
	 * memory generally enabled for mapping.
	 */
	
	irq_spinlock_lock(&zones.lock, true);
	size_t znum = find_zone(ADDR2PFN(pf), pages, 0);
	
	if (znum == (size_t) -1) {
		/*
		 * Frames not found in any zone
		 * -> assume it is a hardware device and allow mapping
		 *    for privileged tasks.
		 */
		irq_spinlock_unlock(&zones.lock, true);
		
		if (!priv)
			return EPERM;
		
		goto map;
	}
	
	if (zones.info[znum].flags & ZONE_FIRMWARE) {
		/*
		 * Frames are part of firmware
		 * -> allow mapping for privileged tasks.
		 */
		irq_spinlock_unlock(&zones.lock, true);
		
		if (!priv)
			return EPERM;
		
		goto map;
	}
	
	irq_spinlock_unlock(&zones.lock, true);
	return ENOENT;
	
map:
	if (!as_area_create(TASK->as, flags, pages * PAGE_SIZE, vp,
	    AS_AREA_ATTR_NONE, &phys_backend, &backend_data)) {
		/*
		 * The address space area was not created.
		 * We report it using ENOMEM.
		 */
		
		if (parea != NULL)
			mutex_unlock(&parea_lock);
		
		return ENOMEM;
	}
	
	/*
	 * Mapping is created on-demand during page fault.
	 */
	
	if (parea != NULL) {
		parea->mapped = true;
		mutex_unlock(&parea_lock);
	}
	
	return EOK;
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
NO_TRACE static int ddi_iospace_enable(task_id_t id, uintptr_t ioaddr,
    size_t size)
{
	/*
	 * Make sure the caller is authorised to make this syscall.
	 */
	cap_t caps = cap_get(TASK);
	if (!(caps & CAP_IO_MANAGER))
		return EPERM;
	
	irq_spinlock_lock(&tasks_lock, true);
	
	task_t *task = task_find_by_id(id);
	
	if ((!task) || (!container_check(CONTAINER, task->container))) {
		/*
		 * There is no task with the specified ID
		 * or the task belongs to a different security
		 * context.
		 */
		irq_spinlock_unlock(&tasks_lock, true);
		return ENOENT;
	}
	
	/* Lock the task and release the lock protecting tasks_btree. */
	irq_spinlock_exchange(&tasks_lock, &task->lock);
	
	int rc = ddi_iospace_enable_arch(task, ioaddr, size);
	
	irq_spinlock_unlock(&task->lock, true);
	
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
sysarg_t sys_physmem_map(sysarg_t phys_base, sysarg_t virt_base,
    sysarg_t pages, sysarg_t flags)
{
	return (sysarg_t) ddi_physmem_map(ALIGN_DOWN((uintptr_t) phys_base,
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
sysarg_t sys_iospace_enable(ddi_ioarg_t *uspace_io_arg)
{
	ddi_ioarg_t arg;
	int rc = copy_from_uspace(&arg, uspace_io_arg, sizeof(ddi_ioarg_t));
	if (rc != 0)
		return (sysarg_t) rc;
	
	return (sysarg_t) ddi_iospace_enable((task_id_t) arg.task_id,
	    (uintptr_t) arg.ioaddr, (size_t) arg.size);
}

/** @}
 */
