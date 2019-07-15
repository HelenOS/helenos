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

/** @addtogroup libcia64
 * @brief
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <str.h>

#include <libarch/rtld/elf_dyn.h>
#include <rtld/symbol.h>
#include <rtld/rtld.h>
#include <rtld/rtld_debug.h>
#include <rtld/rtld_arch.h>

static void fill_fun_desc(uintptr_t *, uintptr_t, uintptr_t);

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

static uintptr_t get_module_gp(module_t *m)
{
#if 0
	printf("get_module_gp: m=%p m->bias=%zu m->dyn.plt_got=%zu\n",
	    m, m->bias, (uintptr_t) m->dyn.plt_got);
#endif
	return (uintptr_t) m->dyn.plt_got;
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
		DPRINTF("r sym: name '%s', value 0x%zx, size 0x%zx, binding 0x%x\n",
		    str_tab + sym->st_name,
		    sym->st_value,
		    sym->st_size,
		    elf_st_bind(sym->st_info));
#endif
		rel_type = ELF64_R_TYPE(r_info);
		r_ptr = (uintptr_t *)(r_offset + m->bias);

		if (elf_st_bind(sym->st_info) == STB_LOCAL) {
			sym_def = sym;
			dest = m;
			sym_addr = (uintptr_t)
			    symbol_get_addr(sym_def, dest, NULL);
			DPRINTF("Resolved local symbol, addr=0x%zx\n", sym_addr);
		} else if (sym->st_name != 0) {
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
		case R_IA_64_DIR64LSB:
			DPRINTF("fixup R_IA_DIR64LSB (S+A)\n");
			DPRINTF("*0x%zx = 0x%zx\n", (uintptr_t)r_ptr,
			    sym_addr + r_addend);
			*r_ptr = sym_addr + r_addend;
			DPRINTF("OK\n");
			break;

		case R_IA_64_FPTR64LSB:
			// FIXME ?????
			DPRINTF("fixup R_IA_FPTR64LSB (@fptr(S+A))\n");
			DPRINTF("*0x%zx = 0x%zx\n", (uintptr_t)r_ptr,
			    sym_addr + r_addend);
			*r_ptr = sym_addr + r_addend;
			DPRINTF("OK\n");
			break;
		case R_IA_64_REL64LSB:
			DPRINTF("fixup R_IA_REL64LSB (BD+A)\n");
			DPRINTF("*0x%zx = 0x%zx\n", (uintptr_t)r_ptr,
			    dest->bias + r_addend);
			*r_ptr = dest->bias + r_addend;
			DPRINTF("OK\n");
			break;

		case R_IA_64_IPLTLSB:
			DPRINTF("fixup R_IA_64_IPLTLSB\n");
			DPRINTF("r_offset=0x%zx r_addend=0x%zx\n",
			    r_offset, r_addend);

			sym_size = sym->st_size;
			if (sym_size != sym_def->st_size) {
				printf("Warning: Mismatched symbol sizes.\n");
				/* Take the lower value. */
				if (sym_size > sym_def->st_size)
					sym_size = sym_def->st_size;
			}

			DPRINTF("symbol='%s'\n", str_tab + sym->st_name);
			DPRINTF("sym_addr = 0x%zx\n", sym_addr);
			DPRINTF("gp = 0x%zx\n", get_module_gp(dest));
			DPRINTF("r_offset=0x%zx\n", r_offset);

			/*
			 * Initialize function descriptor entry with the
			 * address of the function and the value of the
			 * global pointer.
			 */
			fill_fun_desc(r_ptr, sym_addr, get_module_gp(dest));

			DPRINTF("OK\n");
			break;

		case R_IA_64_DTPMOD64LSB:
			DPRINTF("fixup R_IA_64_DTPMOD64LSB\n");
			DPRINTF("*0x%zx = 0x%zx\n", (uintptr_t)r_ptr, (size_t)dest->id);
			*r_ptr = dest->id;
			DPRINTF("OK\n");
			break;

		case R_IA_64_DTPREL64LSB:
			DPRINTF("fixup R_IA_64_DTPREL64LSB\n");
			DPRINTF("*0x%zx = 0x%zx\n", (uintptr_t)r_ptr, sym_def->st_value);
			*r_ptr = sym_def->st_value;
			DPRINTF("OK\n");
			break;

		default:
			printf("Error: Unknown relocation type %d\n",
			    rel_type);
			exit(1);
		}

	}
}

static void fill_fun_desc(uintptr_t *fdesc, uintptr_t faddr, uintptr_t gp)
{
	fdesc[0] = faddr;
	fdesc[1] = gp;
}

#include <rtld/rtld_arch.h>

/** Get the adress of a function.
 *
 * On IA-64 we actually return the address of the function descriptor.
 *
 * @param sym Symbol
 * @param m Module in which the symbol is located
 * @return Address of function
 */
void *func_get_addr(elf_symbol_t *sym, module_t *m)
{
	void *fa;
	uintptr_t *fdesc;

#if 0
	printf("func_get_addr: name='%s'\n", m->dyn.str_tab + sym->st_name);
#endif
	fa = symbol_get_addr(sym, m, __tcb_get());
	if (fa == NULL)
		return NULL;

	fdesc = malloc(16);
	if (fdesc == NULL)
		return NULL;

	fill_fun_desc(fdesc, (uintptr_t)fa, get_module_gp(m));
#if 0
	printf("-> fun_desc=%p\n", fdesc);
#endif
	return fdesc;
}

/** @}
 */
