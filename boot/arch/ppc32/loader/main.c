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

#include <printf.h>
#include <ofw.h>
#include <align.h>
#include <macros.h>
#include <string.h>
#include "main.h"
#include "asm.h"
#include "_components.h"

static bootinfo_t bootinfo;
static component_t components[COMPONENTS];
static char *release = STRING(RELEASE);

#ifdef REVISION
	static char *revision = ", revision " STRING(REVISION);
#else
	static char *revision = "";
#endif

#ifdef TIMESTAMP
	static char *timestamp = "\nBuilt on " STRING(TIMESTAMP);
#else
	static char *timestamp = "";
#endif

/** Print version information. */
static void version_print(void)
{
	printf("HelenOS PPC32 Bootloader\nRelease %s%s%s\n"
	    "Copyright (c) 2006 HelenOS project\n\n",
	    release, revision, timestamp);
}

static void check_align(const void *addr, const char *desc)
{
	if ((uintptr_t) addr % PAGE_SIZE != 0) {
		printf("Error: %s not on page boundary, halting.\n", desc);
		halt();
	}
}

static void check_overlap(const void *pa, const char *desc, const uintptr_t top)
{
	if ((uintptr_t) pa + PAGE_SIZE < top) {
		printf("Error: %s overlaps destination physical area\n", desc);
		halt();
	}
}

void bootstrap(void)
{
	version_print();
	init_components(components);
	
	if (!ofw_memmap(&bootinfo.memmap)) {
		printf("Error: Unable to get memory map, halting.\n");
		halt();
	}
	
	if (bootinfo.memmap.total == 0) {
		printf("Error: No memory detected, halting.\n");
		halt();
	}
	
	check_align(&real_mode, "bootstrap trampoline");
	check_align(trans, "translation table");
	check_align(balloc_base, "boot allocations");
	
	unsigned int i;
	for (i = 0; i < COMPONENTS; i++)
		check_align(components[i].start, components[i].name);
	
	void *bootinfo_pa = ofw_translate(&bootinfo);
	void *real_mode_pa = ofw_translate(&real_mode);
	void *trans_pa = ofw_translate(trans);
	void *balloc_base_pa = ofw_translate(balloc_base);
	
	printf("Memory statistics (total %d MB)\n", bootinfo.memmap.total >> 20);
	printf(" %L: boot info structure (physical %L)\n", &bootinfo, bootinfo_pa);
	printf(" %L: bootstrap trampoline (physical %L)\n", &real_mode, real_mode_pa);
	printf(" %L: translation table (physical %L)\n", trans, trans_pa);
	printf(" %L: boot allocations (physical %L)\n", balloc_base, balloc_base_pa);
	for (i = 0; i < COMPONENTS; i++)
		printf(" %L: %s image (size %d bytes)\n", components[i].start, components[i].name, components[i].size);
	
	uintptr_t top = 0;
	for (i = 0; i < COMPONENTS; i++)
		top += ALIGN_UP(components[i].size, PAGE_SIZE);
	top += ALIGN_UP(BALLOC_MAX_SIZE, PAGE_SIZE);
	
	if (top >= TRANS_SIZE * PAGE_SIZE) {
		printf("Error: boot image is too large\n");
		halt();
	}
	
	check_overlap(bootinfo_pa, "boot info", top);
	check_overlap(real_mode_pa, "bootstrap trampoline", top);
	check_overlap(trans_pa, "translation table", top);
	
	unsigned int pages = ALIGN_UP(KERNEL_SIZE, PAGE_SIZE) >> PAGE_WIDTH;
	
	for (i = 0; i < pages; i++) {
		void *pa = ofw_translate(KERNEL_START + (i << PAGE_WIDTH));
		check_overlap(pa, "kernel", top);
		trans[i] = (uintptr_t) pa;
	}
	
	bootinfo.taskmap.count = 0;
	for (i = 1; i < COMPONENTS; i++) {
		if (bootinfo.taskmap.count == TASKMAP_MAX_RECORDS) {
			printf("\nSkipping superfluous components.\n");
			break;
		}
		
		unsigned int component_pages = ALIGN_UP(components[i].size, PAGE_SIZE) >> PAGE_WIDTH;
		unsigned int j;
		
		for (j = 0; j < component_pages; j++) {
			void *pa = ofw_translate(components[i].start + (j << PAGE_WIDTH));
			check_overlap(pa, components[i].name, top);
			trans[pages + j] = (uintptr_t) pa;
			if (j == 0) {
				
				bootinfo.taskmap.tasks[bootinfo.taskmap.count].addr = (void *) PA2KA(pages << PAGE_WIDTH);
				bootinfo.taskmap.tasks[bootinfo.taskmap.count].size = components[i].size;
				strncpy(bootinfo.taskmap.tasks[bootinfo.taskmap.count].name,
				    components[i].name, BOOTINFO_TASK_NAME_BUFLEN);
				
				bootinfo.taskmap.count++;
			}
		}
		
		pages += component_pages;
	}
	
	uintptr_t balloc_kernel_base = PA2KA(pages << PAGE_WIDTH);
	unsigned int balloc_pages = ALIGN_UP(BALLOC_MAX_SIZE, PAGE_SIZE) >> PAGE_WIDTH;
	for (i = 0; i < balloc_pages; i++) {
		void *pa = ofw_translate(balloc_base + (i << PAGE_WIDTH));
		check_overlap(pa, "boot allocations", top);
		trans[pages + i] = (uintptr_t) pa;
	}
	
	pages += balloc_pages;
	
	balloc_init(&bootinfo.ballocs, (uintptr_t) balloc_base, balloc_kernel_base);
	printf("\nCanonizing OpenFirmware device tree...");
	bootinfo.ofw_root = ofw_tree_build();
	printf("done.\n");
	
	ofw_setup_palette();
	
	printf("\nBooting the kernel...\n");
	jump_to_kernel(bootinfo_pa, sizeof(bootinfo), trans_pa, pages << PAGE_WIDTH, real_mode_pa);
}
