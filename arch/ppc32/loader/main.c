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

memmap_t memmap;


static void check_align(const void *addr, const char *desc)
{
	if ((unsigned int) addr % PAGE_SIZE != 0) {
		printf("Error: %s not on page boundary\n", desc);
		halt();
	}
}


void bootstrap(void)
{
	printf("\nHelenOS PPC Bootloader\n");
	
	check_align(KERNEL_START, "Kernel image");
	check_align(&real_mode, "Bootstrap trampoline");
	check_align(&trans, "Translation table");
	
	void *real_mode_pa = ofw_translate(&real_mode);
	void *trans_pa = ofw_translate(&trans);
	void *memmap_pa = ofw_translate(&memmap);
	
	printf("Memory statistics\n");
	printf(" kernel image         at %L (size %d bytes)\n", KERNEL_START, KERNEL_SIZE);
	printf(" memory map           at %L (physical %L)\n", &memmap, memmap_pa);
	printf(" bootstrap trampoline at %L (physical %L)\n", &real_mode, real_mode_pa);
	printf(" translation table    at %L (physical %L)\n", &trans, trans_pa);
	
	if (!ofw_memmap(&memmap)) {
		printf("Unable to get memory map\n");
		halt();
	}
	printf("Total memory %d MB\n", memmap.total >> 20);
	
	unsigned int addr;
	unsigned int pages;
	for (addr = 0, pages = 0; addr < KERNEL_SIZE; addr += PAGE_SIZE, pages++) {
		void *pa = ofw_translate(KERNEL_START + addr);
		if ((unsigned int) pa < KERNEL_SIZE) {
			printf("Error: Kernel image overlaps kernel physical area\n");
			halt();
		}
		trans[addr >> PAGE_WIDTH] = pa;
	}
	
	printf("Booting the kernel...\n");
	jump_to_kernel(memmap_pa, trans_pa, pages, real_mode_pa);
}
