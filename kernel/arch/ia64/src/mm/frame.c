/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia64_mm
 * @{
 */
/** @file
 */

#include <arch/mm/frame.h>
#include <mm/frame.h>
#include <config.h>
#include <panic.h>
#include <arch/bootinfo.h>
#include <align.h>
#include <macros.h>

#define KERNEL_RESERVED_AREA_BASE 	(0x4400000)
#define KERNEL_RESERVED_AREA_SIZE 	(16 * 1024 * 1024)

#define ROM_BASE	0xa0000               /* for simulators */
#define ROM_SIZE	(384 * 1024)          /* for simulators */

#define MIN_ZONE_SIZE	(64 * 1024)

#define MINCONF 1

uintptr_t end_of_identity = -1ULL;

static void frame_common_arch_init(bool low)
{
	unsigned int i;

	for (i = 0; i < bootinfo->memmap_items; i++) {
		if (bootinfo->memmap[i].type != MEMMAP_FREE_MEM)
			continue;

		uintptr_t base = bootinfo->memmap[i].base;
		size_t size = bootinfo->memmap[i].size;
		uintptr_t abase = ALIGN_UP(base, FRAME_SIZE);

		if (size > FRAME_SIZE)
			size -= abase - base;

		if (!frame_adjust_zone_bounds(low, &abase, &size))
			continue;

		if (size > MIN_ZONE_SIZE) {
			pfn_t pfn = ADDR2PFN(abase);
			size_t count = SIZE2FRAMES(size);

			if (low) {
				zone_create(pfn, count, max(MINCONF, pfn),
				    ZONE_AVAILABLE | ZONE_LOWMEM);
			} else {
				pfn_t conf = zone_external_conf_alloc(count);
				if (conf != 0)
					zone_create(pfn, count, conf,
					    ZONE_AVAILABLE | ZONE_HIGHMEM);
			}
		}
	}
}

void frame_low_arch_init(void)
{
	if (config.cpu_active > 1)
		return;

	frame_common_arch_init(true);

	/*
	 * Blacklist ROM regions.
	 */
	frame_mark_unavailable(ADDR2PFN(ROM_BASE),
	    SIZE2FRAMES(ROM_SIZE));

	frame_mark_unavailable(ADDR2PFN(KERNEL_RESERVED_AREA_BASE),
	    SIZE2FRAMES(KERNEL_RESERVED_AREA_SIZE));

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
