/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcabs32le
 * @{
 */
/** @file
 */

#ifndef _LIBC_abs32le_ELF_LINUX_H_
#define _LIBC_abs32le_ELF_LINUX_H_

#include <libarch/istate.h>

typedef struct {
} elf_regs_t;

static inline void istate_to_elf_regs(istate_t *istate, elf_regs_t *elf_regs)
{
	(void) istate;
	(void) elf_regs;
}

#endif

/** @}
 */
