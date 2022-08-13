/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_mm
 * @{
 */
/** @file
 */

#include <arch/mm/frame.h>
#include <mm/frame.h>
#include <arch/boot/boot.h>
#include <typedefs.h>
#include <config.h>
#include <align.h>
#include <macros.h>

/** Create memory zones according to information stored in memmap.
 *
 * Walk the memory map and create frame zones according to it.
 */
static void frame_common_arch_init(bool low)
{
	unsigned int i;

	for (i = 0; i < memmap.cnt; i++) {
		uintptr_t base;
		size_t size;

		/*
		 * The memmap is created by HelenOS boot loader.
		 * It already contains no holes.
		 */

		/* To be safe, make the available zone possibly smaller */
		base = ALIGN_UP((uintptr_t) memmap.zones[i].start, FRAME_SIZE);
		size = ALIGN_DOWN(memmap.zones[i].size -
		    (base - ((uintptr_t) memmap.zones[i].start)), FRAME_SIZE);

		if (!frame_adjust_zone_bounds(low, &base, &size))
			continue;

		pfn_t confdata;
		pfn_t pfn = ADDR2PFN(base);
		size_t count = SIZE2FRAMES(size);

		if (low) {
			confdata = pfn;
			if (confdata == ADDR2PFN(KA2PA(PFN2ADDR(0))))
				confdata = ADDR2PFN(KA2PA(PFN2ADDR(2)));

			zone_create(pfn, count, confdata,
			    ZONE_AVAILABLE | ZONE_LOWMEM);
		} else {
			confdata = zone_external_conf_alloc(count);
			if (confdata != 0)
				zone_create(pfn, count, confdata,
				    ZONE_AVAILABLE | ZONE_HIGHMEM);
		}
	}
}

void frame_low_arch_init(void)
{
	if (config.cpu_active > 1)
		return;

	frame_common_arch_init(true);

	/*
	 * On sparc64, physical memory can start on a non-zero address.
	 * The generic frame_init() only marks PFN 0 as not free, so we
	 * must mark the physically first frame not free explicitly
	 * here, no matter what is its address.
	 */
	frame_mark_unavailable(ADDR2PFN(KA2PA(PFN2ADDR(0))), 1);

	/* PA2KA will work only on low-memory. */
	end_of_identity = PA2KA(config.physmem_end - FRAME_SIZE) + PAGE_SIZE;
}

void frame_high_arch_init(void)
{
	if (config.cpu_active > 1)
		return;

	frame_common_arch_init(false);
}

/** @}
 */
