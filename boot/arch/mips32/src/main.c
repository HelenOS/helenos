/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch/main.h>
#include <arch/arch.h>
#include <arch/asm.h>
#include <arch/types.h>
#include <halt.h>
#include <printf.h>
#include <memstr.h>
#include <version.h>
#include <macros.h>
#include <align.h>
#include <str.h>
#include <errno.h>
#include <payload.h>
#include <kernel.h>

static bootinfo_t *bootinfo = (bootinfo_t *) PA2KA(BOOTINFO_OFFSET);
static uint32_t *cpumap = (uint32_t *) PA2KA(CPUMAP_OFFSET);

void bootstrap(int kargc, char **kargv)
{
	version_print();

	printf("\nMemory statistics\n");
	printf(" %p|%p: CPU map\n", (void *) PA2KA(CPUMAP_OFFSET),
	    (void *) CPUMAP_OFFSET);
	printf(" %p|%p: bootstrap stack\n", (void *) PA2KA(STACK_OFFSET),
	    (void *) STACK_OFFSET);
	printf(" %p|%p: boot info structure\n",
	    (void *) PA2KA(BOOTINFO_OFFSET), (void *) BOOTINFO_OFFSET);
	printf(" %p|%p: kernel entry point\n", (void *) PA2KA(BOOT_OFFSET),
	    (void *) BOOT_OFFSET);
	printf(" %p|%p: bootloader entry point\n",
	    (void *) PA2KA(LOADER_OFFSET), (void *) LOADER_OFFSET);

	uint8_t *kernel_start = (uint8_t *) PA2KA(BOOT_OFFSET);
	// FIXME: Use the correct value.
	uint8_t *ram_end = kernel_start + (1 << 24);

	// TODO: Make sure that the I-cache, D-cache and memory are coherent.
	//       (i.e. provide the clear_cache callback)

	extract_payload(&bootinfo->taskmap, kernel_start, ram_end,
	    (uintptr_t) kernel_start, NULL);

	printf("Copying CPU map ... \n");

	bootinfo->cpumap = 0;
	for (int i = 0; i < CPUMAP_MAX_RECORDS; i++) {
		if (cpumap[i] != 0)
			bootinfo->cpumap |= (1 << i);
	}

	str_cpy(bootinfo->bootargs, sizeof(bootinfo->bootargs),
	    (kargc > 1) ? kargv[1] : "");

	uintptr_t entry = check_kernel(kernel_start);

	printf("Booting the kernel...\n");
	jump_to_kernel((void *) entry, bootinfo);
}
