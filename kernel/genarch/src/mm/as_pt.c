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

/** @addtogroup kernel_genarch_mm
 * @{
 */

/**
 * @file
 * @brief Address space functions for 4-level hierarchical pagetables.
 */

#include <genarch/mm/page_pt.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <mm/as.h>
#include <synch/mutex.h>
#include <arch/mm/page.h>
#include <arch/mm/as.h>
#include <typedefs.h>
#include <memw.h>
#include <arch.h>

static pte_t *ptl0_create(unsigned int);
static void ptl0_destroy(pte_t *);

static void pt_lock(as_t *, bool);
static void pt_unlock(as_t *, bool);
static bool pt_locked(as_t *);

const as_operations_t as_pt_operations = {
	.page_table_create = ptl0_create,
	.page_table_destroy = ptl0_destroy,
	.page_table_lock = pt_lock,
	.page_table_unlock = pt_unlock,
	.page_table_locked = pt_locked,
};

/** Create PTL0.
 *
 * PTL0 of 4-level page table will be created for each address space.
 *
 * @param flags Flags can specify whether ptl0 is for the kernel address space.
 *
 * @return New PTL0.
 *
 */
pte_t *ptl0_create(unsigned int flags)
{
	pte_t *dst_ptl0 = (pte_t *)
	    PA2KA(frame_alloc(PTL0_FRAMES, FRAME_LOWMEM, PTL0_SIZE - 1));

	memsetb(dst_ptl0, PTL0_SIZE, 0);

	if (!KERNEL_SEPARATE_PTL0 && !(flags & FLAG_AS_KERNEL)) {
		/*
		 * Copy the kernel address space portion to new PTL0.
		 */

		mutex_lock(&AS_KERNEL->lock);

		pte_t *src_ptl0 =
		    (pte_t *) PA2KA((uintptr_t) AS_KERNEL->genarch.page_table);

		uintptr_t src = (uintptr_t)
		    &src_ptl0[PTL0_INDEX(KERNEL_ADDRESS_SPACE_START)];
		uintptr_t dst = (uintptr_t)
		    &dst_ptl0[PTL0_INDEX(KERNEL_ADDRESS_SPACE_START)];

		memcpy((void *) dst, (void *) src,
		    PTL0_SIZE - (src - (uintptr_t) src_ptl0));

		mutex_unlock(&AS_KERNEL->lock);
	}

	return (pte_t *) KA2PA((uintptr_t) dst_ptl0);
}

/** Destroy page table.
 *
 * Destroy PTL0, other levels are expected to be already deallocated.
 *
 * @param page_table Physical address of PTL0.
 *
 */
void ptl0_destroy(pte_t *page_table)
{
	frame_free((uintptr_t) page_table, PTL0_FRAMES);
}

/** Lock page tables.
 *
 * Lock only the address space.
 * Interrupts must be disabled.
 *
 * @param as   Address space.
 * @param lock If false, do not attempt to lock the address space.
 *
 */
void pt_lock(as_t *as, bool lock)
{
	if (lock)
		mutex_lock(&as->lock);
}

/** Unlock page tables.
 *
 * Unlock the address space.
 * Interrupts must be disabled.
 *
 * @param as     Address space.
 * @param unlock If false, do not attempt to unlock the address space.
 *
 */
void pt_unlock(as_t *as, bool unlock)
{
	if (unlock)
		mutex_unlock(&as->lock);
}

/** Test whether page tables are locked.
 *
 * @param as		Address space where the page tables belong.
 *
 * @return		True if the page tables belonging to the address soace
 *			are locked, otherwise false.
 */
bool pt_locked(as_t *as)
{
	return mutex_locked(&as->lock);
}

/** @}
 */
