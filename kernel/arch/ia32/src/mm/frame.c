/*
 * SPDX-FileCopyrightText: 2008 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia32_mm
 * @{
 */
/** @file
 * @ingroup kernel_ia32_mm, kernel_amd64_mm
 */

#include <mm/frame.h>
#include <arch/mm/frame.h>
#include <mm/as.h>
#include <config.h>
#include <arch/boot/boot.h>
#include <arch/boot/memmap.h>
#include <panic.h>
#include <debug.h>
#include <align.h>
#include <macros.h>
#include <stdio.h>

#define PHYSMEM_LIMIT32  UINT64_C(0x100000000)

static void init_e820_memory(pfn_t minconf, bool low)
{
	unsigned int i;

	for (i = 0; i < e820counter; i++) {
		uint64_t base64 = e820table[i].base_address;
		uint64_t size64 = e820table[i].size;

#ifdef KARCH_ia32
		/*
		 * Restrict the e820 table entries to 32-bits.
		 */
		if (base64 >= PHYSMEM_LIMIT32)
			continue;

		if (base64 + size64 > PHYSMEM_LIMIT32)
			size64 = PHYSMEM_LIMIT32 - base64;
#endif

		uintptr_t base = (uintptr_t) base64;
		size_t size = (size_t) size64;

		if (!frame_adjust_zone_bounds(low, &base, &size))
			continue;

		if (e820table[i].type == MEMMAP_MEMORY_AVAILABLE) {
			/* To be safe, make the available zone possibly smaller */
			uint64_t new_base = ALIGN_UP(base, FRAME_SIZE);
			uint64_t new_size = ALIGN_DOWN(size - (new_base - base),
			    FRAME_SIZE);

			size_t count = SIZE2FRAMES(new_size);
			pfn_t pfn = ADDR2PFN(new_base);
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
		} else if ((e820table[i].type == MEMMAP_MEMORY_ACPI) ||
		    (e820table[i].type == MEMMAP_MEMORY_NVS)) {
			/* To be safe, make the firmware zone possibly larger */
			uint64_t new_base = ALIGN_DOWN(base, FRAME_SIZE);
			uint64_t new_size = ALIGN_UP(size + (base - new_base),
			    FRAME_SIZE);

			zone_create(ADDR2PFN(new_base), SIZE2FRAMES(new_size), 0,
			    ZONE_FIRMWARE);
		} else {
			/* To be safe, make the reserved zone possibly larger */
			uint64_t new_base = ALIGN_DOWN(base, FRAME_SIZE);
			uint64_t new_size = ALIGN_UP(size + (base - new_base),
			    FRAME_SIZE);

			zone_create(ADDR2PFN(new_base), SIZE2FRAMES(new_size), 0,
			    ZONE_RESERVED);
		}
	}
}

static const char *e820names[] = {
	"invalid",
	"available",
	"reserved",
	"acpi",
	"nvs",
	"unusable"
};

void physmem_print(void)
{
	unsigned int i;
	printf("[base            ] [size            ] [name   ]\n");

	for (i = 0; i < e820counter; i++) {
		const char *name;

		if (e820table[i].type <= MEMMAP_MEMORY_UNUSABLE)
			name = e820names[e820table[i].type];
		else
			name = "invalid";

		printf("%#018" PRIx64 " %#018" PRIx64 " %s\n", e820table[i].base_address,
		    e820table[i].size, name);
	}
}

void frame_low_arch_init(void)
{
	pfn_t minconf;

	if (config.cpu_active == 1) {
		minconf = 1;

#ifdef CONFIG_SMP
		size_t unmapped_size =
		    (uintptr_t) unmapped_end - BOOT_OFFSET;

		minconf = max(minconf,
		    ADDR2PFN(AP_BOOT_OFFSET + unmapped_size));
#endif

		init_e820_memory(minconf, true);

		/* Reserve frame 0 (BIOS data) */
		frame_mark_unavailable(0, 1);

#ifdef CONFIG_SMP
		/* Reserve AP real mode bootstrap memory */
		frame_mark_unavailable(AP_BOOT_OFFSET >> FRAME_WIDTH,
		    unmapped_size >> FRAME_WIDTH);
#endif
	}
}

void frame_high_arch_init(void)
{
	if (config.cpu_active == 1)
		init_e820_memory(0, false);
}

/** @}
 */
