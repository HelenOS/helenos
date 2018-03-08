/*
 * Copyright (c) 2006 Jakub Jermar
 * Copyright (c) 2008 Pavel Rimsky
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

#ifndef KERN_sparc64_sun4v_MMU_TRAP_H_
#define KERN_sparc64_sun4v_MMU_TRAP_H_

#include <arch/stack.h>
#include <arch/regdef.h>
#include <arch/arch.h>
#include <arch/sun4v/arch.h>
#include <arch/sun4v/hypercall.h>
#include <arch/mm/sun4v/mmu.h>
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
#define TT_CPU_MONDO				0x7c

#define FAST_MMU_HANDLER_SIZE			128

#ifdef __ASSEMBLER__

/* MMU fault status area data fault offset */
#define FSA_DFA_OFFSET				0x48

/* MMU fault status area data context */
#define FSA_DFC_OFFSET				0x50

/* offset of the target address within the TTE Data entry */
#define TTE_DATA_TADDR_OFFSET			13

.macro FAST_INSTRUCTION_ACCESS_MMU_MISS_HANDLER
	mov TT_FAST_INSTRUCTION_ACCESS_MMU_MISS, %g2
	clr %g5		! XXX
	PREEMPTIBLE_HANDLER exc_dispatch
.endm

/*
 * Handler of the Fast Data Access MMU Miss trap. If the trap occurred in the kernel
 * (context 0), an identity mapping (with displacement) is installed. Otherwise
 * a higher level service routine is called.
 */
.macro FAST_DATA_ACCESS_MMU_MISS_HANDLER tl
	mov SCRATCHPAD_MMU_FSA, %g1
	ldxa [%g1] ASI_SCRATCHPAD, %g1			! g1 <= RA of MMU fault status area

	/* read faulting context */
	add %g1, FSA_DFC_OFFSET, %g2			! g2 <= RA of data fault context
	ldxa [%g2] ASI_REAL, %g3			! read the fault context

	/* read the faulting address */
	add %g1, FSA_DFA_OFFSET, %g2			! g2 <= RA of data fault address
	ldxa [%g2] ASI_REAL, %g1			! read the fault address
	srlx %g1, TTE_DATA_TADDR_OFFSET, %g1		! truncate it to page boundary
	sllx %g1, TTE_DATA_TADDR_OFFSET, %g1

	/* service by higher-level routine when context != 0 */
	brnz %g3, 0f
	nop
	/* exclude page number 0 from installing the identity mapping */
	brz %g1, 0f
	nop

	/* exclude pages beyond the end of memory from the identity mapping */
	sethi %hi(end_of_identity), %g4
	ldx [%g4 + %lo(end_of_identity)], %g4
	cmp %g1, %g4
	bgeu %xcc, 0f
	nop

	/*
	 * Installing the identity does not fit into 32 instructions, call
	 * a separate routine. The routine performs RETRY, hence the call never
	 * returns.
	 */
	ba,a %xcc, install_identity_mapping

0:

	/*
	 * One of the scenarios in which this trap can occur is when the
	 * register window spill/fill handler accesses a memory which is not
	 * mapped. In such a case, this handler will be called from TL = 1.
	 * We handle the situation by pretending that the MMU miss occurred
	 * on TL = 0. Once the MMU miss trap is serviced, the instruction which
	 * caused the spill/fill trap is restarted, the spill/fill trap occurs,
	 * but this time its handler accesses memory which is mapped.
	 */
	.if (\tl > 0)
		wrpr %g0, 1, %tl
	.endif

	mov TT_FAST_DATA_ACCESS_MMU_MISS, %g2

	/*
	 * Save the faulting virtual page and faulting context to the %g5
	 * register. The most significant 51 bits of the %g5 register will
	 * contain the virtual address which caused the fault truncated to the
	 * page boundary. The least significant 13 bits of the %g5 register
	 * will contain the number of the context in which the fault occurred.
	 * The value of the %g5 register will be stored in the istate structure
	 * for inspeciton by the higher level service routine.
	 */
	or %g1, %g3, %g5

	PREEMPTIBLE_HANDLER exc_dispatch
.endm

/*
 * Handler of the Fast Data MMU Protection trap. Finds the trapping address
 * and context and calls higher level service routine.
 */
.macro FAST_DATA_ACCESS_PROTECTION_HANDLER tl
	/*
	 * The same special case as in FAST_DATA_ACCESS_MMU_MISS_HANDLER.
	 */
	.if (\tl > 0)
		wrpr %g0, 1, %tl
	.endif

	mov SCRATCHPAD_MMU_FSA, %g1
	ldxa [%g1] ASI_SCRATCHPAD, %g1			! g1 <= RA of MMU fault status area

	/* read faulting context */
	add %g1, FSA_DFC_OFFSET, %g2			! g2 <= RA of data fault context
	ldxa [%g2] ASI_REAL, %g3			! read the fault context

	/* read the faulting address */
	add %g1, FSA_DFA_OFFSET, %g2			! g2 <= RA of data fault address
	ldxa [%g2] ASI_REAL, %g1			! read the fault address
	srlx %g1, TTE_DATA_TADDR_OFFSET, %g1		! truncate it to page boundary
	sllx %g1, TTE_DATA_TADDR_OFFSET, %g1

	mov TT_FAST_DATA_ACCESS_PROTECTION, %g2

	/* the same as for FAST_DATA_ACCESS_MMU_MISS_HANDLER */
	or %g1, %g3, %g5

	PREEMPTIBLE_HANDLER exc_dispatch
.endm
#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
