/*
 * SPDX-FileCopyrightText: 2007 Pavel Jancik, Michal Kebrt
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm32_mm
 * @{
 */
/** @file
 *  @brief Paging related functions.
 */

#include <arch/mm/page.h>
#include <genarch/mm/page_pt.h>
#include <mm/page.h>
#include <arch/mm/frame.h>
#include <align.h>
#include <config.h>
#include <arch/exception.h>
#include <typedefs.h>
#include <interrupt.h>
#include <macros.h>

/** Initializes page tables.
 *
 * 1:1 virtual-physical mapping is created in kernel address space. Mapping
 * for table with exception vectors is also created.
 */
void page_arch_init(void)
{
	int flags = PAGE_CACHEABLE | PAGE_EXEC;
	page_mapping_operations = &pt_mapping_operations;

	page_table_lock(AS_KERNEL, true);

	/* Kernel identity mapping */
	//FIXME: We need to consider the possibility that
	//identity_base > identity_size and physmem_end.
	//This might lead to overflow if identity_size is too big.
	for (uintptr_t cur = PHYSMEM_START_ADDR;
	    cur < min(KA2PA(config.identity_base) +
	    config.identity_size, config.physmem_end);
	    cur += FRAME_SIZE)
		page_mapping_insert(AS_KERNEL, PA2KA(cur), cur, flags);

#ifdef HIGH_EXCEPTION_VECTORS
	/* Create mapping for exception table at high offset */
	uintptr_t ev_frame = frame_alloc(1, FRAME_HIGHMEM, 0);
	page_mapping_insert(AS_KERNEL, EXC_BASE_ADDRESS, ev_frame, flags);
#else
#error "Only high exception vector supported now"
#endif

	page_table_unlock(AS_KERNEL, true);

	as_switch(NULL, AS_KERNEL);

	boot_page_table_free();
}

/** @}
 */
