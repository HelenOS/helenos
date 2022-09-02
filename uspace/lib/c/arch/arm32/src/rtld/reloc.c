/*
 * Copyright (c) 2019 Jiri Svoboda
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

/** @addtogroup libcarm32
 * @brief
 * @{
 */
/**
 * @file
 */

#include <bitops.h>
#include <smc.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <str.h>

#include <libarch/rtld/elf_dyn.h>
#include <rtld/symbol.h>
#include <rtld/rtld.h>
#include <rtld/rtld_debug.h>
#include <rtld/rtld_arch.h>

void module_process_pre_arch(module_t *m)
{
	/* Unused */
}

/**
 * Process (fixup) all relocations in a relocation table.
 */
void rel_table_process(module_t *m, elf_rel_t *rt, size_t rt_size)
{
	unsigned i;

	size_t rt_entries;
	size_t r_offset;
	elf_word r_info;
	unsigned rel_type;
	elf_word sym_idx;
	uint32_t sym_addr;

	elf_symbol_t *sym_table;
	elf_symbol_t *sym;
	uint32_t *r_ptr;
	uint32_t sym_size;
	char *str_tab;

	elf_symbol_t *sym_def;
	module_t *dest;

	DPRINTF("parse relocation table\n");

	sym_table = m->dyn.sym_tab;
	rt_entries = rt_size / sizeof(elf_rel_t);
	str_tab = m->dyn.str_tab;

	DPRINTF("address: 0x%" PRIxPTR ", entries: %zd\n", (uintptr_t)rt, rt_entries);

	for (i = 0; i < rt_entries; ++i) {
#if 0
		DPRINTF("symbol %d: ", i);
#endif
		r_offset = rt[i].r_offset;
		r_info = rt[i].r_info;

		sym_idx = ELF32_R_SYM(r_info);
		sym = &sym_table[sym_idx];

#if 0
		DPRINTF("name '%s', value 0x%x, size 0x%x\n",
		    str_tab + sym->st_name, sym->st_value, sym->st_size);
#endif
		rel_type = ELF32_R_TYPE(r_info);
		r_ptr = (uint32_t *)(r_offset + m->bias);

		if (sym->st_name != 0) {
#if 0
			DPRINTF("rel_type: %x, rel_offset: 0x%x\n", rel_type, r_offset);
#endif
			sym_def = symbol_def_find(str_tab + sym->st_name,
			    m, ssf_none, &dest);
			DPRINTF("dest name: '%s'\n", dest->dyn.soname);
			DPRINTF("dest bias: 0x%x\n", dest->bias);
			if (sym_def) {
				sym_addr = (uint32_t)
				    symbol_get_addr(sym_def, dest, NULL);
#if 0
				DPRINTF("symbol definition found, addr=0x%x\n", sym_addr);
#endif
			} else {
				printf("Definition of '%s' not found.\n",
				    str_tab + sym->st_name);
				continue;
			}
		} else {
			sym_addr = 0;
			sym_def = NULL;
			dest = m;
		}

		switch (rel_type) {
		case R_ARM_TLS_DTPMOD32:
			DPRINTF("fixup R_ARM_TLS_DTPMOD32\n");
			*r_ptr = dest->id;
			break;

		case R_ARM_TLS_DTPOFF32:
			DPRINTF("fixup R_ARM_TLS_DTPOFF32\n");
			*r_ptr = sym_def->st_value;
			break;

		case R_ARM_TLS_TPOFF32:
			DPRINTF("fixup R_ARM_TLS_TPOFF\n");
			if (sym_def != NULL)
				*r_ptr = sym_def->st_value + dest->tpoff;
			else
				*r_ptr = m->tpoff;
			break;

		case R_ARM_COPY:
			/*
			 * Copy symbol data from shared object to specified
			 * location. Need to find the 'source', i.e. the
			 * other instance of the object than the one in the
			 * executable program.
			 */
			DPRINTF("fixup R_ARM_COPY (s)\n");

			sym_def = symbol_def_find(str_tab + sym->st_name,
			    m, ssf_noexec, &dest);

			if (sym_def) {
				sym_addr = (uint32_t)
				    symbol_get_addr(sym_def, dest, NULL);
			} else {
				printf("Source definition of '%s' not found.\n",
				    str_tab + sym->st_name);
				continue;
			}

			sym_size = sym->st_size;
			if (sym_size != sym_def->st_size) {
				printf("Warning: Mismatched symbol sizes.\n");
				/* Take the lower value. */
				if (sym_size > sym_def->st_size)
					sym_size = sym_def->st_size;
			}

			memcpy(r_ptr, (const void *)sym_addr, sym_size);
			break;

		case R_ARM_GLOB_DAT:
		case R_ARM_JUMP_SLOT:
		case R_ARM_ABS32:
			DPRINTF("fixup R_ARM_GLOB_DAT/JUMP_SLOT/ABS32 (S)\n");
			*r_ptr = sym_addr;
			break;

		case R_ARM_RELATIVE:
			DPRINTF("fixup R_ARM_RELATIVE (B)\n");
			*r_ptr += dest->bias;
			break;

		default:
			printf("Error: Unknown relocation type %d\n",
			    rel_type);
			exit(1);
		}
	}
}

/**
 * Process (fixup) all relocations in a relocation table with explicit addends.
 */
void rela_table_process(module_t *m, elf_rela_t *rt, size_t rt_size)
{
	(void) m;
	(void) rt;
	(void) rt_size;
}

/** Get the adress of a function.
 *
 * @param sym Symbol
 * @param m Module in which the symbol is located
 * @return Address of function
 */
void *func_get_addr(elf_symbol_t *sym, module_t *m)
{
	return symbol_get_addr(sym, m, __tcb_get());
}

/** @}
 */
