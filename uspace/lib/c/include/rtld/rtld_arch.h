/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_RTLD_RTLD_ARCH_H_
#define _LIBC_RTLD_RTLD_ARCH_H_

#include <rtld/rtld.h>
#include <loader/pcb.h>

void module_process_pre_arch(module_t *m);

void rel_table_process(module_t *m, elf_rel_t *rt, size_t rt_size);
void rela_table_process(module_t *m, elf_rela_t *rt, size_t rt_size);
void *func_get_addr(elf_symbol_t *, module_t *);

void program_run(void *entry, pcb_t *pcb);

#endif

/** @}
 */
