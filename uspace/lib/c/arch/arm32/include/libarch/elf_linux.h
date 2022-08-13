/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcarm32
 * @{
 */
/** @file Definitions needed to write core files in Linux-ELF format.
 */

#ifndef _LIBC_arm32_ELF_LINUX_H_
#define _LIBC_arm32_ELF_LINUX_H_

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
	uint32_t fp;
	uint32_t r12;
	uint32_t sp;
	uint32_t lr;
	uint32_t pc;
	uint32_t cpsr;
	uint32_t old_r0;
} elf_regs_t;

/** Convert istate_t to elf_regs_t. */
static inline void istate_to_elf_regs(istate_t *istate, elf_regs_t *elf_regs)
{
	elf_regs->r0 = istate->r0;
	elf_regs->r1 = istate->r1;
	elf_regs->r2 = istate->r2;
	elf_regs->r3 = istate->r3;
	elf_regs->r4 = istate->r4;
	elf_regs->r5 = istate->r5;
	elf_regs->r6 = istate->r6;
	elf_regs->r7 = istate->r7;
	elf_regs->r8 = istate->r8;
	elf_regs->r9 = istate->r9;
	elf_regs->r10 = istate->r10;

	elf_regs->fp = istate->fp;
	elf_regs->r12 = istate->r12;
	elf_regs->sp = istate->sp;
	elf_regs->lr = istate->lr;
	elf_regs->pc = istate->pc;
	elf_regs->cpsr = istate->spsr;
	elf_regs->old_r0 = 0;
}

#endif

/** @}
 */
