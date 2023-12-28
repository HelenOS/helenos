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

/** @addtogroup boot_arm32
 * @{
 */
/** @file
 * @brief Bootstrap.
 */

#include <arch/main.h>
#include <arch/types.h>
#include <arch/asm.h>
#include <arch/mm.h>
#include <halt.h>
#include <printf.h>
#include <mem.h>
#include <version.h>
#include <macros.h>
#include <align.h>
#include <stdbool.h>
#include <str.h>
#include <errno.h>
#include <inflate.h>
#include <arch/cp15.h>
#include <payload.h>
#include <kernel.h>

static void clean_dcache_poc(void *address, size_t size)
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

	printf("Boot loader: %p -> %p\n", loader_start, loader_end);
	printf("\nMemory statistics\n");
	printf(" %p|%p: bootstrap stack\n", &boot_stack, &boot_stack);
	printf(" %p|%p: bootstrap page table\n", &boot_pt, &boot_pt);
	printf(" %p|%p: boot info structure\n", &bootinfo, &bootinfo);
	printf(" %p|%p: kernel entry point\n",
	    (void *) PA2KA(BOOT_OFFSET), (void *) BOOT_OFFSET);

	// FIXME: Use the correct value.
	uint8_t *kernel_dest = (uint8_t *) BOOT_OFFSET;
	uint8_t *ram_end = kernel_dest + (1 << 24);

	extract_payload(&bootinfo.taskmap, kernel_dest, ram_end,
	    PA2KA(kernel_dest), clean_dcache_poc);

	/* Flush PT too. We need this if we disable caches later */
	clean_dcache_poc(boot_pt, PTL0_ENTRIES * PTL0_ENTRY_SIZE);

	uintptr_t entry = check_kernel((void *) PA2KA(BOOT_OFFSET));

	printf("Booting the kernel...\n");
	jump_to_kernel((void *) entry, &bootinfo);
}

/** @}
 */
