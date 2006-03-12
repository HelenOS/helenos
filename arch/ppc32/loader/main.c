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
#include "ofw.h"
#include "asm.h"

#define KERNEL_LOAD_ADDRESS 0x800000
#define KERNEL_START &_binary_____________kernel_kernel_bin_start
#define KERNEL_END &_binary_____________kernel_kernel_bin_end
#define KERNEL_SIZE ((unsigned int) KERNEL_END - (unsigned int) KERNEL_START)

void bootstrap(void)
{
	printf("\nHelenOS PPC Bootloader\nLoaded at %L\nKernel size %d bytes, load address %L\n", &start, KERNEL_SIZE, KERNEL_LOAD_ADDRESS);
	
	void *addr = ofw_claim((void *) KERNEL_LOAD_ADDRESS, KERNEL_SIZE, 1);
	if (addr == NULL) {
		printf("Error: Unable to claim memory");
		halt();
	}
	printf("Claimed memory at %L\n", addr);
	memcpy(addr, KERNEL_START, KERNEL_SIZE);
	
	printf("Booting the kernel...\n");
	jump_to_kernel(addr);
}
