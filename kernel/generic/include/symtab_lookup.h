/*
 * SPDX-FileCopyrightText: 2005 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_SYMTAB_LOOKUP_H_
#define KERN_SYMTAB_LOOKUP_H_

#include <typedefs.h>

#define MAX_SYMBOL_NAME  64

struct symtab_entry {
	uint64_t address_le;
	char symbol_name[MAX_SYMBOL_NAME];
};

extern errno_t symtab_name_lookup(uintptr_t, const char **, uintptr_t *);
extern const char *symtab_fmt_name_lookup(uintptr_t);
extern errno_t symtab_addr_lookup(const char *, uintptr_t *);

#ifdef CONFIG_SYMTAB

/** Symtable linked together by build process
 *
 */
extern struct symtab_entry symbol_table[];

#endif /* CONFIG_SYMTAB */

#endif

/** @}
 */
