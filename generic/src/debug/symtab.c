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


#include <symtab.h>
#include <typedefs.h>
#include <arch/byteorder.h>
#include <func.h>
#include <print.h>

/** Return entry that seems most likely to correspond to address
 *
 * Return entry that seems most likely to correspond
 * to address passed in the argument.
 *
 * @param addr Address.
 *
 * @return Pointer to respective symbol string on success, NULL otherwise.
 */
char * get_symtab_entry(__native addr)
{
	count_t i;

	for (i=1;symbol_table[i].address_le;++i) {
		if (addr < __u64_le2host(symbol_table[i].address_le))
			break;
	}
	if (addr >= __u64_le2host(symbol_table[i-1].address_le))
		return symbol_table[i-1].symbol_name;
	return NULL;
}

/** Return address that corresponds to the entry
 *
 * Search symbol table, and if the address ENDS with
 * the parameter, return value
 *
 * @param name Name of the symbol
 * @return 0 - Not found, -1 - Duplicate symbol, other - address of symbol
 */
__address get_symbol_addr(const char *name)
{
	count_t i;
	count_t found = 0;
	count_t found_pos;

	count_t nmlen = strlen(name);
	count_t slen;

	for (i=0;symbol_table[i].address_le;++i) {
		slen = strlen(symbol_table[i].symbol_name);
		if (slen < nmlen)
			continue;
		if (strncmp(name, symbol_table[i].symbol_name + (slen-nmlen),
			    nmlen) == 0) {
			found++;
			found_pos = i;
		}
	}
	if (found == 0)
		return NULL;
	if (found == 1)
		return __u64_le2host(symbol_table[found_pos].address_le);
	return ((__address) -1);
}

void symtab_print_search(const char *name)
{
	int i;
	count_t nmlen = strlen(name);
	count_t slen;
	__address addr;
	char *realname;

	for (i=0;symbol_table[i].address_le;++i) {
		slen = strlen(symbol_table[i].symbol_name);
		if (slen < nmlen)
			continue;
		if (strncmp(name, symbol_table[i].symbol_name + (slen-nmlen),
			    nmlen) == 0) {
			addr =  __u64_le2host(symbol_table[i].address_le);
			realname = symbol_table[i].symbol_name;
			printf("0x%p: %s\n", addr, realname);
		}
	}
}
