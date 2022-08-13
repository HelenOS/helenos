/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 * SPDX-FileCopyrightText: 2009 Pavel Rimsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_mm
 * @{
 */
/** @file
 */

#include <arch/mm/as.h>
#include <arch/mm/pagesize.h>
#include <arch/mm/tlb.h>
#include <assert.h>
#include <genarch/mm/page_ht.h>
#include <genarch/mm/asid_fifo.h>
#include <config.h>
#include <arch/sun4v/hypercall.h>

#ifdef CONFIG_TSB

#include <arch/mm/tsb.h>
#include <arch/asm.h>
#include <mm/frame.h>
#include <bitops.h>
#include <macros.h>
#include <mem.h>

#endif /* CONFIG_TSB */

/** Architecture dependent address space init. */
void as_arch_init(void)
{
	if (config.cpu_active == 1) {
		as_operations = &as_ht_operations;
		asid_fifo_init();
	}
}

errno_t as_constructor_arch(as_t *as, unsigned int flags)
{
#ifdef CONFIG_TSB
	uintptr_t tsb_base = frame_alloc(TSB_FRAMES, FRAME_LOWMEM | flags,
	    TSB_SIZE - 1);
	if (!tsb_base)
		return ENOMEM;

	tsb_entry_t *tsb = (tsb_entry_t *) PA2KA(tsb_base);

	as->arch.tsb_description.page_size = PAGESIZE_8K;
	as->arch.tsb_description.associativity = 1;
	as->arch.tsb_description.num_ttes = TSB_ENTRY_COUNT;
	as->arch.tsb_description.pgsize_mask = 1 << PAGESIZE_8K;
	as->arch.tsb_description.tsb_base = tsb_base;
	as->arch.tsb_description.reserved = 0;
	as->arch.tsb_description.context = 0;

	memsetb(tsb, TSB_SIZE, 0);
#endif

	return EOK;
}

int as_destructor_arch(as_t *as)
{
#ifdef CONFIG_TSB
	frame_free(as->arch.tsb_description.tsb_base, TSB_FRAMES);

	return TSB_FRAMES;
#else
	return 0;
#endif
}

errno_t as_create_arch(as_t *as, unsigned int flags)
{
#ifdef CONFIG_TSB
	tsb_invalidate(as, 0, (size_t) -1);
#endif

	return EOK;
}

/** Perform sparc64-specific tasks when an address space becomes active on the
 * processor.
 *
 * Install ASID and map TSBs.
 *
 * @param as Address space.
 *
 */
void as_install_arch(as_t *as)
{
	mmu_secondary_context_write(as->asid);

#ifdef CONFIG_TSB
	uintptr_t base = ALIGN_DOWN(config.base, 1 << KERNEL_PAGE_WIDTH);

	assert(as->arch.tsb_description.tsb_base);
	uintptr_t tsb = PA2KA(as->arch.tsb_description.tsb_base);

	if (!overlaps(tsb, TSB_SIZE, base, 1 << KERNEL_PAGE_WIDTH)) {
		/*
		 * TSBs were allocated from memory not covered
		 * by the locked 4M kernel DTLB entry. We need
		 * to map both TSBs explicitly.
		 *
		 */
		mmu_demap_page(tsb, 0, MMU_FLAG_DTLB);
		dtlb_insert_mapping(tsb, KA2PA(tsb), PAGESIZE_64K, true, true);
	}

	__hypercall_fast2(MMU_TSB_CTXNON0, 1, KA2PA(&as->arch.tsb_description));
#endif
}

/** Perform sparc64-specific tasks when an address space is removed from the
 * processor.
 *
 * Demap TSBs.
 *
 * @param as Address space.
 *
 */
void as_deinstall_arch(as_t *as)
{
	/*
	 * Note that we don't and may not lock the address space. That's ok
	 * since we only read members that are currently read-only.
	 *
	 * Moreover, the as->asid is protected by asidlock, which is being held.
	 *
	 */

#ifdef CONFIG_TSB
	uintptr_t base = ALIGN_DOWN(config.base, 1 << KERNEL_PAGE_WIDTH);

	assert(as->arch.tsb_description.tsb_base);

	uintptr_t tsb = PA2KA(as->arch.tsb_description.tsb_base);

	if (!overlaps(tsb, TSB_SIZE, base, 1 << KERNEL_PAGE_WIDTH)) {
		/*
		 * TSBs were allocated from memory not covered
		 * by the locked 4M kernel DTLB entry. We need
		 * to demap the entry installed by as_install_arch().
		 *
		 */
		__hypercall_fast3(MMU_UNMAP_PERM_ADDR, tsb, 0, MMU_FLAG_DTLB);
	}
#endif
}

/** @}
 */
