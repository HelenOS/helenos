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
#include "msim.h"
#include "asm.h"
#include "_components.h"

#define KERNEL_VIRTUAL_ADDRESS 0x80100000

void bootstrap(void)
{
	printf("HelenOS MIPS Bootloader\n");
	
	component_t components[COMPONENTS];
	bootinfo_t bootinfo;
	init_components(components);
	
	printf("\nMemory statistics\n");
	printf(" kernel entry point at %L\n", KERNEL_VIRTUAL_ADDRESS);
	printf(" %L: boot info structure\n", &bootinfo);
	
	unsigned int i;
	for (i = 0; i < COMPONENTS; i++)
		printf(" %L: %s image (size %d bytes)\n", components[i].start, components[i].name, components[i].size);
	
	printf("\nCopying components\n");
	unsigned int top = 0;
	bootinfo.cnt = 0;
	for (i = 0; i < COMPONENTS; i++) {
		printf(" %s...", components[i].name);
		top = ALIGN_UP(top, PAGE_SIZE);
		memcpy(((void *) KERNEL_VIRTUAL_ADDRESS) + top, components[i].start, components[i].size);
		if (i > 0) {
			bootinfo.tasks[bootinfo.cnt].addr = ((void *) KERNEL_VIRTUAL_ADDRESS) + top;
			bootinfo.tasks[bootinfo.cnt].size = components[i].size;
			bootinfo.cnt++;
		}
		top += components[i].size;
		printf("done.\n");
	}
	
	printf("\nBooting the kernel...\n");
	jump_to_kernel((void *) KERNEL_VIRTUAL_ADDRESS, &bootinfo, sizeof(bootinfo));
}
