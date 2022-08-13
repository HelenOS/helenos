/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_mm
 * @{
 */

/**
 * @file
 * @brief RAM disk support.
 *
 * Support for RAM disk images.
 */

#include <assert.h>
#include <log.h>
#include <lib/rd.h>
#include <mm/frame.h>
#include <sysinfo/sysinfo.h>
#include <ddi/ddi.h>

/** Physical memory area for RAM disk. */
static parea_t rd_parea;

/** RAM disk initialization routine
 *
 * The information about the RAM disk is provided as sysinfo
 * values to the uspace tasks.
 *
 */
void init_rd(void *data, size_t size)
{
	uintptr_t base = (uintptr_t) data;
	assert((base % FRAME_SIZE) == 0);

	ddi_parea_init(&rd_parea);
	rd_parea.pbase = base;
	rd_parea.frames = SIZE2FRAMES(size);
	rd_parea.unpriv = false;
	rd_parea.mapped = false;
	ddi_parea_register(&rd_parea);

	sysinfo_set_item_val("rd", NULL, true);
	sysinfo_set_item_val("rd.size", NULL, size);
	sysinfo_set_item_val("rd.address.physical", NULL, (sysarg_t) base);

	log(LF_OTHER, LVL_NOTE, "RAM disk at %p (size %zu bytes)", (void *) base,
	    size);
}

/** @}
 */
