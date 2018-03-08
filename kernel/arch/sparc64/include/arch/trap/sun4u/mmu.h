/*
 * Copyright (c) 2006 Jakub Jermar
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

/** @addtogroup sparc64interrupt
 * @{
 */
/**
 * @file
 * @brief This file contains fast MMU trap handlers.
 */

#ifndef KERN_sparc64_SUN4U_MMU_TRAP_H_
#define KERN_sparc64_SUN4U_MMU_TRAP_H_

#include <arch/stack.h>
#include <arch/regdef.h>
#include <arch/mm/tlb.h>
#include <arch/mm/mmu.h>
#include <arch/mm/tte.h>
#include <arch/trap/regwin.h>

#ifdef CONFIG_TSB
#include <arch/mm/tsb.h>
#endif

#define TT_FAST_INSTRUCTION_ACCESS_MMU_MISS	0x64
#define TT_FAST_DATA_ACCESS_MMU_MISS		0x68
#define TT_FAST_DATA_ACCESS_PROTECTION		0x6c

#define FAST_MMU_HANDLER_SIZE			128

#ifdef __ASSEMBLER__

.macro FAST_INSTRUCTION_ACCESS_MMU_MISS_HANDLER
	/*
	 * First, try to refill TLB from TSB.
	 */
#ifdef CONFIG_TSB
	ldxa [%g0] ASI_IMMU, %g1			! read TSB Tag Target Register
	ldxa [%g0] ASI_IMMU_TSB_8KB_PTR_REG, %g2	! read TSB 8K Pointer
	ldda [%g2] ASI_NUCLEUS_QUAD_LDD, %g4		! 16-byte atomic load into %g4 and %g5
	cmp %g1, %g4					! is this the entry we are looking for?
	bne,pn %xcc, 0f
	nop
	stxa %g5, [%g0] ASI_ITLB_DATA_IN_REG		! copy mapping from ITSB to ITLB
	retry
#endif

0:
	wrpr %g0, PSTATE_PRIV_BIT | PSTATE_AG_BIT, %pstate
	mov TT_FAST_INSTRUCTION_ACCESS_MMU_MISS, %g2
	mov VA_IMMU_TAG_ACCESS, %g5
	ldxa [%g5] ASI_IMMU, %g5			! read the faulting Context and VPN
	PREEMPTIBLE_HANDLER exc_dispatch
.endm

.macro FAST_DATA_ACCESS_MMU_MISS_HANDLER tl
	/*
	 * First, try to refill TLB from TSB.
	 */

#ifdef CONFIG_TSB
	ldxa [%g0] ASI_DMMU, %g1			! read TSB Tag Target Register
	srlx %g1, TSB_TAG_TARGET_CONTEXT_SHIFT, %g2	! is this a kernel miss?
	brz,pn %g2, 0f
	ldxa [%g0] ASI_DMMU_TSB_8KB_PTR_REG, %g3	! read TSB 8K Pointer
	ldda [%g3] ASI_NUCLEUS_QUAD_LDD, %g4		! 16-byte atomic load into %g4 and %g5
	cmp %g1, %g4					! is this the entry we are looking for?
	bne,pn %xcc, 0f
	nop
	stxa %g5, [%g0] ASI_DTLB_DATA_IN_REG		! copy mapping from DTSB to DTLB
	retry
#endif

	/*
	 * Second, test if it is the portion of the kernel address space
	 * which is faulting. If that is the case, immediately create
	 * identity mapping for that page in DTLB. VPN 0 is excluded from
	 * this treatment.
	 *
	 * Note that branch-delay slots are used in order to save space.
	 */
0:
	sethi %hi(fast_data_access_mmu_miss_data_hi), %g7
	wr %g0, ASI_DMMU, %asi
	ldxa [VA_DMMU_TAG_ACCESS] %asi, %g1		! read the faulting Context and VPN
	ldx [%g7 + %lo(tlb_tag_access_context_mask)], %g2
	andcc %g1, %g2, %g3				! get Context
	bnz %xcc, 0f					! Context is non-zero
	andncc %g1, %g2, %g3				! get page address into %g3
	bz  %xcc, 0f					! page address is zero
	ldx [%g7 + %lo(end_of_identity)], %g4
	cmp %g3, %g4
	bgeu %xcc, 0f

	ldx [%g7 + %lo(kernel_8k_tlb_data_template)], %g2
	add %g3, %g2, %g2
	stxa %g2, [%g0] ASI_DTLB_DATA_IN_REG		! identity map the kernel page
	retry

	/*
	 * Third, catch and handle special cases when the trap is caused by
	 * the userspace register window spill or fill handler. In case
	 * one of these two traps caused this trap, we just lower the trap
	 * level and service the DTLB miss. In the end, we restart
	 * the offending SAVE or RESTORE.
	 */
0:
.if (\tl > 0)
	wrpr %g0, 1, %tl
.endif

	/*
	 * Switch from the MM globals.
	 */
	wrpr %g0, PSTATE_PRIV_BIT | PSTATE_AG_BIT, %pstate

	mov TT_FAST_DATA_ACCESS_MMU_MISS, %g2
	ldxa [VA_DMMU_TAG_ACCESS] %asi, %g5		! read the faulting Context and VPN
	PREEMPTIBLE_HANDLER exc_dispatch
.endm

.macro FAST_DATA_ACCESS_PROTECTION_HANDLER tl
	/*
	 * The same special case as in FAST_DATA_ACCESS_MMU_MISS_HANDLER.
	 */

.if (\tl > 0)
	wrpr %g0, 1, %tl
.endif

	/*
	 * Switch from the MM globals.
	 */
	wrpr %g0, PSTATE_PRIV_BIT | PSTATE_AG_BIT, %pstate

	mov TT_FAST_DATA_ACCESS_PROTECTION, %g2
	mov VA_DMMU_TAG_ACCESS, %g5
	ldxa [%g5] ASI_DMMU, %g5			! read the faulting Context and VPN
	PREEMPTIBLE_HANDLER exc_dispatch
.endm

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
