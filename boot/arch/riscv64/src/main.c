/*
 * Copyright (c) 2016 Martin Decky
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
#include <version.h>
#include <typedefs.h>
#include <printf.h>
#include <macros.h>
#include <errno.h>
#include <align.h>
#include <str.h>
#include <halt.h>
#include <inflate.h>
#include <arch/_components.h>

#define KA2PA(x)  (((uintptr_t) (x)) - UINT64_C(0xffff800000000000))
#define PA2KA(x)  (((uintptr_t) (x)) + UINT64_C(0xffff800000000000))

static bootinfo_t bootinfo;

void bootstrap(void)
{
	version_print();
	
	// FIXME TODO: read from device tree
	bootinfo.memmap.total = 1024 * 1024 * 1024;
	bootinfo.memmap.cnt = 0;
	
	printf("\nMemory statistics (total %lu MB)\n\n", bootinfo.memmap.total >> 20);
	printf(" %p: boot info structure\n", &bootinfo);
	
	uintptr_t top = 0;
	
	for (size_t i = 0; i < COMPONENTS; i++) {
		printf(" %p: %s image (%zu/%zu bytes)\n", components[i].start,
		    components[i].name, components[i].inflated,
		    components[i].size);
		
		uintptr_t tail = (uintptr_t) components[i].start +
		    components[i].size;
		if (tail > top)
			top = tail;
	}
	
	top = ALIGN_UP(top, PAGE_SIZE);
	printf(" %p: inflate area\n", (void *) top);
	
	void *kernel_entry = NULL;
	void *dest[COMPONENTS];
	size_t cnt = 0;
	bootinfo.taskmap.cnt = 0;
	
	for (size_t i = 0; i < min(COMPONENTS, TASKMAP_MAX_RECORDS); i++) {
		top = ALIGN_UP(top, PAGE_SIZE);
		
		if (i > 0) {
			bootinfo.taskmap.tasks[bootinfo.taskmap.cnt].addr =
			    (void *) PA2KA(top);
			bootinfo.taskmap.tasks[bootinfo.taskmap.cnt].size =
			    components[i].inflated;
			
			str_cpy(bootinfo.taskmap.tasks[bootinfo.taskmap.cnt].name,
			    BOOTINFO_TASK_NAME_BUFLEN, components[i].name);
			
			bootinfo.taskmap.cnt++;
		} else
			kernel_entry = (void *) top;
		
		dest[i] = (void *) top;
		top += components[i].inflated;
		cnt++;
	}
	
	printf(" %p: kernel entry point\n", kernel_entry);
	
	if (top >= bootinfo.memmap.total) {
		printf("Not enough physical memory available.\n");
		printf("The boot image is too large. Halting.\n");
		halt();
	}
	
	printf("\nInflating components ... ");
	
	for (size_t i = cnt; i > 0; i--) {
		printf("%s ", components[i - 1].name);
		
		int err = inflate(components[i - 1].start, components[i - 1].size,
		    dest[i - 1], components[i - 1].inflated);
		
		if (err != EOK) {
			printf("\n%s: Inflating error %d, halting.\n",
			    components[i - 1].name, err);
			halt();
		}
	}
	
	printf(".\n");
	
	printf("Booting the kernel...\n");
	jump_to_kernel(kernel_entry, PA2KA(&bootinfo));
}
