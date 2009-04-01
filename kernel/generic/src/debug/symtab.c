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

/** @addtogroup genericdebug
 * @{
 */

/**
 * @file
 * @brief	Kernel symbol resolver.
 */

#include <symtab.h>
#include <byteorder.h>
#include <string.h>
#include <print.h>
#include <arch/types.h>
#include <typedefs.h>
#include <errno.h>

/** Get name of symbol that seems most likely to correspond to address.
 *
 * @param addr	Address.
 * @param name	Place to store pointer to the symbol name.
 *
 * @return	Zero on success or negative error code, ENOENT if not found,
 *		ENOTSUP if symbol table not available.
 */
int symtab_name_lookup(unative_t addr, char **name)
{
#ifdef CONFIG_SYMTAB
	count_t i;

	for (i = 1; symbol_table[i].address_le; ++i) {
		if (addr < uint64_t_le2host(symbol_table[i].address_le))
			break;
	}
	if (addr >= uint64_t_le2host(symbol_table[i - 1].address_le)) {
		*name = symbol_table[i - 1].symbol_name;
		return EOK;
	}

	*name = NULL;
	return ENOENT;
#else
	*name = NULL;
	return ENOTSUP;
#endif
}

/** Lookup symbol by address and format for display.
 *
 * Returns name of closest corresponding symbol, "Not found" if none exists
 * or "N/A" if no symbol information is available.
 *
 * @param addr	Address.
 * @param name	Place to store pointer to the symbol name.
 *
 * @return	Pointer to a human-readable string.
 */
char *symtab_fmt_name_lookup(unative_t addr)
{
	int rc;
	char *name;

	rc = symtab_name_lookup(addr, &name);
	switch (rc) {
	case EOK: return name;
	case ENOENT: return "Not found";
	default: return "N/A";
	}
}

#ifdef CONFIG_SYMTAB

/** Find symbols that match the parameter forward and print them.
 *
 * @param name - search string
 * @param startpos - starting position, changes to found position
 * @return Pointer to the part of string that should be completed or NULL
 */
static char * symtab_search_one(const char *name, int *startpos)
{
	unsigned int namelen = str_size(name);
	char *curname;
	int i, j;
	int colonoffset = -1;

	for (i = 0; name[i]; i++)
		if (name[i] == ':') {
			colonoffset = i;
			break;
		}

	for (i = *startpos; symbol_table[i].address_le; ++i) {
		/* Find a ':' in name */
		curname = symbol_table[i].symbol_name;
		for (j = 0; curname[j] && curname[j] != ':'; j++)
			;
		if (!curname[j])
			continue;
		j -= colonoffset;
		curname += j;
		if (str_size(curname) < namelen)
			continue;
		if (strncmp(curname, name, namelen) == 0) {
			*startpos = i;
			return curname + namelen;
		}
	}
	return NULL;
}

#endif

/** Return address that corresponds to the entry
 *
 * Search symbol table, and if there is one match, return it
 *
 * @param name	Name of the symbol
 * @param addr	Place to store symbol address
 *
 * @return 	Zero on success, ENOENT - not found, EOVERFLOW - duplicate
 *		symbol, ENOTSUP - no symbol information available.
 */
int symtab_addr_lookup(const char *name, uintptr_t *addr)
{
#ifdef CONFIG_SYMTAB
	count_t found = 0;
	char *hint;
	int i;

	i = 0;
	while ((hint = symtab_search_one(name, &i))) {
		if (!str_size(hint)) {
			*addr =  uint64_t_le2host(symbol_table[i].address_le);
			found++;
		}
		i++;
	}
	if (found > 1)
		return EOVERFLOW;
	if (found < 1)
		return ENOENT;
	return EOK;
#else
	return ENOTSUP;
#endif
}

/** Find symbols that match parameter and prints them */
void symtab_print_search(const char *name)
{
#ifdef CONFIG_SYMTAB
	int i;
	uintptr_t addr;
	char *realname;


	i = 0;
	while (symtab_search_one(name, &i)) {
		addr =  uint64_t_le2host(symbol_table[i].address_le);
		realname = symbol_table[i].symbol_name;
		printf("%p: %s\n", addr, realname);
		i++;
	}
#else
	printf("No symbol information available.\n");
#endif
}

/** Symtab completion
 *
 * @param input - Search string, completes to symbol name
 * @returns - 0 - nothing found, 1 - success, >1 print duplicates 
 */
int symtab_compl(char *input)
{
#ifdef CONFIG_SYMTAB
	char output[MAX_SYMBOL_NAME + 1];
	int startpos = 0;
	char *foundtxt;
	int found = 0;
	int i;
	char *name = input;

	/* Allow completion of pointers  */
	if (name[0] == '*' || name[0] == '&')
		name++;

	/* Do not print everything */
	if (!str_size(name))
		return 0;
	

	output[0] = '\0';

	while ((foundtxt = symtab_search_one(name, &startpos))) {
		startpos++;
		if (!found)
			strncpy(output, foundtxt, str_size(foundtxt) + 1);
		else {
			for (i = 0; output[i] && foundtxt[i] &&
			     output[i] == foundtxt[i]; i++)
				;
			output[i] = '\0';
		}
		found++;
	}
	if (!found)
		return 0;

	if (found > 1 && !str_size(output)) {
		printf("\n");
		startpos = 0;
		while ((foundtxt = symtab_search_one(name, &startpos))) {
			printf("%s\n", symbol_table[startpos].symbol_name);
			startpos++;
		}
	}
	strncpy(input, output, MAX_SYMBOL_NAME);
	return found;
#else
	return 0;
#endif
}

/** @}
 */
