/*
 * Copyright (c) 2005 Martin Decky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <arch/main.h>
#include <arch/arch.h>
#include <arch/asm.h>
#include <genarch/ofw.h>
#include <genarch/ofw_tree.h>
#include <halt.h>
#include <printf.h>
#include <memstr.h>
#include <version.h>
#include <macros.h>
#include <align.h>
#include <str.h>
#include <errno.h>
#include <inflate.h>
#include "../../components.h"

#define BALLOC_MAX_SIZE  131072

static bootinfo_t bootinfo;

static void check_overlap(const char *dest, void *phys, size_t pages)
{
	if ((pages << PAGE_WIDTH) > (uintptr_t) phys) {
		printf("Error: Image (%u pages) overlaps %s at %p, halting.\n",
		    pages, dest, phys);
		halt();
	}
}

void bootstrap(void)
{
	version_print();
	ofw_memmap(&bootinfo.memmap);

	void *bootinfo_pa = ofw_translate(&bootinfo);
	void *real_mode_pa = ofw_translate(&real_mode);
	void *loader_address_pa = ofw_translate((void *) LOADER_ADDRESS);

	printf("\nMemory statistics (total %llu MB)\n", bootinfo.memmap.total >> 20);
	printf(" %p|%p: real mode trampoline\n", &real_mode, real_mode_pa);
	printf(" %p|%p: boot info structure\n", &bootinfo, bootinfo_pa);
	printf(" %p|%p: kernel entry point\n",
	    (void *) PA2KA(BOOT_OFFSET), (void *) BOOT_OFFSET);
	printf(" %p|%p: loader entry point\n",
	    (void *) LOADER_ADDRESS, loader_address_pa);

	size_t i;
	for (i = 0; i < COMPONENTS; i++)
		printf(" %p|%p: %s image (%zu/%zu bytes)\n", components[i].addr,
		    ofw_translate(components[i].addr), components[i].name,
		    components[i].inflated, components[i].size);

	size_t dest[COMPONENTS];
	size_t top = 0;
	size_t cnt = 0;
	bootinfo.taskmap.cnt = 0;
	for (i = 0; i < min(COMPONENTS, TASKMAP_MAX_RECORDS); i++) {
		top = ALIGN_UP(top, PAGE_SIZE);

		if (i > 0) {
			bootinfo.taskmap.tasks[bootinfo.taskmap.cnt].addr =
			    (void *) PA2KA(top);
			bootinfo.taskmap.tasks[bootinfo.taskmap.cnt].size =
			    components[i].inflated;

			str_cpy(bootinfo.taskmap.tasks[bootinfo.taskmap.cnt].name,
			    BOOTINFO_TASK_NAME_BUFLEN, components[i].name);

			bootinfo.taskmap.cnt++;
		}

		dest[i] = top;
		top += components[i].inflated;
		cnt++;
	}

	if (top >= (size_t) loader_address_pa) {
		printf("Inflated components overlap loader area.\n");
		printf("The boot image is too large. Halting.\n");
		halt();
	}

	void *balloc_base;
	void *balloc_base_pa;
	ofw_alloc("boot allocator area", &balloc_base, &balloc_base_pa,
	    BALLOC_MAX_SIZE, loader_address_pa);
	printf(" %p|%p: boot allocator area\n", balloc_base, balloc_base_pa);

	void *inflate_base;
	void *inflate_base_pa;
	ofw_alloc("inflate area", &inflate_base, &inflate_base_pa, top,
	    loader_address_pa);
	printf(" %p|%p: inflate area\n", inflate_base, inflate_base_pa);

	uintptr_t balloc_start = ALIGN_UP(top, PAGE_SIZE);
	size_t pages = (balloc_start + ALIGN_UP(BALLOC_MAX_SIZE, PAGE_SIZE)) >>
	    PAGE_WIDTH;
	void *transtable;
	void *transtable_pa;
	ofw_alloc("translate table", &transtable, &transtable_pa,
	    pages * sizeof(void *), loader_address_pa);
	printf(" %p|%p: translate table\n", transtable, transtable_pa);

	check_overlap("boot allocator area", balloc_base_pa, pages);
	check_overlap("inflate area", inflate_base_pa, pages);
	check_overlap("translate table", transtable_pa, pages);

	printf("\nInflating components ... ");

	for (i = cnt; i > 0; i--) {
		printf("%s ", components[i - 1].name);

		int err = inflate(components[i - 1].addr, components[i - 1].size,
		    inflate_base + dest[i - 1], components[i - 1].inflated);

		if (err != EOK) {
			printf("\n%s: Inflating error %d, halting.\n",
			    components[i - 1].name, err);
			halt();
		}
	}

	printf(".\n");

	printf("Setting up boot allocator ...\n");
	balloc_init(&bootinfo.ballocs, balloc_base, PA2KA(balloc_start),
	    BALLOC_MAX_SIZE);

	printf("Setting up screens ...\n");
	ofw_setup_screens();

	printf("Canonizing OpenFirmware device tree ...\n");
	bootinfo.ofw_root = ofw_tree_build();

	printf("Setting up translate table ...\n");
	for (i = 0; i < pages; i++) {
		uintptr_t off = i << PAGE_WIDTH;
		void *phys;

		if (off < balloc_start)
			phys = ofw_translate(inflate_base + off);
		else
			phys = ofw_translate(balloc_base + off - balloc_start);

		((void **) transtable)[i] = phys;
	}

	printf("Booting the kernel...\n");
	jump_to_kernel(bootinfo_pa, transtable_pa, pages, real_mode_pa);
}
