/*
 * Copyright (c) 2006 Martin Decky
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

/** @addtogroup ia32xen_mm	
 * @{
 */
/** @file
 * @ingroup ia32xen_mm
 */

#include <mm/tlb.h>
#include <arch/mm/asid.h>
#include <arch/asm.h>
#include <arch/types.h>
#include <arch/hypercall.h>

/** Invalidate all entries in TLB. */
void tlb_invalidate_all(void)
{
	mmuext_op_t mmu_ext;
	
	mmu_ext.cmd = MMUEXT_TLB_FLUSH_LOCAL;
	xen_mmuext_op(&mmu_ext, 1, NULL, DOMID_SELF);
}

/** Invalidate all entries in TLB that belong to specified address space.
 *
 * @param asid This parameter is ignored as the architecture doesn't support it.
 */
void tlb_invalidate_asid(asid_t asid)
{
	tlb_invalidate_all();
}

/** Invalidate TLB entries for specified page range belonging to specified address space.
 *
 * @param asid This parameter is ignored as the architecture doesn't support it.
 * @param page Address of the first page whose entry is to be invalidated.
 * @param cnt Number of entries to invalidate.
 */
void tlb_invalidate_pages(asid_t asid, uintptr_t page, count_t cnt)
{
	int i;

	for (i = 0; i < cnt; i++)
		invlpg(page + i * PAGE_SIZE);
}

/** @}
 */
