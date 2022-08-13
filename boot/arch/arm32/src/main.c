/*
 * SPDX-FileCopyrightText: 2007 Michal Kebrt
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
#include <memstr.h>
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
