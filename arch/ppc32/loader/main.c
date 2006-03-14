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

#define KERNEL_PHYSICAL_ADDRESS 0x2000
#define KERNEL_VIRTUAL_ADDRESS 0x80002000
#define KERNEL_START &_binary_____________kernel_kernel_bin_start
#define KERNEL_END &_binary_____________kernel_kernel_bin_end
#define KERNEL_SIZE ((unsigned int) KERNEL_END - (unsigned int) KERNEL_START)

memmap_t memmap;

void bootstrap(void)
{
	printf("\nHelenOS PPC Bootloader\n");
	
	void *phys = ofw_translate(&start);
	printf("loaded at %L (physical %L)\n", &start, phys);
	
	if (!ofw_memmap(&memmap)) {
		printf("Unable to get memory map\n");
		halt();
	}
	printf("total memory %d MB\n", memmap.total >> 20);
	
	// FIXME: map just the kernel
	if (ofw_map((void *) KERNEL_PHYSICAL_ADDRESS, (void *) KERNEL_VIRTUAL_ADDRESS, memmap.total - 64 * 1024 * 1024, 0) != 0) {
		printf("Unable to map kernel memory at %L (physical %L)\n", KERNEL_VIRTUAL_ADDRESS, KERNEL_PHYSICAL_ADDRESS);
		halt();
	}
	printf("kernel memory mapped at %L (physical %L, size %d bytes)\n", KERNEL_VIRTUAL_ADDRESS, KERNEL_PHYSICAL_ADDRESS, KERNEL_SIZE);
	// FIXME: relocate the kernel in real mode
	memcpy((void *) KERNEL_VIRTUAL_ADDRESS, KERNEL_START, KERNEL_SIZE);
	
	// FIXME: proper hardware detection & mapping
	ofw_map((void *) 0x84000000, (void *) 0xf0000000, 0x01000000, 0);
	ofw_map((void *) 0x80816000, (void *) 0xf2000000, 0x00018000, 0);
	
	printf("Booting the kernel...\n");
	
	flush_instruction_cache();
	jump_to_kernel((void *) KERNEL_VIRTUAL_ADDRESS, ofw_translate(&memmap));
}
