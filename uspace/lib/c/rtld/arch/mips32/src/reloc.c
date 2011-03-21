/*
 * Copyright (c) 2008 Jiri Svoboda
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

/** @addtogroup rtld rtld
 * @brief
 * @{
 */ 
/**
 * @file
 */

#include <stdio.h>
#include <stdlib.h>

#include <arch.h>
#include <elf_dyn.h>
#include <symbol.h>
#include <rtld.h>

void module_process_pre_arch(module_t *m)
{
	elf_symbol_t *sym_table;
	elf_symbol_t *sym_def;
	elf_symbol_t *sym;
	uint32_t gotsym;
	uint32_t lgotno;
	uint32_t *got;
	char *str_tab;
	int i, j;

	uint32_t sym_addr;
	module_t *dest;

	got = (uint32_t *) m->dyn.plt_got;
	sym_table = m->dyn.sym_tab;
	str_tab = m->dyn.str_tab;
	gotsym = m->dyn.arch.gotsym;
	lgotno = m->dyn.arch.lgotno;

	DPRINTF("** Relocate GOT entries **\n");
	DPRINTF("MIPS base = 0x%x\n", m->dyn.arch.base);

	/*
	 * Local entries.
	 */
	for (i = 0; i < gotsym; i++) {
		/* FIXME: really subtract MIPS base? */
		got[i] += m->bias - m->dyn.arch.base;
	}

	DPRINTF("sym_ent = %d, gotsym = %d\n", m->dyn.arch.sym_no, gotsym);
	DPRINTF("lgotno = %d\n", lgotno);

	/*
	 * Iterate over GOT-mapped symbol entries.
	 */
	for (j = gotsym; j < m->dyn.arch.sym_no; j++) {
		/* Corresponding (global) GOT entry. */
		i = lgotno + j - gotsym;

		DPRINTF("relocate GOT entry %d\n", i);
//		getchar();
//		getchar();

		sym = &sym_table[j];
		if (ELF32_R_TYPE(sym->st_info) == STT_FUNC) {
			if (sym->st_shndx == SHN_UNDEF) {
				if (sym->st_value == 0) {
					/* 1 */
				} else {
					if (got[i] == sym->st_value) {
						/* 2 */
						DPRINTF("(2)\n");
						got[i] += m->bias - m->dyn.arch.base;
						continue;
					} else {
						/* 3 */
						DPRINTF("(3)\n");
						got[i] = sym->st_value + m->bias - m->dyn.arch.base;
						continue;
					}
				}
			} else {
				/* 2 */
				DPRINTF("(2)\n");
				got[i] += m->bias - m->dyn.arch.base;
				continue;
			}
		} else {
			if (sym->st_shndx == SHN_UNDEF ||
			    sym->st_shndx == SHN_COMMON) {
				/* 1 */
			} else {
				/* 1 */
			}
		}
		
		DPRINTF("(1) symbol name='%s'\n", str_tab + sym->st_name);
		sym_def = symbol_def_find(str_tab + sym->st_name, m, &dest);
		if (sym_def) {
			sym_addr = symbol_get_addr(sym_def, dest);
			DPRINTF("symbol definition found, addr=0x%x\n", sym_addr);
		} else {
			DPRINTF("symbol definition not found\n");
			continue;
		}
		DPRINTF("write 0x%x at 0x%x\n", sym_addr, (uint32_t) &got[i]);
		got[i] = sym_addr;
	}

	DPRINTF("** Done **\n");
}

/**
 * Process (fixup) all relocations in a relocation table.
 */
void rel_table_process(module_t *m, elf_rel_t *rt, size_t rt_size)
{
	int i;

	size_t rt_entries;
	size_t r_offset;
	elf_word r_info;
	unsigned rel_type;
	elf_word sym_idx;
	uintptr_t sym_addr;
	
	elf_symbol_t *sym_table;
	elf_symbol_t *sym;
	uint32_t *r_ptr;
	uint16_t *r_ptr16;
	char *str_tab;
	
	elf_symbol_t *sym_def;
	module_t *dest;

	uint32_t *got;
	uint32_t gotsym;
	uint32_t lgotno;
	uint32_t ea;

	DPRINTF("parse relocation table\n");

	sym_table = m->dyn.sym_tab;
	rt_entries = rt_size / sizeof(elf_rela_t);
	str_tab = m->dyn.str_tab;
	got = (uint32_t *) m->dyn.plt_got;
	gotsym = m->dyn.arch.gotsym;
	lgotno = m->dyn.arch.lgotno;

	DPRINTF("got=0x%lx, gotsym=%d\n", (uintptr_t) got, gotsym);

	DPRINTF("address: 0x%x, entries: %d\n", (uintptr_t)rt, rt_entries);
	
	for (i = 0; i < rt_entries; ++i) {
		DPRINTF("symbol %d: ", i);
		r_offset = rt[i].r_offset;
		r_info = rt[i].r_info;

		sym_idx = ELF32_R_SYM(r_info);
		sym = &sym_table[sym_idx];

		DPRINTF("name '%s', value 0x%x, size 0x%x\n",
		    str_tab + sym->st_name,
		    sym->st_value,
		    sym->st_size);

		rel_type = ELF32_R_TYPE(r_info);
		r_ptr = (uint32_t *)(r_offset + m->bias);
		r_ptr16 = (uint16_t *)(r_offset + m->bias);

		if (sym->st_name != 0) {
			DPRINTF("rel_type: %x, rel_offset: 0x%x\n", rel_type, r_offset);
			DPRINTF("dest name: '%s'\n", dest->dyn.soname);
			DPRINTF("dest bias: 0x%x\n", dest->bias);
			if (sym_def) {
				sym_addr = symbol_get_addr(sym_def, dest);
				DPRINTF("symbol definition found, addr=0x%x\n", sym_addr);
			} else {
				DPRINTF("symbol definition not found\n");
				continue;
			}
		}

		DPRINTF("switch(%u)\n", rel_type);

		switch (rel_type) {
		case R_MIPS_NONE:
			DPRINTF("Ignoring R_MIPS_NONE\n");
			break;

		case R_MIPS_REL32:
			DPRINTF("fixup R_MIPS_REL32 (r - ea + s)\n");
			if (sym_idx < gotsym)
				ea = sym_addr;
			else
				ea = got[lgotno + sym_idx - gotsym];

			*r_ptr += sym_addr - ea;
			DPRINTF("p = 0x%x, val := 0x%x\n", (uint32_t) r_ptr,
			    *r_ptr);
//			getchar();
			break;

		/* No other non-TLS relocation types should appear. */

		case R_MIPS_TLS_DTPMOD32:
			/*
			 * We can ignore this as long as the only module
			 * with TLS variables is libc.so.
			 */
			DPRINTF("Ignoring R_MIPS_DTPMOD32\n");
			break;

		default:
			printf("Error: Unknown relocation type %d.\n",
			    rel_type);
			exit(1);
			break;
		}
	}


	printf("relocation done\n");
}

void rela_table_process(module_t *m, elf_rela_t *rt, size_t rt_size)
{
	/* Unused */
	(void)m; (void)rt; (void)rt_size;
}

/** @}
 */
