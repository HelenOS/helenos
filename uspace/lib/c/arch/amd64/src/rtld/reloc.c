/*
 * Copyright (c) 2016 Jiri Svoboda
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

/** @addtogroup libcamd64
 * @brief
 * @{
 */
/**
 * @file
 */

#include <mem.h>
#include <stdio.h>
#include <stdlib.h>

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
 * Process (fixup) all relocations in a relocation table with implicit addends.
 */
void rel_table_process(module_t *m, elf_rel_t *rt, size_t rt_size)
{

	DPRINTF("rel table address: 0x%zx, size: %zd\n", (uintptr_t)rt, rt_size);
	/* Unused */
	(void)m;
	(void)rt;
	(void)rt_size;
}

/**
 * Process (fixup) all relocations in a relocation table with explicit addends.
 */
void rela_table_process(module_t *m, elf_rela_t *rt, size_t rt_size)
{
	unsigned i;

	size_t rt_entries;
	size_t r_offset;
	size_t r_addend;
	elf_xword r_info;
	unsigned rel_type;
	elf_word sym_idx;
	uintptr_t sym_addr;

	elf_symbol_t *sym_table;
	elf_symbol_t *sym;
	uintptr_t *r_ptr;
	uint32_t *r_ptr32;
	uintptr_t sym_size;
	char *str_tab;

	elf_symbol_t *sym_def;
	module_t *dest;

	DPRINTF("parse relocation table\n");

	sym_table = m->dyn.sym_tab;
	rt_entries = rt_size / sizeof(elf_rela_t);
	str_tab = m->dyn.str_tab;

	DPRINTF("rel table address: 0x%zx, entries: %zd\n", (uintptr_t)rt, rt_entries);

	for (i = 0; i < rt_entries; ++i) {
#if 0
		DPRINTF("symbol %d: ", i);
#endif
		r_offset = rt[i].r_offset;
		r_info = rt[i].r_info;
		r_addend = rt[i].r_addend;

		sym_idx = ELF64_R_SYM(r_info);
		sym = &sym_table[sym_idx];

#if 0
		DPRINTF("name '%s', value 0x%x, size 0x%x\n",
		    str_tab + sym->st_name,
		    sym->st_value,
		    sym->st_size);
#endif
		rel_type = ELF64_R_TYPE(r_info);
		r_ptr = (uintptr_t *)(r_offset + m->bias);
		r_ptr32 = (uint32_t *)r_ptr;

		if (sym->st_name != 0) {
			DPRINTF("rel_type: %x, rel_offset: 0x%zx\n", rel_type, r_offset);
			sym_def = symbol_def_find(str_tab + sym->st_name,
			    m, ssf_none, &dest);
			DPRINTF("dest name: '%s'\n", dest->dyn.soname);
			DPRINTF("dest bias: 0x%zx\n", dest->bias);
			if (sym_def) {
				sym_addr = (uintptr_t)
				    symbol_get_addr(sym_def, dest, NULL);
				DPRINTF("symbol definition found, value=0x%zx addr=0x%zx\n", sym_def->st_value, sym_addr);
			} else {
				printf("Definition of '%s' not found.\n",
				    str_tab + sym->st_name);
				continue;
			}
		} else {
			sym_addr = 0;
			sym_def = NULL;

			/*
			 * DTPMOD with null st_name should return the index
			 * of the current module.
			 */
			dest = m;
		}

		switch (rel_type) {
		case R_X86_64_64:
			DPRINTF("fixup R_X86_64_64 (S+A)\n");
			DPRINTF("*0x%zx = 0x%zx\n", (uintptr_t)r_ptr, sym_addr + r_addend);
			*r_ptr = sym_addr + r_addend;
			DPRINTF("OK\n");
			break;

		case R_X86_64_PC32:
			DPRINTF("fixup R_X86_64_PC32 (S+A-P)\n");
			DPRINTF("(32)*0x%zx = 0x%zx\n", (uintptr_t)r_ptr, sym_addr + r_addend - (uintptr_t) r_ptr);
			*r_ptr32 = sym_addr + r_addend - (uintptr_t) r_ptr;
			DPRINTF("OK\n");
			break;

		case R_X86_64_COPY:
			/*
			 * Copy symbol data from shared object to specified
			 * location. Need to find the 'source', i.e. the
			 * other instance of the object than the one in the
			 * executable program.
			 */
			DPRINTF("fixup R_X86_64_COPY (s)\n");

			sym_def = symbol_def_find(str_tab + sym->st_name,
			    m, ssf_noexec, &dest);

			if (sym_def) {
				sym_addr = (uintptr_t)
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
			DPRINTF("OK\n");
			break;

		case R_X86_64_GLOB_DAT:
		case R_X86_64_JUMP_SLOT:
			DPRINTF("fixup R_X86_64_GLOB_DAT/JUMP_SLOT (S)\n");
			DPRINTF("*0x%zx = 0x%zx\n", (uintptr_t)r_ptr, sym_addr);
			*r_ptr = sym_addr;
			DPRINTF("OK\n");
			break;

		case R_X86_64_RELATIVE:
			DPRINTF("fixup R_X86_64_RELATIVE (B+A)\n");
			DPRINTF("*0x%zx = 0x%zx\n", (uintptr_t)r_ptr, m->bias + r_addend);
			*r_ptr = m->bias + r_addend;
			DPRINTF("OK\n");
			break;

		case R_X86_64_DTPMOD64:
			DPRINTF("fixup R_X86_64_DTPMOD64\n");
			DPRINTF("*0x%zx = 0x%zx\n", (uintptr_t)r_ptr, (size_t)dest->id);
			*r_ptr = dest->id;
			DPRINTF("OK\n");
			break;

		case R_X86_64_DTPOFF64:
			DPRINTF("fixup R_X86_64_DTPOFF64\n");
			DPRINTF("*0x%zx = 0x%zx\n", (uintptr_t)r_ptr, sym_def->st_value);
			*r_ptr = sym_def->st_value;
			DPRINTF("OK\n");
			break;

		case R_X86_64_TPOFF64:
			DPRINTF("fixup R_X86_64_TPOFF64\n");
			*r_ptr = sym_def->st_value + dest->tpoff;
			break;

		default:
			printf("Error: Unknown relocation type %d\n",
			    rel_type);
			exit(1);
		}

	}
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
