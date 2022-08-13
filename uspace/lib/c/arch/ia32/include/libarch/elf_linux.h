/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcia32
 * @{
 */
/** @file Definitions needed to write core files in Linux-ELF format.
 */

#ifndef _LIBC_ia32_ELF_LINUX_H_
#define _LIBC_ia32_ELF_LINUX_H_

#include <libarch/istate.h>
#include <stdint.h>

/** Linux kernel struct pt_regs structure.
 *
 * We need this to save register state to a core file in Linux format
 * (readable by GDB configured for Linux target).
 */
typedef struct {
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
	uint32_t esi;
	uint32_t edi;
	uint32_t ebp;
	uint32_t eax;
	uint32_t ds;
	uint32_t es;
	uint32_t fs;
	uint32_t gs;
	uint32_t old_eax;
	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;
	uint32_t esp;
	uint32_t ss;
} elf_regs_t;

/** Convert istate_t to elf_regs_t. */
static inline void istate_to_elf_regs(istate_t *istate, elf_regs_t *elf_regs)
{
	elf_regs->ebx = istate->ebx;
	elf_regs->ecx = istate->ecx;
	elf_regs->edx = istate->edx;
	elf_regs->esi = istate->esi;
	elf_regs->edi = istate->edi;
	elf_regs->ebp = istate->ebp;
	elf_regs->eax = istate->eax;

	elf_regs->ds = istate->ds;
	elf_regs->es = istate->es;
	elf_regs->fs = istate->fs;
	elf_regs->gs = istate->gs;

	elf_regs->old_eax = 0;
	elf_regs->eip = istate->eip;
	elf_regs->cs = istate->cs;
	elf_regs->eflags = istate->eflags;
	elf_regs->esp = istate->esp;
	elf_regs->ss = istate->ss;
}

#endif

/** @}
 */
