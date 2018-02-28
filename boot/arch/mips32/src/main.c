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

#define TOP2ADDR(top)  (((void *) PA2KA(BOOT_OFFSET)) + (top))

static bootinfo_t *bootinfo = (bootinfo_t *) PA2KA(BOOTINFO_OFFSET);
static uint32_t *cpumap = (uint32_t *) PA2KA(CPUMAP_OFFSET);

void bootstrap(void)
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

	size_t i;
	for (i = 0; i < COMPONENTS; i++)
		printf(" %p|%p: %s image (%zu/%zu bytes)\n", components[i].addr,
		    (uintptr_t) components[i].addr >= PA2KSEG(0) ?
		    (void *) KSEG2PA(components[i].addr) :
		    (void *) KA2PA(components[i].addr),
		    components[i].name, components[i].inflated,
		    components[i].size);

	void *dest[COMPONENTS];
	size_t top = 0;
	size_t cnt = 0;
	bootinfo->cnt = 0;
	for (i = 0; i < min(COMPONENTS, TASKMAP_MAX_RECORDS); i++) {
		top = ALIGN_UP(top, PAGE_SIZE);

		if (i > 0) {
			bootinfo->tasks[bootinfo->cnt].addr = TOP2ADDR(top);
			bootinfo->tasks[bootinfo->cnt].size = components[i].inflated;

			str_cpy(bootinfo->tasks[bootinfo->cnt].name,
			    BOOTINFO_TASK_NAME_BUFLEN, components[i].name);

			bootinfo->cnt++;
		}

		dest[i] = TOP2ADDR(top);
		top += components[i].inflated;
		cnt++;
	}

	printf("\nInflating components ... ");

	for (i = cnt; i > 0; i--) {
#ifdef MACHINE_msim
		void *tail = dest[i - 1] + components[i].inflated;
		if (tail >= ((void *) PA2KA(LOADER_OFFSET))) {
			printf("\n%s: Image too large to fit (%p >= %p), halting.\n",
			    components[i].name, tail, (void *) PA2KA(LOADER_OFFSET));
			halt();
		}
#endif

		printf("%s ", components[i - 1].name);

		int err = inflate(components[i - 1].addr, components[i - 1].size,
		    dest[i - 1], components[i - 1].inflated);

		if (err != EOK) {
			printf("\n%s: Inflating error %d, halting.\n",
			    components[i - 1].name, err);
			halt();
		}
	}

	printf(".\n");

	printf("Copying CPU map ... \n");

	bootinfo->cpumap = 0;
	for (i = 0; i < CPUMAP_MAX_RECORDS; i++) {
		if (cpumap[i] != 0)
			bootinfo->cpumap |= (1 << i);
	}

	printf("Booting the kernel ... \n");
	jump_to_kernel((void *) PA2KA(BOOT_OFFSET), bootinfo);
}
