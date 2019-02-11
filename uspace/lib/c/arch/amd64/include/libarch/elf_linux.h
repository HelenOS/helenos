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

/** @addtogroup libcamd64
 * @{
 */
/** @file Definitions needed to write core files in Linux-ELF format.
 */

#ifndef _LIBC_amd64_ELF_LINUX_H_
#define _LIBC_amd64_ELF_LINUX_H_

#include <libarch/istate.h>
#include <stdint.h>

/** Linux kernel struct pt_regs structure.
 *
 * We need this to save register state to a core file in Linux format
 * (readable by GDB configured for Linux target).
 */
typedef struct {
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t rbp;
	uint64_t rbx;
	uint64_t r11;
	uint64_t r10;
	uint64_t r9;
	uint64_t r8;
	uint64_t rax;
	uint64_t rcx;
	uint64_t rdx;
	uint64_t rsi;
	uint64_t rdi;
	uint64_t old_rax;
	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	uint64_t rsp;
	uint64_t ss;

	/*
	 * The following registers need to be part of elf_regs_t.
	 * Unfortunately, we don't have any information about them in our
	 * istate_t.
	 */
	uint64_t unused_fs_base;
	uint64_t unused_gs_base;
	uint64_t unused_ds;
	uint64_t unused_es;
	uint64_t unused_fs;
	uint64_t unused_gs;
} elf_regs_t;

/** Convert istate_t to elf_regs_t. */
static inline void istate_to_elf_regs(istate_t *istate, elf_regs_t *elf_regs)
{
	elf_regs->r15 = istate->r15;
	elf_regs->r14 = istate->r14;
	elf_regs->r13 = istate->r13;
	elf_regs->r12 = istate->r12;
	elf_regs->rbp = istate->rbp;
	elf_regs->rbx = istate->rbx;
	elf_regs->r11 = istate->r11;
	elf_regs->r10 = istate->r10;
	elf_regs->r9 = istate->r9;
	elf_regs->r8 = istate->r8;
	elf_regs->rax = istate->rax;
	elf_regs->rcx = istate->rcx;
	elf_regs->rdx = istate->rdx;
	elf_regs->rsi = istate->rsi;
	elf_regs->rdi = istate->rdi;
	elf_regs->rip = istate->rip;
	elf_regs->cs = istate->cs;
	elf_regs->rflags = istate->rflags;
	elf_regs->rsp = istate->rsp;
	elf_regs->ss = istate->ss;

	/*
	 * Reset the registers for which there is not enough info in istate_t.
	 */
	elf_regs->unused_fs_base = 0;
	elf_regs->unused_gs_base = 0;
	elf_regs->unused_ds = 0;
	elf_regs->unused_es = 0;
	elf_regs->unused_fs = 0;
	elf_regs->unused_gs = 0;
}

#endif

/** @}
 */
