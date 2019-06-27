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

/** @addtogroup libcmips32
 * @{
 */
/** @file Definitions needed to write core files in Linux-ELF format.
 */

#ifndef _LIBC_mips32_ELF_LINUX_H_
#define _LIBC_mips32_ELF_LINUX_H_

#include <libarch/istate.h>
#include <stdint.h>

/** Linux kernel struct pt_regs structure.
 *
 * We need this to save register state to a core file in Linux format
 * (readable by GDB configured for Linux target).
 */
typedef struct {
	uint32_t pad0[6];

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

	uint32_t cp0_status;
	uint32_t hi;
	uint32_t lo;
	uint32_t cp0_badvaddr;
	uint32_t cp0_cause;
	uint32_t cp0_epc;
} elf_regs_t;

/** Convert istate_t to elf_regs_t. */
static inline void istate_to_elf_regs(istate_t *istate, elf_regs_t *elf_regs)
{
	elf_regs->r1 = istate->at;
	elf_regs->r2 = istate->v0;
	elf_regs->r3 = istate->v1;
	elf_regs->r4 = istate->a0;
	elf_regs->r5 = istate->a1;
	elf_regs->r6 = istate->a2;
	elf_regs->r7 = istate->a3;
	elf_regs->r8 = istate->t0;
	elf_regs->r9 = istate->t1;
	elf_regs->r10 = istate->t2;
	elf_regs->r11 = istate->t3;
	elf_regs->r12 = istate->t4;
	elf_regs->r13 = istate->t5;
	elf_regs->r14 = istate->t6;
	elf_regs->r15 = istate->t7;
	elf_regs->r16 = istate->s0;
	elf_regs->r17 = istate->s1;
	elf_regs->r18 = istate->s2;
	elf_regs->r19 = istate->s3;
	elf_regs->r20 = istate->s4;
	elf_regs->r21 = istate->s5;
	elf_regs->r22 = istate->s6;
	elf_regs->r23 = istate->s7;
	elf_regs->r24 = istate->t8;
	elf_regs->r25 = istate->t9;
	elf_regs->r26 = istate->kt0;
	elf_regs->r27 = istate->kt1;
	elf_regs->r28 = istate->gp;
	elf_regs->r29 = istate->sp;
	elf_regs->r30 = istate->s8;
	elf_regs->r31 = istate->ra;

	elf_regs->cp0_status = istate->status;
	elf_regs->hi = istate->hi;
	elf_regs->lo = istate->lo;
	elf_regs->cp0_epc = istate->epc;
}

#endif

/** @}
 */
