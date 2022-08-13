/*
 * SPDX-FileCopyrightText: 2016 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia32
 * @{
 */
/** @file
 */

#include <arch/vreg.h>
#include <arch/pm.h>
#include <config.h>
#include <typedefs.h>
#include <arch/asm.h>
#include <panic.h>
#include <mm/km.h>
#include <mm/frame.h>

/*
 * During initialization, we need to make sure that context_save() and
 * context_restore() touch some meaningful address when saving/restoring VREGs.
 * When a processor is initialized, we set its GS base to a private page and
 * vreg_ptr to zero.
 */
static uint32_t vreg_tp_dummy;
uint32_t *vreg_ptr = &vreg_tp_dummy;

/**
 * Allocate and initialize a per-CPU user page to be accessible via the GS
 * segment register and to hold the virtual registers.
 */
void vreg_init(void)
{
	uintptr_t frame;
	uint32_t *page;
	descriptor_t *gdt_p = (descriptor_t *) gdtr.base;

	frame = frame_alloc(1, FRAME_ATOMIC | FRAME_HIGHMEM, 0);
	if (!frame)
		panic("Cannot allocate VREG frame.");

	page = (uint32_t *) km_map(frame, PAGE_SIZE, PAGE_SIZE,
	    PAGE_READ | PAGE_WRITE | PAGE_USER | PAGE_CACHEABLE);

	gdt_setbase(&gdt_p[VREG_DES], (uintptr_t) page);
	gdt_setlimit(&gdt_p[VREG_DES], sizeof(uint32_t));

	gs_load(GDT_SELECTOR(VREG_DES));

	vreg_ptr = NULL;
}

/** @}
 */
