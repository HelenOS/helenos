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

/** @addtogroup sparc64mm
 * @{
 */
/** @file
 */

#include <arch/mm/as.h>
#include <arch/mm/tlb.h>
#include <genarch/mm/as_ht.h>
#include <genarch/mm/asid_fifo.h>
#include <debug.h>

#ifdef CONFIG_TSB
#include <arch/mm/tsb.h>
#endif

/** Architecture dependent address space init. */
void as_arch_init(void)
{
	as_operations = &as_ht_operations;
	asid_fifo_init();
}

/** Perform sparc64-specific tasks when an address space becomes active on the processor.
 *
 * Install ASID and map TSBs.
 *
 * @param as Address space.
 */
void as_install_arch(as_t *as)
{
	tlb_context_reg_t ctx;
	
	/*
	 * Note that we don't lock the address space.
	 * That's correct - we can afford it here
	 * because we only read members that are
	 * currently read-only.
	 */
	
	/*
	 * Write ASID to secondary context register.
	 * The primary context register has to be set
	 * from TL>0 so it will be filled from the
	 * secondary context register from the TL=1
	 * code just before switch to userspace.
	 */
	ctx.v = 0;
	ctx.context = as->asid;
	mmu_secondary_context_write(ctx.v);

#ifdef CONFIG_TSB	
	if (as != AS_KERNEL) {
		uintptr_t base = ALIGN_DOWN(config.base, 1 << KERNEL_PAGE_WIDTH);

		ASSERT(as->arch.itsb && as->arch.dtsb);

		uintptr_t tsb = as->arch.itsb;
		
		if (!overlaps(tsb, 8*PAGE_SIZE, base, 1 << KERNEL_PAGE_WIDTH)) {
			/*
			 * TSBs were allocated from memory not covered
			 * by the locked 4M kernel DTLB entry. We need
			 * to map both TSBs explicitly.
			 */
			dtlb_demap(TLB_DEMAP_PAGE, TLB_DEMAP_NUCLEUS, tsb);
			dtlb_insert_mapping(tsb, KA2PA(tsb), PAGESIZE_64K, true, true);
		}
		
		/*
		 * Setup TSB Base registers.
		 */
		tsb_base_reg_t tsb_base;
		
		tsb_base.value = 0;
		tsb_base.size = TSB_SIZE;
		tsb_base.split = 0;

		tsb_base.base = as->arch.itsb >> PAGE_WIDTH;
		itsb_base_write(tsb_base.value);
		tsb_base.base = as->arch.dtsb >> PAGE_WIDTH;
		dtsb_base_write(tsb_base.value);
	}
#endif
}

/** Perform sparc64-specific tasks when an address space is removed from the processor.
 *
 * Demap TSBs.
 *
 * @param as Address space.
 */
void as_deinstall_arch(as_t *as)
{

	/*
	 * Note that we don't lock the address space.
	 * That's correct - we can afford it here
	 * because we only read members that are
	 * currently read-only.
	 */

#ifdef CONFIG_TSB
	if (as != AS_KERNEL) {
		uintptr_t base = ALIGN_DOWN(config.base, 1 << KERNEL_PAGE_WIDTH);

		ASSERT(as->arch.itsb && as->arch.dtsb);

		uintptr_t tsb = as->arch.itsb;
		
		if (!overlaps(tsb, 8*PAGE_SIZE, base, 1 << KERNEL_PAGE_WIDTH)) {
			/*
			 * TSBs were allocated from memory not covered
			 * by the locked 4M kernel DTLB entry. We need
			 * to demap the entry installed by as_install_arch().
			 */
			dtlb_demap(TLB_DEMAP_PAGE, TLB_DEMAP_NUCLEUS, tsb);
		}
		
	}
#endif
}

/** @}
 */
