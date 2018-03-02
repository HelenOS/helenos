/*
 * Copyright (c) 2007 Michal Kebrt
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

/** @addtogroup arm32boot
 * @{
 */
/** @file
 * @brief Bootstrap.
 */

#include <arch/main.h>
#include <arch/asm.h>
#include <arch/mm.h>
#include <halt.h>
#include <printf.h>
#include <memstr.h>
#include <version.h>
#include <macros.h>
#include <align.h>
#include <stdbool.h>
#include <str.h>
#include <errno.h>
#include <inflate.h>
#include <arch/cp15.h>
#include "../../components.h"

#define TOP2ADDR(top)  (((void *) PA2KA(BOOT_OFFSET)) + (top))

extern void *bdata_start;
extern void *bdata_end;

static inline void clean_dcache_poc(void *address, size_t size)
{
	const uintptr_t addr = (uintptr_t) address;

#if !defined(PROCESSOR_ARCH_armv7_a)
	bool sep;
	if (MIDR_read() != CTR_read()) {
		sep = (CTR_read() & CTR_SEP_FLAG) == CTR_SEP_FLAG;
	} else {
		printf("Unknown cache type.\n");
		halt();
	}
#endif

	for (uintptr_t a = ALIGN_DOWN(addr, CP15_C7_MVA_ALIGN); a < addr + size;
	    a += CP15_C7_MVA_ALIGN) {
#if defined(PROCESSOR_ARCH_armv7_a)
		DCCMVAC_write(a);
#else
		if (sep)
			DCCMVA_write(a);
		else
			CCMVA_write(a);
#endif
	}
}

static bootinfo_t bootinfo;

void bootstrap(void)
{
	/* Enable MMU and caches */
	mmu_start();
	version_print();

	printf("Boot data: %p -> %p\n", &bdata_start, &bdata_end);
	printf("\nMemory statistics\n");
	printf(" %p|%p: bootstrap stack\n", &boot_stack, &boot_stack);
	printf(" %p|%p: bootstrap page table\n", &boot_pt, &boot_pt);
	printf(" %p|%p: boot info structure\n", &bootinfo, &bootinfo);
	printf(" %p|%p: kernel entry point\n",
	    (void *) PA2KA(BOOT_OFFSET), (void *) BOOT_OFFSET);

	for (size_t i = 0; i < COMPONENTS; i++) {
		printf(" %p|%p: %s image (%u/%u bytes)\n", components[i].addr,
		    components[i].addr, components[i].name, components[i].inflated,
		    components[i].size);
	}

	void *dest[COMPONENTS];
	size_t top = 0;
	size_t cnt = 0;
	bootinfo.cnt = 0;
	for (size_t i = 0; i < min(COMPONENTS, TASKMAP_MAX_RECORDS); i++) {
		top = ALIGN_UP(top, PAGE_SIZE);

		if (i > 0) {
			bootinfo.tasks[bootinfo.cnt].addr = TOP2ADDR(top);
			bootinfo.tasks[bootinfo.cnt].size = components[i].inflated;

			str_cpy(bootinfo.tasks[bootinfo.cnt].name,
			    BOOTINFO_TASK_NAME_BUFLEN, components[i].name);

			bootinfo.cnt++;
		}

		dest[i] = TOP2ADDR(top);
		top += components[i].inflated;
		cnt++;
	}

	printf("\nInflating components ... ");

	for (size_t i = cnt; i > 0; i--) {
		void *tail = components[i - 1].addr + components[i - 1].size;
		if (tail >= dest[i - 1]) {
			printf("\n%s: Image too large to fit (%p >= %p), halting.\n",
			    components[i].name, tail, dest[i - 1]);
			halt();
		}

		printf("%s ", components[i - 1].name);

		int err = inflate(components[i - 1].addr, components[i - 1].size,
		    dest[i - 1], components[i - 1].inflated);
		if (err != EOK) {
			printf("\n%s: Inflating error %d\n", components[i - 1].name, err);
			halt();
		}
		/* Make sure data are in the memory, ICache will need them */
		clean_dcache_poc(dest[i - 1], components[i - 1].inflated);
	}

	printf(".\n");

	/* Flush PT too. We need this if we disable caches later */
	clean_dcache_poc(boot_pt, PTL0_ENTRIES * PTL0_ENTRY_SIZE);

	printf("Booting the kernel...\n");
	jump_to_kernel((void *) PA2KA(BOOT_OFFSET), &bootinfo);
}

/** @}
 */
