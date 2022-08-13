/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcsparc64
 * @{
 */
/** @file
 */

#ifndef _LIBC_sparc64_ELF_LINUX_H_
#define _LIBC_sparc64_ELF_LINUX_H_

#include <libarch/istate.h>
#include <stdint.h>

typedef struct {
	/* TODO */
	uint64_t pad[16];
} elf_regs_t;

static inline void istate_to_elf_regs(istate_t *istate, elf_regs_t *elf_regs)
{
	/* TODO */
	(void) istate;
	(void) elf_regs;
}

#endif

/** @}
 */
