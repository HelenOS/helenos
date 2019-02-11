/*
 * Copyright (c) 2011 Jiri Svoboda
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

/** @addtogroup libcppc32
 * @{
 */
/** @file Definitions needed to write core files in Linux-ELF format.
 */

#ifndef _LIBC_ppc32_ELF_LINUX_H_
#define _LIBC_ppc32_ELF_LINUX_H_

#include <libarch/istate.h>
#include <stdint.h>

/** Linux kernel struct pt_regs structure.
 *
 * We need this to save register state to a core file in Linux format
 * (readable by GDB configured for Linux target).
 */
typedef struct {
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
	uint32_t r12;
	uint32_t r13;
	uint32_t r14;
	uint32_t r15;
	uint32_t r16;
	uint32_t r17;
	uint32_t r18;
	uint32_t r19;
	uint32_t r20;
	uint32_t r21;
	uint32_t r22;
	uint32_t r23;
	uint32_t r24;
	uint32_t r25;
	uint32_t r26;
	uint32_t r27;
	uint32_t r28;
	uint32_t r29;
	uint32_t r30;
	uint32_t r31;

	uint32_t nip;
	uint32_t msr;
	uint32_t old_r3;
	uint32_t ctr;
	uint32_t link;
	uint32_t xer;
	uint32_t ccr;
	uint32_t mq;
	uint32_t trap;
	uint32_t dar;
	uint32_t dsisr;
	uint32_t result;
} elf_regs_t;

/** Convert istate_t to elf_regs_t. */
static inline void istate_to_elf_regs(istate_t *istate, elf_regs_t *elf_regs)
{
	elf_regs->r0 = istate->r0;
	elf_regs->r1 = istate->sp;
	elf_regs->r2 = istate->r2;
	elf_regs->r3 = istate->r3;
	elf_regs->r4 = istate->r4;
	elf_regs->r5 = istate->r5;
	elf_regs->r6 = istate->r6;
	elf_regs->r7 = istate->r7;
	elf_regs->r8 = istate->r8;
	elf_regs->r9 = istate->r9;
	elf_regs->r10 = istate->r10;
	elf_regs->r11 = istate->r11;
	elf_regs->r12 = istate->r12;
	elf_regs->r13 = istate->r13;
	elf_regs->r14 = istate->r14;
	elf_regs->r15 = istate->r15;
	elf_regs->r16 = istate->r16;
	elf_regs->r17 = istate->r17;
	elf_regs->r18 = istate->r18;
	elf_regs->r19 = istate->r19;
	elf_regs->r20 = istate->r20;
	elf_regs->r21 = istate->r21;
	elf_regs->r22 = istate->r22;
	elf_regs->r23 = istate->r23;
	elf_regs->r24 = istate->r24;
	elf_regs->r25 = istate->r25;
	elf_regs->r26 = istate->r26;
	elf_regs->r27 = istate->r27;
	elf_regs->r28 = istate->r28;
	elf_regs->r29 = istate->r29;
	elf_regs->r30 = istate->r30;
	elf_regs->r31 = istate->r31;

	elf_regs->ctr = istate->ctr;
	elf_regs->link = istate->lr;
	elf_regs->xer = istate->xer;
	elf_regs->ccr = istate->cr;
	elf_regs->dar = istate->dar;
}

#endif

/** @}
 */
