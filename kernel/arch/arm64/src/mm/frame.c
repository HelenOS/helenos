/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm64_mm
 * @{
 */
/** @file
 * @brief Frame related functions.
 */

#include <arch/mm/frame.h>
#include <mm/frame.h>
#include <config.h>
#include <align.h>
#include <macros.h>

/** Physical memory map received from the bootcode. */
memmap_t memmap;

/** Print memory layout. */
void physmem_print(void)
{
	printf("[base            ] [size            ] [type      ]\n");

	size_t i;
	for (i = 0; i < memmap.cnt; i++) {
		const char *type;
		switch (memmap.zones[i].type) {
		case MEMTYPE_AVAILABLE:
			type = "available";
			break;
		case MEMTYPE_ACPI_RECLAIM:
			type = "ACPI reclaim";
			break;
		default:
			type = "unusable";
			break;
		}

		printf("%p %#018zx %s\n", memmap.zones[i].start,
		    memmap.zones[i].size, type);
	}
}

/** Create memory zones according to information stored in memmap.
 *
 * Walk the memory map and create frame zones according to it.
 */
static void frame_common_arch_init(bool low)
{
	size_t i;

	for (i = 0; i < memmap.cnt; i++) {
		if (memmap.zones[i].type != MEMTYPE_AVAILABLE)
			continue;

		/* To be safe, make the available zone possibly smaller. */
		uintptr_t base = ALIGN_UP((uintptr_t) memmap.zones[i].start,
		    FRAME_SIZE);
		size_t size = ALIGN_DOWN(memmap.zones[i].size -
		    (base - (uintptr_t) memmap.zones[i].start), FRAME_SIZE);

		if (!frame_adjust_zone_bounds(low, &base, &size))
			continue;

		pfn_t confdata;
		pfn_t pfn = ADDR2PFN(base);
		size_t count = SIZE2FRAMES(size);

		if (low) {
			/* Determine where to place confdata. */
			if (pfn == 0) {
				/*
				 * Avoid placing confdata at the NULL address.
				 */
				if (count == 1)
					continue;
				confdata = 1;
			} else
				confdata = pfn;

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

/** Create low memory zones. */
void frame_low_arch_init(void)
{
	if (config.cpu_active > 1)
		return;

	frame_common_arch_init(true);

	/*
	 * On ARM64, physical memory can start on a non-zero address. The
	 * generic frame_init() only marks PFN 0 as not free, so we must mark
	 * the physically first frame not free explicitly here, no matter what
	 * is its address.
	 */
	frame_mark_unavailable(ADDR2PFN(physmem_base), 1);
}

/** Create high memory zones. */
void frame_high_arch_init(void)
{
	if (config.cpu_active > 1)
		return;

	frame_common_arch_init(false);
}

/** @}
 */
