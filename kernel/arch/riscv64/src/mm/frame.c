/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_riscv64_mm
 * @{
 */

#include <mm/frame.h>
#include <arch/boot/boot.h>
#include <arch/mm/frame.h>
#include <arch/drivers/ucb.h>
#include <mm/as.h>
#include <config.h>
#include <panic.h>
#include <debug.h>
#include <align.h>
#include <macros.h>

uintptr_t physmem_start;
uintptr_t htif_frame;
uintptr_t pt_frame;
memmap_t memmap;

void physmem_print(void)
{
}

static void frame_common_arch_init(bool low)
{
	pfn_t minconf =
	    max3(ADDR2PFN(physmem_start), htif_frame + 1, pt_frame + 1);

	for (size_t i = 0; i < memmap.cnt; i++) {
		/* To be safe, make the available zone possibly smaller */
		uintptr_t base = ALIGN_UP((uintptr_t) memmap.zones[i].start,
		    FRAME_SIZE);
		size_t size = ALIGN_DOWN(memmap.zones[i].size -
		    (base - ((uintptr_t) memmap.zones[i].start)), FRAME_SIZE);

		if (!frame_adjust_zone_bounds(low, &base, &size))
			return;

		pfn_t pfn = ADDR2PFN(base);
		size_t count = SIZE2FRAMES(size);
		pfn_t conf;

		if (low) {
			if ((minconf < pfn) || (minconf >= pfn + count))
				conf = pfn;
			else
				conf = minconf;

			zone_create(pfn, count, conf,
			    ZONE_AVAILABLE | ZONE_LOWMEM);
		} else {
			conf = zone_external_conf_alloc(count);
			if (conf != 0)
				zone_create(pfn, count, conf,
				    ZONE_AVAILABLE | ZONE_HIGHMEM);
		}
	}
}

void frame_low_arch_init(void)
{
	frame_common_arch_init(true);

	frame_mark_unavailable(htif_frame, 1);
	frame_mark_unavailable(pt_frame, 1);
}

void frame_high_arch_init(void)
{
	frame_common_arch_init(false);
}

/** @}
 */
