/*
 * SPDX-FileCopyrightText: 2016 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_amd64
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
 * When a processor is initialized, we set its FS base to a private page and
 * vreg_ptr to zero.
 */
static uint64_t vreg_tp_dummy;
uint64_t *vreg_ptr = &vreg_tp_dummy;

/**
 * Allocate and initialize a per-CPU user page to be accessible via the FS
 * segment register and to hold the virtual registers.
 */
void vreg_init(void)
{
	uintptr_t frame;
	uint64_t *page;

	frame = frame_alloc(1, FRAME_ATOMIC | FRAME_HIGHMEM, 0);
	if (!frame)
		panic("Cannot allocate VREG frame.");

	page = (uint64_t *) km_map(frame, PAGE_SIZE, PAGE_SIZE,
	    PAGE_READ | PAGE_WRITE | PAGE_USER | PAGE_CACHEABLE);

	write_msr(AMD_MSR_FS, (uintptr_t) page);

	vreg_ptr = NULL;
}

/** @}
 */
