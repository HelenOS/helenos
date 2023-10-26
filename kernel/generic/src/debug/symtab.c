/*
 * Copyright (c) 2005 Ondrej Palkovsky
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

/** @addtogroup kernel_generic_debug
 * @{
 */

/**
 * @file
 * @brief Kernel symbol resolver.
 */

#include <symtab.h>
#include <byteorder.h>
#include <str.h>
#include <stdio.h>
#include <typedefs.h>
#include <errno.h>
#include <console/prompt.h>

#include <abi/elf.h>
#include <debug/sections.h>

static inline size_t symtab_len()
{
	return symtab_size / sizeof(elf_symbol_t);
}

static inline const char *symtab_entry_name(int entry)
{
	size_t index = symtab[entry].st_name;

	if (index >= strtab_size)
		return NULL;

	return strtab + index;
}

static inline size_t symtab_next(size_t i)
{
	for (; i < symtab_len(); i++) {
		const char *name = symtab_entry_name(i);
		int st_bind = elf_st_bind(symtab[i].st_info);
		int st_type = elf_st_type(symtab[i].st_info);

		if (st_bind == STB_LOCAL)
			continue;

		if (name == NULL || *name == '\0')
			continue;

		if (st_type == STT_FUNC || st_type == STT_OBJECT)
			break;
	}

	return i;
}

const char *symtab_name_lookup(uintptr_t addr, uintptr_t *symbol_addr)
{
	if (symtab == NULL || strtab == NULL)
		return NULL;

	uintptr_t closest_symbol_addr = 0;
	uintptr_t closest_symbol_name = 0;

	for (size_t i = symtab_next(0); i < symtab_len(); i = symtab_next(i + 1)) {
		if (symtab[i].st_value > addr)
			continue;

		if (symtab[i].st_value + symtab[i].st_size > addr) {
			closest_symbol_addr = symtab[i].st_value;
			closest_symbol_name = symtab[i].st_name;
			break;
		}

		if (symtab[i].st_value > closest_symbol_addr) {
			closest_symbol_addr = symtab[i].st_value;
			closest_symbol_name = symtab[i].st_name;
		}
	}

	if (closest_symbol_addr == 0)
		return NULL;

	if (symbol_addr)
		*symbol_addr = closest_symbol_addr;

	if (closest_symbol_name >= strtab_size)
		return NULL;

	return strtab + closest_symbol_name;
}

/** Lookup symbol by address and format for display.
 *
 * Returns name of closest corresponding symbol,
 * "unknown" if none exists and "N/A" if no symbol
 * information is available.
 *
 * @param addr Address.
 * @param name Place to store pointer to the symbol name.
 *
 * @return Pointer to a human-readable string.
 *
 */
const char *symtab_fmt_name_lookup(uintptr_t addr)
{
	const char *name = symtab_name_lookup(addr, NULL);
	if (name == NULL)
		name = "<unknown>";
	return name;
}

/** Return address that corresponds to the entry.
 *
 * Search symbol table, and if there is one match, return it
 *
 * @param name Name of the symbol
 * @param addr Place to store symbol address
 *
 * @return Zero on success, ENOENT - not found
 *
 */
errno_t symtab_addr_lookup(const char *name, uintptr_t *addr)
{
	for (size_t i = symtab_next(0); i < symtab_len(); i = symtab_next(i + 1)) {
		if (str_cmp(name, symtab_entry_name(i)) == 0) {
			*addr = symtab[i].st_value;
			return EOK;
		}
	}

	return ENOENT;
}

/** Find symbols that match parameter and print them */
void symtab_print_search(const char *name)
{
	if (symtab == NULL || strtab == NULL) {
		printf("No symbol information available.\n");
		return;
	}

	size_t namelen = str_length(name);

	for (size_t i = symtab_next(0); i < symtab_len(); i = symtab_next(i + 1)) {
		const char *n = symtab_entry_name(i);

		if (str_lcmp(name, n, namelen) == 0) {
			printf("%p: %s\n", (void *) symtab[i].st_value, n);
		}
	}
}

/** Symtab completion enum, see kernel/generic/include/kconsole.h */
const char *symtab_hints_enum(const char *input, const char **help, void **ctx)
{
	if (symtab == NULL || strtab == NULL)
		return NULL;

	if (help)
		*help = NULL;

	size_t len = str_length(input);
	for (size_t i = symtab_next((size_t) *ctx); i < symtab_len(); i = symtab_next(i + 1)) {
		const char *curname = symtab_entry_name(i);

		if (str_lcmp(input, curname, len) == 0) {
			*ctx = (void *) (i + 1);
			return (curname + str_lsize(curname, len));
		}
	}

	return NULL;
}

/** @}
 */
