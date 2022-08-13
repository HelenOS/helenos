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

#ifndef _LIBC_RTLD_SYMBOL_H_
#define _LIBC_RTLD_SYMBOL_H_

#include <elf/elf.h>
#include <rtld/rtld.h>
#include <tls.h>

/** Symbol search flags */
typedef enum {
	/** No flags */
	ssf_none = 0,
	/** Do not search in the executable */
	ssf_noexec = 0x1
} symbol_search_flags_t;

extern elf_symbol_t *symbol_bfs_find(const char *, module_t *, module_t **);
extern elf_symbol_t *symbol_def_find(const char *, module_t *,
    symbol_search_flags_t, module_t **);
extern void *symbol_get_addr(elf_symbol_t *, module_t *, tcb_t *);

#endif

/** @}
 */
