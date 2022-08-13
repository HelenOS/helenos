/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ppc32_mm
 * @{
 */
/** @file
 */

#include <arch/boot/boot.h>
#include <arch/mm/frame.h>
#include <mm/frame.h>
#include <align.h>
#include <macros.h>
#include <stdio.h>

memmap_t memmap;

void physmem_print(void)
{
	printf("[base    ] [size    ]\n");

	size_t i;
	for (i = 0; i < memmap.cnt; i++) {
		printf("%p %#0zx\n", memmap.zones[i].start,
		    memmap.zones[i].size);
	}
}

static void frame_common_arch_init(bool low)
{
	pfn_t minconf = 2;
	size_t i;

	for (i = 0; i < memmap.cnt; i++) {
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

	/*
	 * First is exception vector, second is 'implementation specific',
	 * third and fourth is reserved, other contain real mode code
	 */
	frame_mark_unavailable(0, 8);

	/* Mark the Page Hash Table frames as unavailable */
	uint32_t sdr1 = sdr1_get();

	// FIXME: compute size of PHT exactly
	frame_mark_unavailable(ADDR2PFN(sdr1 & 0xffff000), 16);
}

void frame_high_arch_init(void)
{
	frame_common_arch_init(false);
}

/** @}
 */
