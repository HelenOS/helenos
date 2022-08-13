/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup debug
 * @{
 */
/** @file
 */

#ifndef SYMTAB_H_
#define SYMTAB_H_

#include <elf/elf.h>
#include <stddef.h>

typedef struct {
	/** Symbol section */
	elf_symbol_t *sym;
	size_t sym_size;
	/** String table */
	char *strtab;
	size_t strtab_size;
} symtab_t;

extern errno_t symtab_load(const char *file_name, symtab_t **symtab);
extern void symtab_delete(symtab_t *st);
extern errno_t symtab_name_to_addr(symtab_t *st, const char *name, uintptr_t *addr);
extern errno_t symtab_addr_to_name(symtab_t *symtab, uintptr_t addr, char **name,
    size_t *offs);

#endif

/** @}
 */
