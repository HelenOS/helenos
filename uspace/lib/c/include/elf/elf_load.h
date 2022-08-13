/*
 * SPDX-FileCopyrightText: 2016 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 * @brief
 */

#ifndef ELF_LOAD_H_
#define ELF_LOAD_H_

#include <elf/elf_mod.h>

/** Information on loaded ELF program */
typedef struct {
	elf_finfo_t finfo;
	struct rtld *env;
} elf_info_t;

extern errno_t elf_load(int, elf_info_t *);
extern void elf_set_pcb(elf_info_t *, pcb_t *);

#endif

/** @}
 */
