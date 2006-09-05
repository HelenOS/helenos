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

#include <arch/regdef.h>
#include <arch/boot/boot.h>

#include <arch/mm/mmu.h>
#include <arch/mm/tlb.h>
#include <arch/mm/tte.h>

.register %g2, #scratch
.register %g3, #scratch

.section K_TEXT_START, "ax"

/*
 * Here is where the kernel is passed control
 * from the boot loader.
 * 
 * The registers are expected to be in this state:
 * - %o0 bootinfo structure address
 * - %o1 bootinfo structure size
 *
 * Moreover, we depend on boot having established the
 * following environment:
 * - TLBs are on
 * - identity mapping for the kernel image
 * - identity mapping for memory stack
 */

.global kernel_image_start
kernel_image_start:

	/*
	 * Setup basic runtime environment.
	 */

	flushw				! flush all but the active register window
	wrpr %g0, 0, %tl		! TL = 0, primary context register is used

	! Disable interrupts and disable 32-bit address masking.
	rdpr %pstate, %g1
	and %g1, ~(PSTATE_AM_BIT|PSTATE_IE_BIT), %g1
	wrpr %g1, 0, %pstate

	wrpr %g0, 0, %pil		! intialize %pil

	/*
	 * Copy the bootinfo structure passed from the boot loader
	 * to the kernel bootinfo structure.
	 */
	mov %o1, %o2
	mov %o0, %o1
	set bootinfo, %o0
	call memcpy
	nop

	/*
	 * Switch to kernel trap table.
	 */
	set trap_table, %g1
	wrpr %g1, 0, %tba

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
	set TTE_L | TTE_CP | TTE_P | TTE_W | LMA | imm, %r1; \
	set PAGESIZE_4M, %r2; \
	sllx %r2, TTE_SIZE_SHIFT, %r2; \
	or %r1, %r2, %r1; \
	set 1, %r2; \
	sllx %r2, TTE_V_SHIFT, %r2; \
	or %r1, %r2, %r1;
	
	! write DTLB data and install the kernel mapping
	SET_TLB_DATA(g1, g2, 0)			! use non-global mapping
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
	SET_TLB_DATA(g1, g2, 0)			! use non-global mapping
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
	set VA_DMMU_TAG_ACCESS, %g2
	stxa %g1, [%g2] ASI_IMMU
	flush %g5

	! write ITLB data and install the temporary mapping in context 1
	SET_TLB_DATA(g1, g2, 0)			! use non-global mapping
	stxa %g1, [%g0] ASI_ITLB_DATA_IN_REG		
	flush %g5
	
	! switch to context 1
	set MEM_CONTEXT_TEMP, %g1
	stxa %g1, [VA_PRIMARY_CONTEXT_REG] %asi	! ASI_DMMU is correct here !!!
	flush %g5
	
	! demap context 0
	SET_TLB_DEMAP_CMD(g1, TLB_DEMAP_NUCLEUS)
	stxa %g0, [%g1] ASI_IMMU_DEMAP			
	flush %g5
	
	! write ITLB tag of context 0
	SET_TLB_TAG(g1, MEM_CONTEXT_KERNEL)
	set VA_DMMU_TAG_ACCESS, %g2
	stxa %g1, [%g2] ASI_IMMU
	flush %g5

	! write ITLB data and install the permanent kernel mapping in context 0
	SET_TLB_DATA(g1, g2, 0)			! use non-global mapping
	stxa %g1, [%g0] ASI_ITLB_DATA_IN_REG		
	flush %g5

	! switch to context 0
	stxa %g0, [VA_PRIMARY_CONTEXT_REG] %asi	! ASI_DMMU is correct here !!!
	flush %g5

	! ensure nucleus mapping
	wrpr %g0, 1, %tl

	! set context 1 in the primary context register
	set MEM_CONTEXT_TEMP, %g1
	stxa %g1, [VA_PRIMARY_CONTEXT_REG] %asi	! ASI_DMMU is correct here !!!
	flush %g5

	! demap context 1
	SET_TLB_DEMAP_CMD(g1, TLB_DEMAP_PRIMARY)
	stxa %g0, [%g1] ASI_IMMU_DEMAP			
	flush %g5
	
	! set context 0 in the primary context register
	stxa %g0, [VA_PRIMARY_CONTEXT_REG] %asi	! ASI_DMMU is correct here !!!
	flush %g5
	
	! set TL back to 0
	wrpr %g0, 0, %tl

	call arch_pre_main
	nop
	
	call main_bsp
	nop

	/* Not reached. */

2:
	b 2b
	nop
