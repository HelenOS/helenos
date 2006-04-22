/*
 * Copyright (C) 2005 Martin Decky
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

#include "main.h" 
#include "printf.h"
#include "asm.h"

#define KERNEL_START ((void *) &_binary_____________kernel_kernel_bin_start)
#define KERNEL_END ((void *) &_binary_____________kernel_kernel_bin_end)
#define KERNEL_SIZE ((unsigned int) KERNEL_END - (unsigned int) KERNEL_START)

#define INIT_START ((void *) &_binary_____________uspace_init_init_start)
#define INIT_END ((void *) &_binary_____________uspace_init_init_end)
#define INIT_SIZE ((unsigned int) INIT_END - (unsigned int) INIT_START)

#define HEAP_GAP 1024000

bootinfo_t bootinfo;


static void check_align(const void *addr, const char *desc)
{
	if ((unsigned int) addr % PAGE_SIZE != 0) {
		printf("Error: %s not on page boundary, halting.\n", desc);
		halt();
	}
}


static void fix_overlap(void *va, void **pa, const char *desc, unsigned int *top)
{
	if ((unsigned int) *pa + PAGE_SIZE < *top) {
		printf("Warning: %s overlaps kernel physical area\n", desc);
		
		void *new_va = (void *) (ALIGN_UP((unsigned int) KERNEL_END + HEAP_GAP, PAGE_SIZE) + *top);
		void *new_pa = (void *) (HEAP_GAP + *top);
		*top += PAGE_SIZE;
		
		if (ofw_map(new_pa, new_va, PAGE_SIZE, 0) != 0) {
			printf("Error: Unable to map page aligned memory at %L (physical %L), halting.\n", new_va, new_pa);
			halt();
		}
		
		if ((unsigned int) new_pa + PAGE_SIZE < KERNEL_SIZE) {
			printf("Error: %s cannot be relocated, halting.\n", desc);
			halt();	
		}
		
		printf("Relocating %L -> %L (physical %L -> %L)\n", va, new_va, *pa, new_pa);
		*pa = new_pa;
		memcpy(new_va, va, PAGE_SIZE);
	}
}


void bootstrap(void)
{
	printf("\nHelenOS PPC Bootloader\n");
	
	check_align(KERNEL_START, "Kernel image");
	check_align(INIT_START, "Init image");
	check_align(&real_mode, "Bootstrap trampoline");
	check_align(&trans, "Translation table");
	
	if (!ofw_memmap(&bootinfo.memmap)) {
		printf("Error: Unable to get memory map, halting.\n");
		halt();
	}
	
	if (bootinfo.memmap.total == 0) {
		printf("Error: No memory detected, halting.\n");
		halt();
	}
	
	if (!ofw_screen(&bootinfo.screen)) {
		printf("Error: Unable to get screen properties, halting.\n");
		halt();
	}
	
	printf("\nDevice statistics\n");
	printf(" screen at %L, resolution %dx%d, %d bpp (scanline %d bytes)\n", bootinfo.screen.addr, bootinfo.screen.width, bootinfo.screen.height, bootinfo.screen.bpp, bootinfo.screen.scanline);
	
	void *real_mode_pa = ofw_translate(&real_mode);
	void *trans_pa = ofw_translate(&trans);
	void *bootinfo_pa = ofw_translate(&bootinfo);
	void *fb = (void *) (((unsigned int) bootinfo.screen.addr) & ((unsigned int) ~0 << 17));
	
	printf("\nMemory statistics (total %d MB)\n", bootinfo.memmap.total >> 20);
	printf(" kernel image         at %L (size %d bytes)\n", KERNEL_START, KERNEL_SIZE);
	printf(" init image           at %L (size %d bytes)\n", INIT_START, INIT_SIZE);
	printf(" boot info structure  at %L (physical %L)\n", &bootinfo, bootinfo_pa);
	printf(" bootstrap trampoline at %L (physical %L)\n", &real_mode, real_mode_pa);
	printf(" translation table    at %L (physical %L)\n", &trans, trans_pa);
	
	unsigned int top = ALIGN_UP(KERNEL_SIZE, PAGE_SIZE) + ALIGN_UP(INIT_SIZE, PAGE_SIZE);
	unsigned int kernel_pages = ALIGN_UP(KERNEL_SIZE, PAGE_SIZE) >> PAGE_WIDTH;
	unsigned int init_pages = ALIGN_UP(INIT_SIZE, PAGE_SIZE) >> PAGE_WIDTH;
	
	unsigned int i;
	
	for (i = 0; i < kernel_pages; i++) {
		void *pa = ofw_translate(KERNEL_START + (i << PAGE_WIDTH));
		fix_overlap(KERNEL_START + (i << PAGE_WIDTH), &pa, "Kernel image", &top);
		trans[i] = pa;
	}
	
	for (i = 0; i < init_pages; i++) {
		void *pa = ofw_translate(INIT_START + (i << PAGE_WIDTH));
		fix_overlap(INIT_START + (i << PAGE_WIDTH), &pa, "Init image", &top);
		trans[kernel_pages + i] = pa;
		if (i == 0) {
			bootinfo.init.addr = (void *) ((kernel_pages + i) << PAGE_WIDTH);
			bootinfo.init.size = INIT_SIZE;
		}
	}
	
	fix_overlap(&real_mode, &real_mode_pa, "Bootstrap trampoline", &top);
	fix_overlap(&trans, &trans_pa, "Translation table", &top);
	fix_overlap(&bootinfo, &bootinfo_pa, "Boot info", &top);
	
	printf("\nBooting the kernel...\n");
	jump_to_kernel(bootinfo_pa, sizeof(bootinfo), trans_pa, (kernel_pages + init_pages) << PAGE_WIDTH, fb, real_mode_pa);
}
