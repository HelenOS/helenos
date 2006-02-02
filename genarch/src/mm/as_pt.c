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

#include <genarch/mm/page_pt.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <mm/as.h>
#include <arch/mm/page.h>
#include <arch/mm/as.h>
#include <arch/types.h>
#include <typedefs.h>
#include <memstr.h>
#include <arch.h>

static pte_t *ptl0_create(int flags);

as_operations_t as_pt_operations = {
	.page_table_create = ptl0_create
};

/** Create PTL0.
 *
 * PTL0 of 4-level page table will be created for each address space.
 *
 * @param flags Flags can specify whether ptl0 is for the kernel address space.
 *
 * @return New PTL0.
 */
pte_t *ptl0_create(int flags)
{
	pte_t *src_ptl0, *dst_ptl0;
	ipl_t ipl;

	dst_ptl0 = (pte_t *) frame_alloc(FRAME_KA | FRAME_PANIC, ONE_FRAME, NULL, NULL);

	if (flags & FLAG_AS_KERNEL) {
		memsetb((__address) dst_ptl0, PAGE_SIZE, 0);
	} else {
		/*
		 * Copy the kernel address space portion to new PTL0.
		 * TODO: copy only kernel address space.
		 */
		 
		ipl = interrupts_disable();
		spinlock_lock(&AS_KERNEL->lock);
		src_ptl0 = (pte_t *) PA2KA((__address) AS_KERNEL->page_table);
		memcpy((void *) dst_ptl0,(void *) src_ptl0, PAGE_SIZE);
		spinlock_unlock(&AS_KERNEL->lock);
		interrupts_restore(ipl);
	}

	return (pte_t *) KA2PA((__address) dst_ptl0);
}
