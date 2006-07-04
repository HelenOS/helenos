/*
 * Copyright (C) 2005 Ondrej Palkovsky
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
#include <typedefs.h>
#include <arch/byteorder.h>
#include <func.h>
#include <print.h>

/** Return entry that seems most likely to correspond to argument.
 *
 * Return entry that seems most likely to correspond
 * to address passed in the argument.
 *
 * @param addr Address.
 *
 * @return Pointer to respective symbol string on success, NULL otherwise.
 */
char * get_symtab_entry(unative_t addr)
{
	count_t i;

	for (i=1;symbol_table[i].address_le;++i) {
		if (addr < uint64_t_le2host(symbol_table[i].address_le))
			break;
	}
	if (addr >= uint64_t_le2host(symbol_table[i-1].address_le))
		return symbol_table[i-1].symbol_name;
	return NULL;
}

/** Find symbols that match the parameter forward and print them.
 *
 * @param name - search string
 * @param startpos - starting position, changes to found position
 * @return Pointer to the part of string that should be completed or NULL
 */
static char * symtab_search_one(const char *name, int *startpos)
{
	int namelen = strlen(name);
	char *curname;
	int i,j;
	int colonoffset = -1;

	for (i=0;name[i];i++)
		if (name[i] == ':') {
			colonoffset = i;
			break;
		}

	for (i=*startpos;symbol_table[i].address_le;++i) {
		/* Find a ':' in name */
		curname = symbol_table[i].symbol_name;
		for (j=0; curname[j] && curname[j] != ':'; j++)
			;
		if (!curname[j])
			continue;
		j -= colonoffset;
		curname += j;
		if (strlen(curname) < namelen)
			continue;
		if (strncmp(curname, name, namelen) == 0) {
			*startpos = i;
			return curname+namelen;
		}
	}
	return NULL;
}

/** Return address that corresponds to the entry
 *
 * Search symbol table, and if there is one match, return it
 *
 * @param name Name of the symbol
 * @return 0 - Not found, -1 - Duplicate symbol, other - address of symbol
 */
uintptr_t get_symbol_addr(const char *name)
{
	count_t found = 0;
	uintptr_t addr = NULL;
	char *hint;
	int i;

	i = 0;
	while ((hint=symtab_search_one(name, &i))) {
		if (!strlen(hint)) {
			addr =  uint64_t_le2host(symbol_table[i].address_le);
			found++;
		}
		i++;
	}
	if (found > 1)
		return ((uintptr_t) -1);
	return addr;
}

/** Find symbols that match parameter and prints them */
void symtab_print_search(const char *name)
{
	int i;
	uintptr_t addr;
	char *realname;


	i = 0;
	while (symtab_search_one(name, &i)) {
		addr =  uint64_t_le2host(symbol_table[i].address_le);
		realname = symbol_table[i].symbol_name;
		printf("%.*p: %s\n", sizeof(uintptr_t) * 2, addr, realname);
		i++;
	}
}

/** Symtab completion
 *
 * @param input - Search string, completes to symbol name
 * @returns - 0 - nothing found, 1 - success, >1 print duplicates 
 */
int symtab_compl(char *input)
{
	char output[MAX_SYMBOL_NAME+1];
	int startpos = 0;
	char *foundtxt;
	int found = 0;
	int i;
	char *name = input;

	/* Allow completion of pointers  */
	if (name[0] == '*' || name[0] == '&')
		name++;

	/* Do not print everything */
	if (!strlen(name))
		return 0;
	

	output[0] = '\0';

	while ((foundtxt = symtab_search_one(name, &startpos))) {
		startpos++;
		if (!found)
			strncpy(output, foundtxt, strlen(foundtxt)+1);
		else {
			for (i=0; output[i] && foundtxt[i] && output[i]==foundtxt[i]; i++)
				;
			output[i] = '\0';
		}
		found++;
	}
	if (!found)
		return 0;

	if (found > 1 && !strlen(output)) {
		printf("\n");
		startpos = 0;
		while ((foundtxt = symtab_search_one(name, &startpos))) {
			printf("%s\n", symbol_table[startpos].symbol_name);
			startpos++;
		}
	}
	strncpy(input, output, MAX_SYMBOL_NAME);
	return found;
	
}

 /** @}
 */

