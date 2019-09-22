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

/** @addtogroup libcarm64
 * @{
 */
/** @file Definitions needed to write core files in Linux-ELF format.
 */

#ifndef _LIBC_arm64_ELF_LINUX_H_
#define _LIBC_arm64_ELF_LINUX_H_

#include <libarch/istate.h>
#include <stdint.h>

/** Linux kernel struct pt_regs structure.
 *
 * We need this to save register state to a core file in Linux format
 * (readable by GDB configured for Linux target).
 */
typedef struct {
	uint64_t x0;
	uint64_t x1;
	uint64_t x2;
	uint64_t x3;
	uint64_t x4;
	uint64_t x5;
	uint64_t x6;
	uint64_t x7;
	uint64_t x8;
	uint64_t x9;
	uint64_t x10;
	uint64_t x11;
	uint64_t x12;
	uint64_t x13;
	uint64_t x14;
	uint64_t x15;
	uint64_t x16;
	uint64_t x17;
	uint64_t x18;
	uint64_t x19;
	uint64_t x20;
	uint64_t x21;
	uint64_t x22;
	uint64_t x23;
	uint64_t x24;
	uint64_t x25;
	uint64_t x26;
	uint64_t x27;
	uint64_t x28;
	uint64_t x29;
	uint64_t x30;

	uint64_t sp;
	uint64_t pc;
	uint64_t pstate;

	uint64_t orig_x0;
	uint64_t syscallno;
} elf_regs_t;

/** Convert istate_t to elf_regs_t. */
static inline void istate_to_elf_regs(istate_t *istate, elf_regs_t *elf_regs)
{
	elf_regs->x0 = istate->x0;
	elf_regs->x1 = istate->x1;
	elf_regs->x2 = istate->x2;
	elf_regs->x3 = istate->x3;
	elf_regs->x4 = istate->x4;
	elf_regs->x5 = istate->x5;
	elf_regs->x6 = istate->x6;
	elf_regs->x7 = istate->x7;
	elf_regs->x8 = istate->x8;
	elf_regs->x9 = istate->x9;
	elf_regs->x10 = istate->x10;
	elf_regs->x11 = istate->x11;
	elf_regs->x12 = istate->x12;
	elf_regs->x13 = istate->x13;
	elf_regs->x14 = istate->x14;
	elf_regs->x15 = istate->x15;
	elf_regs->x16 = istate->x16;
	elf_regs->x17 = istate->x17;
	elf_regs->x18 = istate->x18;
	elf_regs->x19 = istate->x19;
	elf_regs->x20 = istate->x20;
	elf_regs->x21 = istate->x21;
	elf_regs->x22 = istate->x22;
	elf_regs->x23 = istate->x23;
	elf_regs->x24 = istate->x24;
	elf_regs->x25 = istate->x25;
	elf_regs->x26 = istate->x26;
	elf_regs->x27 = istate->x27;
	elf_regs->x28 = istate->x28;
	elf_regs->x29 = istate->x29;
	elf_regs->x30 = istate->x30;

	elf_regs->sp = istate->sp;
	elf_regs->pc = istate->pc;
	elf_regs->pstate = istate->spsr;

	elf_regs->orig_x0 = 0;
	elf_regs->syscallno = 0;
}

#endif

/** @}
 */
