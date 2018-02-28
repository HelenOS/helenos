#
# Copyright (c) 2005 Jakub Jermar
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

#include <abi/asmtool.h>

#include <arch/arch.h>
#include <arch/cpu.h>
#include <arch/regdef.h>
#include <arch/boot/boot.h>
#include <arch/stack.h>

#include <arch/mm/mmu.h>
#include <arch/mm/tlb.h>
#include <arch/mm/tte.h>
#include <arch/mm/cache_spec.h>

#ifdef CONFIG_SMP
#include <arch/context_struct.h>
#endif

.register %g2, #scratch
.register %g3, #scratch

.section K_TEXT_START, "ax"

#define BSP_FLAG	1

/*
 * 2^PHYSMEM_ADDR_SIZE is the size of the physical address space on
 * a given processor.
 */
#if defined (US)
    #define PHYSMEM_ADDR_SIZE	41
#elif defined (US3)
    #define PHYSMEM_ADDR_SIZE	43
#endif

/*
 * Here is where the kernel is passed control from the boot loader.
 *
 * The registers are expected to be in this state:
 *  - %o0 starting address of physical memory
 *        + bootstrap processor flag
 *          bits 63...1: physical memory starting address / 2
 *          bit 0:       non-zero on BSP processor, zero on AP processors
 *  - %o1 bootinfo structure address (BSP only)
 *
 *
 * Moreover, we depend on boot having established the following environment:
 *  - TLBs are on
 *  - identity mapping for the kernel image
 *
 */

SYMBOL(kernel_image_start)
	mov BSP_FLAG, %l0
	and %o0, %l0, %l7			! l7 <= bootstrap processor?
	andn %o0, %l0, %l6			! l6 <= start of physical memory

	! Get bits (PHYSMEM_ADDR_SIZE - 1):13 of physmem_base.
	srlx %l6, 13, %l5

	! l5 <= physmem_base[(PHYSMEM_ADDR_SIZE - 1):13]
	sllx %l5, 13 + (63 - (PHYSMEM_ADDR_SIZE - 1)), %l5
	srlx %l5, 63 - (PHYSMEM_ADDR_SIZE - 1), %l5

	/*
	 * Setup basic runtime environment.
	 */

	wrpr %g0, NWINDOWS - 2, %cansave	! set maximum saveable windows
	wrpr %g0, 0, %canrestore		! get rid of windows we will
						! never need again
	wrpr %g0, 0, %otherwin			! make sure the window state is
						! consistent
	wrpr %g0, NWINDOWS - 1, %cleanwin	! prevent needless clean_window
						! traps for kernel

	wrpr %g0, 0, %wstate			! use default spill/fill trap

	wrpr %g0, 0, %tl			! TL = 0, primary context
						! register is used

	wrpr %g0, PSTATE_PRIV_BIT, %pstate	! disable interrupts and disable
						! 32-bit address masking

	wrpr %g0, 0, %pil			! intialize %pil

	/*
	 * Switch to kernel trap table.
	 */
	sethi %hi(trap_table), %g1
	wrpr %g1, %lo(trap_table), %tba

	/*
	 * Take over the DMMU by installing locked TTE entry identically
	 * mapping the first 4M of memory.
	 *
	 * In case of DMMU, no FLUSH instructions need to be issued. Because of
	 * that, the old DTLB contents can be demapped pretty straightforwardly
	 * and without causing any traps.
	 */

	wr %g0, ASI_DMMU, %asi

#define SET_TLB_DEMAP_CMD(r1, context_id) \
	set (TLB_DEMAP_CONTEXT << TLB_DEMAP_TYPE_SHIFT) | (context_id << \
		TLB_DEMAP_CONTEXT_SHIFT), %r1

	! demap context 0
	SET_TLB_DEMAP_CMD(g1, TLB_DEMAP_NUCLEUS)
	stxa %g0, [%g1] ASI_DMMU_DEMAP
	membar #Sync

#define SET_TLB_TAG(r1, context) \
	set VMA | (context << TLB_TAG_ACCESS_CONTEXT_SHIFT), %r1

	! write DTLB tag
	SET_TLB_TAG(g1, MEM_CONTEXT_KERNEL)
	stxa %g1, [VA_DMMU_TAG_ACCESS] %asi
	membar #Sync

#ifdef CONFIG_VIRT_IDX_DCACHE
#define TTE_LOW_DATA(imm) 	(TTE_CP | TTE_CV | TTE_P | LMA | (imm))
#else /* CONFIG_VIRT_IDX_DCACHE */
#define TTE_LOW_DATA(imm) 	(TTE_CP | TTE_P | LMA | (imm))
#endif /* CONFIG_VIRT_IDX_DCACHE */

#define SET_TLB_DATA(r1, r2, imm) \
	set TTE_LOW_DATA(imm), %r1; \
	or %r1, %l5, %r1; \
	mov PAGESIZE_4M, %r2; \
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
	 * Because we cannot use global mappings (because we want to have
	 * separate 64-bit address spaces for both the kernel and the
	 * userspace), we prepare the identity mapping also in context 1. This
	 * step is required by the code installing the ITLB mapping.
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
	 * Now is time to take over the IMMU. Unfortunatelly, it cannot be done
	 * as easily as the DMMU, because the IMMU is mapping the code it
	 * executes.
	 *
	 * [ Note that brave experiments with disabling the IMMU and using the
	 * DMMU approach failed after a dozen of desparate days with only little
	 * success. ]
	 *
	 * The approach used here is inspired from OpenBSD. First, the kernel
	 * creates IMMU mapping for itself in context 1 (MEM_CONTEXT_TEMP) and
	 * switches to it. Context 0 (MEM_CONTEXT_KERNEL) can be demapped
	 * afterwards and replaced with the kernel permanent mapping. Finally,
	 * the kernel switches back to context 0 and demaps context 1.
	 *
	 * Moreover, the IMMU requires use of the FLUSH instructions. But that
	 * is OK because we always use operands with addresses already mapped by
	 * the taken over DTLB.
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

	/*
	 * Save physmem_base for use by the mm subsystem.
	 * %l6 contains starting physical address
	 */
	sethi %hi(physmem_base), %l4
	stx %l6, [%l4 + %lo(physmem_base)]

	/*
	 * Precompute kernel 8K TLB data template.
	 * %l5 contains starting physical address
	 * bits [(PHYSMEM_ADDR_SIZE - 1):13]
	 */
	sethi %hi(kernel_8k_tlb_data_template), %l4
	ldx [%l4 + %lo(kernel_8k_tlb_data_template)], %l3
	or %l3, %l5, %l3
	stx %l3, [%l4 + %lo(kernel_8k_tlb_data_template)]

	! flush the whole D-cache
	set (DCACHE_SIZE - DCACHE_LINE_SIZE), %g1
	stxa %g0, [%g1] ASI_DCACHE_TAG

0:
	membar #Sync
	subcc %g1, DCACHE_LINE_SIZE, %g1
	bnz,pt %xcc, 0b
	stxa %g0, [%g1] ASI_DCACHE_TAG
	membar #Sync

	/*
	 * So far, we have not touched the stack.
	 * It is a good idea to set the kernel stack to a known state now.
	 */
	sethi %hi(temporary_boot_stack), %sp
	or %sp, %lo(temporary_boot_stack), %sp
	sub %sp, STACK_BIAS, %sp

	/*
	 * Call sparc64_pre_main(bootinfo)
	 */
	call sparc64_pre_main
	mov %o1, %o0

	/*
	 * Create the first stack frame.
	 */
	save %sp, -(STACK_WINDOW_SAVE_AREA_SIZE + STACK_ARG_SAVE_AREA_SIZE), %sp
	flushw
	add %g0, -STACK_BIAS, %fp

	call main_bsp
	nop

	/* Not reached. */

0:
	ba,a %xcc, 0b


1:
#ifdef CONFIG_SMP
	/*
	 * Determine the width of the MID and save its mask to %g3. The width
	 * is
	 * 	* 5 for US and US-IIIi,
	 * 	* 10 for US3 except US-IIIi.
	 */
#if defined(US)
	mov 0x1f, %g3
#elif defined(US3)
	mov 0x3ff, %g3
	rdpr %ver, %g2
	sllx %g2, 16, %g2
	srlx %g2, 48, %g2
	cmp %g2, IMPL_ULTRASPARCIII_I
	move %xcc, 0x1f, %g3
#endif

	/*
	 * Read MID from the processor.
	 */
	ldxa [%g0] ASI_ICBUS_CONFIG, %g1
	srlx %g1, ICBUS_CONFIG_MID_SHIFT, %g1
	and %g1, %g3, %g1

	/*
	 * Active loop for APs until the BSP picks them up. A processor cannot
	 * leave the loop until the global variable 'waking_up_mid' equals its
	 * MID.
	 */
	set waking_up_mid, %g2
2:
	ldx [%g2], %g3
	cmp %g3, %g1
	bne %xcc, 2b
	nop

	/*
	 * Configure stack for the AP.
	 * The AP is expected to use the stack saved
	 * in the ctx global variable.
	 */
	set ctx, %g1
	add %g1, CONTEXT_OFFSET_SP, %g1
	ldx [%g1], %o6

	/*
	 * Create the first stack frame.
	 */
	save %sp, -(STACK_WINDOW_SAVE_AREA_SIZE + STACK_ARG_SAVE_AREA_SIZE), %sp
	flushw
	add %g0, -STACK_BIAS, %fp

	call main_ap
	nop

	/* Not reached. */
#endif

0:
	ba,a %xcc, 0b


.section K_DATA_START, "aw", @progbits

/*
 * Create small stack to be used by the bootstrap processor. It is going to be
 * used only for a very limited period of time, but we switch to it anyway,
 * just to be sure we are properly initialized.
 */

#define INITIAL_STACK_SIZE	1024

.align STACK_ALIGNMENT
	.space INITIAL_STACK_SIZE
.align STACK_ALIGNMENT
temporary_boot_stack:
	.space STACK_WINDOW_SAVE_AREA_SIZE


.data

.align 8
SYMBOL(physmem_base)	! copy of the physical memory base address
	.quad 0

/*
 * The fast_data_access_mmu_miss_data_hi label, the end_of_identity,
 * kernel_8k_tlb_data_template and tlb_tag_access_context_mask variables
 * are meant to stay together, aligned on a 32B boundary.
 */

.align 32
/*
 * This label is used by the fast_data_access_MMU_miss trap handler.
 */
SYMBOL(fast_data_access_mmu_miss_data_hi)
/*
 * This variable is used by the fast_data_access_MMU_miss trap handler.
 * In runtime, it is modified to contain the address of the end of physical
 * memory.
 */
SYMBOL(end_of_identity)
	.quad -1
/*
 * This variable is used by the fast_data_access_MMU_miss trap handler.
 * In runtime, it is further modified to reflect the starting address of
 * physical memory.
 */
SYMBOL(kernel_8k_tlb_data_template)
#ifdef CONFIG_VIRT_IDX_DCACHE
	.quad ((1 << TTE_V_SHIFT) | (PAGESIZE_8K << TTE_SIZE_SHIFT) | TTE_CP | \
		 TTE_CV | TTE_P | TTE_W)
#else /* CONFIG_VIRT_IDX_DCACHE */
	.quad ((1 << TTE_V_SHIFT) | (PAGESIZE_8K << TTE_SIZE_SHIFT) | TTE_CP | \
		TTE_P | TTE_W)
#endif /* CONFIG_VIRT_IDX_DCACHE */

/*
 * This variable is used by the fast_data_access_MMU_miss trap handler.
 * It allows us to save one precious instruction slot of this handler.
 */
SYMBOL(tlb_tag_access_context_mask)
	.quad TLB_TAG_ACCESS_CONTEXT_MASK

