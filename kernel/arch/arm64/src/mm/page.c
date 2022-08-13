/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm64_mm
 * @{
 */
/** @file
 * @brief Paging related functions.
 */

#include <arch/mm/page.h>
#include <genarch/mm/page_pt.h>
#include <mm/as.h>
#include <mm/page.h>

/** Initializes page tables. */
void page_arch_init(void)
{
	if (config.cpu_active > 1) {
		as_switch(NULL, AS_KERNEL);
		return;
	}

	page_mapping_operations = &pt_mapping_operations;

	page_table_lock(AS_KERNEL, true);

	/* PA2KA(identity) mapping for all low-memory frames. */
	for (uintptr_t cur = 0; cur < config.identity_size; cur += FRAME_SIZE) {
		uintptr_t addr = physmem_base + cur;
		page_mapping_insert(AS_KERNEL, PA2KA(addr), addr,
		    PAGE_GLOBAL | PAGE_CACHEABLE | PAGE_EXEC | PAGE_WRITE |
		    PAGE_READ);
	}

	page_table_unlock(AS_KERNEL, true);

	as_switch(NULL, AS_KERNEL);
}

/** @}
 */
