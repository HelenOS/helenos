/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch/main.h>
#include <arch/arch.h>
#include <arch/asm.h>
#include <arch/ucb.h>
#include <arch/mm.h>
#include <arch/types.h>
#include <version.h>
#include <stddef.h>
#include <printf.h>
#include <macros.h>
#include <errno.h>
#include <align.h>
#include <str.h>
#include <halt.h>
#include <payload.h>
#include <kernel.h>

static bootinfo_t bootinfo;

void bootstrap(void)
{
	version_print();

	bootinfo.htif_frame = ((uintptr_t) &htif_page) >> PAGE_WIDTH;
	bootinfo.pt_frame = ((uintptr_t) &pt_page) >> PAGE_WIDTH;

	bootinfo.ucbinfo.tohost =
	    (volatile uint64_t *) PA2KA((uintptr_t) &tohost);
	bootinfo.ucbinfo.fromhost =
	    (volatile uint64_t *) PA2KA((uintptr_t) &fromhost);

	// FIXME TODO: read from device tree
	bootinfo.physmem_start = PHYSMEM_START;
	bootinfo.memmap.total = PHYSMEM_SIZE;
	bootinfo.memmap.cnt = 1;
	bootinfo.memmap.zones[0].start = (void *) PHYSMEM_START;
	bootinfo.memmap.zones[0].size = PHYSMEM_SIZE;

	printf("\nMemory statistics (total %" PRIu64 " MB, starting at %p)\n\n",
	    bootinfo.memmap.total >> 20, (void *) bootinfo.physmem_start);
	printf(" %p: boot info structure\n", &bootinfo);

	uint8_t *load_addr = (uint8_t *) BOOT_OFFSET;
	uintptr_t kernel_addr = PA2KA(load_addr);

	printf(" %p: inflate area\n", load_addr);
	printf(" %p: kernel entry point\n", (void *) kernel_addr);

	/* Find the end of the memory zone containing the load address. */
	uint8_t *end = NULL;
	for (size_t i = 0; i < bootinfo.memmap.cnt; i++) {
		memzone_t z = bootinfo.memmap.zones[i];

		if (z.start <= (void *) load_addr &&
		    z.start + z.size > (void *) load_addr) {
			end = z.start + z.size;
		}
	}

	// TODO: Cache-coherence callback?
	extract_payload(&bootinfo.taskmap, load_addr, end, kernel_addr, NULL);

	uintptr_t entry = check_kernel(load_addr);

	printf("Booting the kernel...\n");
	jump_to_kernel(PA2KA(&bootinfo), entry);
}
