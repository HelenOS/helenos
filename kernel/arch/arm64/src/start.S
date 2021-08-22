/*
 * Copyright (c) 2015 Petr Pavlu
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

#include <abi/asmtool.h>
#include <arch/boot/boot.h>
#include <arch/mm/km.h>
#include <arch/mm/page.h>
#include <arch/regutils.h>

.section K_TEXT_START, "ax"

.macro dcache_flush addr size temp0 temp1
	mov \temp0, \addr
	mov \temp1, xzr

	0:
		/* Data or Unified Cache Line Clean */
		dc cvau, \temp0
		add \temp0, \temp0, #4
		add \temp1, \temp1, #4
		cmp \temp1, \size
		blo 0b

	dsb ish
	isb
.endm

/** Kernel entry
 *
 * MMU must be disabled at this point.
 *
 * @param x0 Kernel entry point (kernel_image_start).
 * @param x1 Pointer to the bootinfo structure.
 *
 */
SYMBOL(kernel_image_start)
	/* Get address of the main memory and remember it. */
	adrp x20, kernel_image_start - BOOT_OFFSET
	adrp x2, physmem_base
	str x20, [x2]

	/* Flush the data cache of physmem_base. */
	mov x28, #8
	dcache_flush x2 x28 x29 x30

	/*
	 * Set up address translation that identity maps the 1 GiB area that
	 * is holding the current execution page.
	 */

	/* Prepare the level 0 page table. */
	adrp x2, lower_page_table_level0
	lsr x3, x20, #PTL0_VA_SHIFT
	and x3, x3, #PTL0_VA_MASK
	add x2, x2, x3, lsl #PTL_ENTRY_SIZE_SHIFT
	mov x3, #( \
	    1 << PTE_ACCESS_SHIFT | \
	    PTE_L012_TYPE_TABLE << PTE_TYPE_SHIFT | \
	    1 << PTE_PRESENT_SHIFT)
	adrp x4, lower_page_table_level1
	lsr x4, x4, #FRAME_WIDTH
	orr x3, x3, x4, lsl #PTE_NEXT_LEVEL_ADDRESS_SHIFT
	str x3, [x2]

	/* Prepare the level 1 page table. */
	adrp x2, lower_page_table_level1
	lsr x3, x20, #PTL1_VA_SHIFT
	and x3, x3, #PTL1_VA_MASK
	add x2, x2, x3, lsl #PTL_ENTRY_SIZE_SHIFT
	mov x3, #( \
	    1 << PTE_ACCESS_SHIFT | \
	    MAIR_EL1_NORMAL_MEMORY_INDEX << PTE_ATTR_INDEX_SHIFT | \
	    PTE_L012_TYPE_BLOCK << PTE_TYPE_SHIFT | \
	    1 << PTE_PRESENT_SHIFT)
	lsr x4, x20, #FRAME_WIDTH
	orr x3, x3, x4, lsl #PTE_OUTPUT_ADDRESS_SHIFT
	str x3, [x2]

	/*
	 * Set up address translation that maps the first 4 GiB of the kernel
	 * identity virtual address space to the first 4 GiB of the physical
	 * memory.
	 */

	mov x21, #KM_ARM64_IDENTITY_START
	ldr x22, =(1024 * 1024 * 1024)

	/* Prepare the level 0 page table. */
	adrp x2, upper_page_table_level0
	lsr x3, x21, #PTL0_VA_SHIFT
	and x3, x3, #PTL0_VA_MASK
	add x2, x2, x3, lsl #PTL_ENTRY_SIZE_SHIFT
	mov x3, #( \
	    1 << PTE_ACCESS_SHIFT | \
	    PTE_L012_TYPE_TABLE << PTE_TYPE_SHIFT | \
	    1 << PTE_PRESENT_SHIFT)
	adrp x4, upper_page_table_level1
	lsr x4, x4, #FRAME_WIDTH
	orr x3, x3, x4, lsl #PTE_NEXT_LEVEL_ADDRESS_SHIFT
	str x3, [x2]

	/* Prepare the level 1 page table. */
	adrp x2, upper_page_table_level1
	lsr x3, x21, #PTL1_VA_SHIFT
	and x3, x3, #PTL1_VA_MASK
	add x2, x2, x3, lsl #PTL_ENTRY_SIZE_SHIFT
	mov x3, #( \
	    1 << PTE_ACCESS_SHIFT | \
	    MAIR_EL1_NORMAL_MEMORY_INDEX << PTE_ATTR_INDEX_SHIFT | \
	    PTE_L012_TYPE_BLOCK << PTE_TYPE_SHIFT | \
	    1 << PTE_PRESENT_SHIFT)
	lsr x4, x20, #FRAME_WIDTH
	orr x3, x3, x4, lsl #PTE_OUTPUT_ADDRESS_SHIFT
	str x3, [x2]

	/* 2nd GiB */
	add x23, x20, x22
	add x24, x21, x22

	adrp x2, upper_page_table_level1
	lsr x3, x24, #PTL1_VA_SHIFT
	and x3, x3, #PTL1_VA_MASK
	add x2, x2, x3, lsl #PTL_ENTRY_SIZE_SHIFT
	mov x3, #( \
	    1 << PTE_ACCESS_SHIFT | \
	    MAIR_EL1_NORMAL_MEMORY_INDEX << PTE_ATTR_INDEX_SHIFT | \
	    PTE_L012_TYPE_BLOCK << PTE_TYPE_SHIFT | \
	    1 << PTE_PRESENT_SHIFT)
	lsr x4, x23, #FRAME_WIDTH
	orr x3, x3, x4, lsl #PTE_OUTPUT_ADDRESS_SHIFT
	str x3, [x2]

	/* 3rd GiB */
	add x23, x23, x22
	add x24, x24, x22

	adrp x2, upper_page_table_level1
	lsr x3, x24, #PTL1_VA_SHIFT
	and x3, x3, #PTL1_VA_MASK
	add x2, x2, x3, lsl #PTL_ENTRY_SIZE_SHIFT
	mov x3, #( \
	    1 << PTE_ACCESS_SHIFT | \
	    MAIR_EL1_NORMAL_MEMORY_INDEX << PTE_ATTR_INDEX_SHIFT | \
	    PTE_L012_TYPE_BLOCK << PTE_TYPE_SHIFT | \
	    1 << PTE_PRESENT_SHIFT)
	lsr x4, x23, #FRAME_WIDTH
	orr x3, x3, x4, lsl #PTE_OUTPUT_ADDRESS_SHIFT
	str x3, [x2]

	/* 4th GiB */
	add x23, x23, x22
	add x24, x24, x22

	adrp x2, upper_page_table_level1
	lsr x3, x24, #PTL1_VA_SHIFT
	and x3, x3, #PTL1_VA_MASK
	add x2, x2, x3, lsl #PTL_ENTRY_SIZE_SHIFT
	mov x3, #( \
	    1 << PTE_ACCESS_SHIFT | \
	    MAIR_EL1_DEVICE_MEMORY_INDEX << PTE_ATTR_INDEX_SHIFT | \
	    PTE_L012_TYPE_BLOCK << PTE_TYPE_SHIFT | \
	    1 << PTE_PRESENT_SHIFT)
	lsr x4, x23, #FRAME_WIDTH
	orr x3, x3, x4, lsl #PTE_OUTPUT_ADDRESS_SHIFT
	str x3, [x2]

	/* Flush the data cache of page tables. */
	adrp x27, lower_page_table_level0
	mov x28, #4096
	dcache_flush x27 x28 x29 x30

	adrp x27, lower_page_table_level1
	mov x28, #4096
	dcache_flush x27 x28 x29 x30

	adrp x27, upper_page_table_level0
	mov x28, #4096
	dcache_flush x27 x28 x29 x30

	adrp x27, upper_page_table_level1
	mov x28, #4096
	dcache_flush x27 x28 x29 x30

	/* Make sure there are not any stale TLB entries. */
	tlbi vmalle1is
	dsb ish

	/*
	 * Set TCR_EL1:
	 * [63:39] - Reserved 0.
	 * [38]    - TBI1=0, top byte of an address is used in the address
	 *           calculation for the TTBR1_EL1 region.
	 * [37]    - TBI0=0, top byte of an address is used in the address
	 *           calculation for the TTBR0_EL1 region.
	 * [36]    - AS=1, the upper 16 bits of TTBR0_EL1 and TTBR1_EL1 are used
	 *           for allocation and matching in the TLB.
	 * [35]    - Reserved 0.
	 * [34:32] - IPS=101, intermediate physical address size is 48 bits,
	 *           256TB.
	 * [31:30] - TG1=10, TTBR1_EL1 granule size is 4KB.
	 * [29:28] - SH1=11, memory associated with translation table walks
	 *           using TTBR1_EL1 is inner shareable.
	 * [27:26] - ORGN1=01, memory associated with translation table walks
	 *           using TTBR1_EL1 is normal memory, outer write-through
	 *           cacheable.
	 * [25:24] - IRGN1=01, memory associated with translation table walks
	 *           using TTBR1_EL1 is normal memory, inner write-back
	 *           write-allocate cacheable.
	 * [23]    - EPD1=0, perform translation table walks using TTBR1_EL1.
	 * [22]    - A1=0, TTBR0_EL1.ASID defines the ASID.
	 * [21:16] - T1SZ=010000, size of the memory region addressed by
	 *           TTBR1_EL1 is 2^(64 - 16) bytes.
	 * [15:14] - TG0=00, TTBR0_EL1 granule size is 4KB.
	 * [13:12] - SH0=11, memory associated with translation table walks
	 *           using TTBR0_EL1 is inner shareable.
	 * [11:10] - ORGN0=01, memory associated with translation table walks
	 *           using TTBR0_EL1 is normal memory, outer write-through
	 *           cacheable.
	 * [9:8]   - IRGN0=01, memory associated with translation table walks
	 *           using TTBR0_EL1 is normal memory, inner write-back
	 *           write-allocate cacheable.
	 * [7]     - EPD0=0, perform translation table walks using TTBR0.
	 * [6]     - Reserved 0.
	 * [5:0]   - T0SZ=010000, size of the memory region addressed by
	 *           TTBR0_EL1 is 2^(64 - 16) bytes.
	 */
	ldr x2, =0x00000015b5103510
	msr tcr_el1, x2

	/* Initialize memory attributes. */
	ldr x2, =(MAIR_EL1_DEVICE_MEMORY_ATTR << \
	    (MAIR_EL1_DEVICE_MEMORY_INDEX * MAIR_EL1_ATTR_SHIFT) | \
	    MAIR_EL1_NORMAL_MEMORY_ATTR << \
	    (MAIR_EL1_NORMAL_MEMORY_INDEX * MAIR_EL1_ATTR_SHIFT))
	msr mair_el1, x2

	/* Set translation tables. */
	adrp x2, lower_page_table_level0
	msr ttbr0_el1, x2
	adrp x2, upper_page_table_level0
	msr ttbr1_el1, x2
	isb

	/*
	 * Set SCTLR_EL1:
	 * [31:30] - Reserved 0.
	 * [29:28] - Reserved 1.
	 * [27]    - Reserved 0.
	 * [26]    - UCI=0, any attempt to execute cache maintenance
	 *           instructions at EL0 is trapped to EL1.
	 * [25]    - EE=0, explicit data accesses at EL1, and stage 1
	 *           translation table walks in the EL1&0 translation regime are
	 *           little-endian.
	 * [24]    - E0E=0, explicit data accesses at EL1 are little-endian.
	 * [23:22] - Reserved 1.
	 * [21]    - Reserved 0.
	 * [20]    - Reserved 1.
	 * [19]    - WXN=0, regions with write permission are not forced to
	 *           Execute Never.
	 * [18]    - nTWE=0, any attempt to execute WFE at EL0 is trapped to
	 *           EL1.
	 * [17]    - Reserved 0.
	 * [16]    - nTWI=0, any attempt to execute WFI at EL0 is trapped to
	 *           EL1.
	 * [15]    - UCT=0, accesses to CTR_EL0 from EL0 are trapped to EL1.
	 * [14]    - DZE=0, any attempt to execute DC ZVA at EL0 is trapped to
	 *           EL1.
	 * [13]    - Reserved 0.
	 * [12]    - I=1, this control has no effect on the cacheability of
	 *           instruction access to normal memory.
	 * [11]    - Reserved 1.
	 * [10]    - Reserved 0.
	 * [9]     - UMA=0, any attempt to execute MSR/MRS that accesses DAIF at
	 *           EL0 is trapped to EL1.
	 * [8]     - SED=1, SETEND is undefined at EL0 using AArch32.
	 * [7]     - ITD=1, disables some uses of IT at EL0 using AArch32.
	 * [6]     - Reserved 0.
	 * [5]     - CP15BEN=0, CP15DMB/DSB/ISB is undefined at EL0 using
	 *           AArch32.
	 * [4]     - SA0=1, use of stack pointer with load/store at EL0 must be
	 *           aligned to a 16-byte boundary.
	 * [3]     - SA=1, use of stack pointer with load/store at EL1 must be
	 *           aligned to a 16-byte boundary.
	 * [2]     - C=1, this control has no effect on the cacheability of data
	 *           access to normal memory from EL0 and EL1, and normal memory
	 *           accesses to the EL1&0 stage 1 translation tables.
	 * [1]     - A=0, instructions that load/store registers (other than
	 *           load/store exclusive and load-acquire/store-release) do not
	 *           check that the address being accessed is aligned to the
	 *           size of the data element(s) being accessed.
	 * [0]     - M=1, EL1 and EL0 stage 1 address translation enabled.
	 */
	ldr w2, =0x30d0199d
	msr sctlr_el1, x2
	isb

	/*
	 * MMU is enabled at this point (SCTLR_EL1.M=1), switch to the kernel
	 * mapping.
	 */
	ldr x2, =1f
	br x2
1:

	/* Disable access to low addresses. */
	mov x2, #0
	msr ttbr0_el1, x2
	isb
	tlbi vmalle1is
	dsb ish

	/* Jump on a temporary stack. */
	ldr x2, =temp_stack
	mov sp, x2

	/* Create the first stack frame. */
	mov x29, #0
	mov x30, #0
	stp x29, x30, [sp, #-16]!
	mov x29, sp

	/* PA2KA(bootinfo). */
	sub x1, x1, x20
	ldr x2, =KM_ARM64_IDENTITY_START
	add x1, x1, x2

	bl arm64_pre_main
	bl main_bsp

.section K_DATA_START, "ax"

	/* Page tables. */
.align 12
lower_page_table_level0:
	.space 4096

lower_page_table_level1:
	.space 4096

upper_page_table_level0:
	.space 4096

upper_page_table_level1:
	.space 4096

	/* Physical memory base address. */
.align 12
SYMBOL(physmem_base)
	.quad 0

	/* Temporary stack. */
.align 10
	.space 1024
temp_stack:
