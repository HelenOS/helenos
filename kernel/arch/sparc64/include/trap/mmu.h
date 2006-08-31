/*
 * Copyright (C) 2006 Jakub Jermar
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

#ifndef KERN_sparc64_MMU_TRAP_H_
#define KERN_sparc64_MMU_TRAP_H_

#include <arch/stack.h>
#include <arch/regdef.h>
#include <arch/mm/tlb.h>
#include <arch/mm/mmu.h>
#include <arch/mm/tte.h>

#define TT_FAST_INSTRUCTION_ACCESS_MMU_MISS	0x64
#define TT_FAST_DATA_ACCESS_MMU_MISS		0x68
#define TT_FAST_DATA_ACCESS_PROTECTION		0x6c

#define FAST_MMU_HANDLER_SIZE			128

#ifdef __ASM__
.macro FAST_INSTRUCTION_ACCESS_MMU_MISS_HANDLER
	/*
	 * First, try to refill TLB from TSB.
	 */
	! TODO

	wrpr %g0, PSTATE_PRIV_BIT | PSTATE_AG_BIT, %pstate
	PREEMPTIBLE_HANDLER fast_instruction_access_mmu_miss
.endm

.macro FAST_DATA_ACCESS_MMU_MISS_HANDLER
	/*
	 * First, try to refill TLB from TSB.
	 */
	! TODO

	/*
	 * Second, test if it is the portion of the kernel address space
	 * which is faulting. If that is the case, immediately create
	 * identity mapping for that page in DTLB. VPN 0 is excluded from
	 * this treatment.
	 *
	 * Note that branch-delay slots are used in order to save space.
	 */
0:
	mov VA_DMMU_TAG_ACCESS, %g1
	ldxa [%g1] ASI_DMMU, %g1			! read the faulting Context and VPN
	set TLB_TAG_ACCESS_CONTEXT_MASK, %g2
	andcc %g1, %g2, %g3				! get Context
	bnz 0f						! Context is non-zero
	andncc %g1, %g2, %g3				! get page address into %g3
	bz 0f						! page address is zero

	or %g3, (TTE_CP|TTE_P|TTE_W), %g2		! 8K pages are the default (encoded as 0)
        set 1, %g3
        sllx %g3, TTE_V_SHIFT, %g3
        or %g2, %g3, %g2
	stxa %g2, [%g0] ASI_DTLB_DATA_IN_REG		! identity map the kernel page
	retry

	/*
	 * Third, catch and handle special cases when the trap is caused by
	 * some register window trap handler.
	 */
0:
	! TODO

0:
	wrpr %g0, PSTATE_PRIV_BIT | PSTATE_AG_BIT, %pstate
	PREEMPTIBLE_HANDLER fast_data_access_mmu_miss
.endm

.macro FAST_DATA_ACCESS_PROTECTION_HANDLER
	wrpr %g0, PSTATE_PRIV_BIT | PSTATE_AG_BIT, %pstate
	PREEMPTIBLE_HANDLER fast_data_access_protection
.endm
#endif /* __ASM__ */

#endif

/** @}
 */
