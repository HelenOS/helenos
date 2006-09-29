#
# Copyright (C) 2005 Jakub Jermar
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# - Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# - The name of the author may not be used to endorse or promote products
#   derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

#include <arch/arch.h>
#include <arch/regdef.h>
#include <arch/boot/boot.h>

#include <arch/mm/mmu.h>
#include <arch/mm/tlb.h>
#include <arch/mm/tte.h>

#ifdef CONFIG_SMP
#include <arch/context_offset.h>
#endif

.register %g2, #scratch
.register %g3, #scratch

.section K_TEXT_START, "ax"

/*
 * Here is where the kernel is passed control
 * from the boot loader.
 * 
 * The registers are expected to be in this state:
 * - %o0 non-zero for the bootstrap processor, zero for application/secondary processors
 * - %o1 bootinfo structure address
 * - %o2 bootinfo structure size
 *
 * Moreover, we depend on boot having established the
 * following environment:
 * - TLBs are on
 * - identity mapping for the kernel image
 * - identity mapping for memory stack
 */

.global kernel_image_start
kernel_image_start:
	mov %o0, %l7

	/*
	 * Setup basic runtime environment.
	 */

	flushw					! flush all but the active register window

	wrpr %g0, 0, %tl			! TL = 0, primary context register is used

	wrpr %g0, PSTATE_PRIV_BIT, %pstate	! Disable interrupts and disable 32-bit address masking.

	wrpr %g0, 0, %pil			! intialize %pil

	/*
	 * Switch to kernel trap table.
	 */
	sethi %hi(trap_table), %g1
	wrpr %g1, %lo(trap_table), %tba

	/* 
	 * Take over the DMMU by installing global locked
	 * TTE entry identically mapping the first 4M
	 * of memory.
	 *
	 * In case of DMMU, no FLUSH instructions need to be
	 * issued. Because of that, the old DTLB contents can
	 * be demapped pretty straightforwardly and without
	 * causing any traps.
	 */

	wr %g0, ASI_DMMU, %asi

#define SET_TLB_DEMAP_CMD(r1, context_id) \
	set (TLB_DEMAP_CONTEXT<<TLB_DEMAP_TYPE_SHIFT) | (context_id<<TLB_DEMAP_CONTEXT_SHIFT), %r1
	
	! demap context 0
	SET_TLB_DEMAP_CMD(g1, TLB_DEMAP_NUCLEUS)
	stxa %g0, [%g1] ASI_DMMU_DEMAP			
	membar #Sync

#define SET_TLB_TAG(r1, context) \
	set VMA | (context<<TLB_TAG_ACCESS_CONTEXT_SHIFT), %r1

	! write DTLB tag
	SET_TLB_TAG(g1, MEM_CONTEXT_KERNEL)
	stxa %g1, [VA_DMMU_TAG_ACCESS] %asi			
	membar #Sync

#define SET_TLB_DATA(r1, r2, imm) \
	set TTE_CV | TTE_CP | TTE_P | LMA | imm, %r1; \
	set PAGESIZE_4M, %r2; \
	sllx %r2, TTE_SIZE_SHIFT, %r2; \
	or %r1, %r2, %r1; \
	mov 1, %r2; \
	sllx %r2, TTE_V_SHIFT, %r2; \
	or %r1, %r2, %r1;
	
	! write DTLB data and install the kernel mapping
	SET_TLB_DATA(g1, g2, TTE_L | TTE_W)	! use non-global mapping
	stxa %g1, [%g0] ASI_DTLB_DATA_IN_REG		
	membar #Sync

	/*
	 * Because we cannot use global mappings (because we want to
	 * have separate 64-bit address spaces for both the kernel
	 * and the userspace), we prepare the identity mapping also in
	 * context 1. This step is required by the
	 * code installing the ITLB mapping.
	 */
	! write DTLB tag of context 1 (i.e. MEM_CONTEXT_TEMP)
	SET_TLB_TAG(g1, MEM_CONTEXT_TEMP)
	stxa %g1, [VA_DMMU_TAG_ACCESS] %asi			
	membar #Sync

	! write DTLB data and install the kernel mapping in context 1
	SET_TLB_DATA(g1, g2, TTE_W)			! use non-global mapping
	stxa %g1, [%g0] ASI_DTLB_DATA_IN_REG		
	membar #Sync
	
	/*
	 * Now is time to take over the IMMU.
	 * Unfortunatelly, it cannot be done as easily as the DMMU,
	 * because the IMMU is mapping the code it executes.
	 *
	 * [ Note that brave experiments with disabling the IMMU
	 * and using the DMMU approach failed after a dozen
	 * of desparate days with only little success. ]
	 *
	 * The approach used here is inspired from OpenBSD.
	 * First, the kernel creates IMMU mapping for itself
	 * in context 1 (MEM_CONTEXT_TEMP) and switches to
	 * it. Context 0 (MEM_CONTEXT_KERNEL) can be demapped
	 * afterwards and replaced with the kernel permanent
	 * mapping. Finally, the kernel switches back to
	 * context 0 and demaps context 1.
	 *
	 * Moreover, the IMMU requires use of the FLUSH instructions.
	 * But that is OK because we always use operands with
	 * addresses already mapped by the taken over DTLB.
	 */
	
	set kernel_image_start, %g5
	
	! write ITLB tag of context 1
	SET_TLB_TAG(g1, MEM_CONTEXT_TEMP)
	mov VA_DMMU_TAG_ACCESS, %g2
	stxa %g1, [%g2] ASI_IMMU
	flush %g5

	! write ITLB data and install the temporary mapping in context 1
	SET_TLB_DATA(g1, g2, 0)			! use non-global mapping
	stxa %g1, [%g0] ASI_ITLB_DATA_IN_REG		
	flush %g5
	
	! switch to context 1
	mov MEM_CONTEXT_TEMP, %g1
	stxa %g1, [VA_PRIMARY_CONTEXT_REG] %asi	! ASI_DMMU is correct here !!!
	flush %g5
	
	! demap context 0
	SET_TLB_DEMAP_CMD(g1, TLB_DEMAP_NUCLEUS)
	stxa %g0, [%g1] ASI_IMMU_DEMAP			
	flush %g5
	
	! write ITLB tag of context 0
	SET_TLB_TAG(g1, MEM_CONTEXT_KERNEL)
	mov VA_DMMU_TAG_ACCESS, %g2
	stxa %g1, [%g2] ASI_IMMU
	flush %g5

	! write ITLB data and install the permanent kernel mapping in context 0
	SET_TLB_DATA(g1, g2, TTE_L)		! use non-global mapping
	stxa %g1, [%g0] ASI_ITLB_DATA_IN_REG		
	flush %g5

	! enter nucleus - using context 0
	wrpr %g0, 1, %tl

	! demap context 1
	SET_TLB_DEMAP_CMD(g1, TLB_DEMAP_PRIMARY)
	stxa %g0, [%g1] ASI_IMMU_DEMAP			
	flush %g5
	
	! set context 0 in the primary context register
	stxa %g0, [VA_PRIMARY_CONTEXT_REG] %asi	! ASI_DMMU is correct here !!!
	flush %g5
	
	! leave nucleus - using primary context, i.e. context 0
	wrpr %g0, 0, %tl

	brz %l7, 1f				! skip if you are not the bootstrap CPU
	nop

	sethi %hi(bootinfo), %o0
	call memcpy				! copy bootinfo
	or %o0, %lo(bootinfo), %o0

	call arch_pre_main
	nop
	
	call main_bsp
	nop

	/* Not reached. */

0:
	ba 0b
	nop


	/*
	 * Read MID from the processor.
	 */
1:
	ldxa [%g0] ASI_UPA_CONFIG, %g1
	srlx %g1, UPA_CONFIG_MID_SHIFT, %g1
	and %g1, UPA_CONFIG_MID_MASK, %g1

#ifdef CONFIG_SMP
	/*
	 * Active loop for APs until the BSP picks them up.
	 * A processor cannot leave the loop until the
	 * global variable 'waking_up_mid' equals its
	 * MID.
	 */
	set waking_up_mid, %g2
2:
	ldx [%g2], %g3
	cmp %g3, %g1
	bne 2b
	nop

	/*
	 * Configure stack for the AP.
	 * The AP is expected to use the stack saved
	 * in the ctx global variable.
	 */
	set ctx, %g1
	add %g1, OFFSET_SP, %g1
	ldx [%g1], %o6

	call main_ap
	nop

	/* Not reached. */
#endif
	
0:
	ba 0b
	nop
