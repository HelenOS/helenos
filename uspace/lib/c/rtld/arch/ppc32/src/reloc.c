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
#include <smc.h>

#define __L(ptr) ((uint32_t)(ptr) & 0x0000ffff)
#define __HA(ptr) ((uint32_t)(ptr) >> 16)

// ldis r11, .PLTtable@ha
static inline uint32_t _ldis(unsigned rD, uint16_t imm16)
{
	/* Special case of addis: ldis rD,SIMM == addis rD,0,SIMM */
	return 0x3C000000 | (rD << 21) | imm16;
}

static inline uint32_t _lwz(unsigned rD, uint16_t disp16, unsigned rA)
{
	return 0x80000000 | (rD << 21) | (rA << 16) | disp16;
}

static inline uint32_t _mtctr(unsigned rS)
{
	/* mtctr rD == mtspr 9, rD */
	return 0x7c0003a6 | (rS << 21) | (9/*CTR*/ << 16);
}

static inline uint32_t _bctr()
{
	/* bcctr 0x1f, 0 */
	return 0x4c000420 | (0x1f/*always*/ << 21);
}

/* branch */
static inline uint32_t _b(uint32_t *addr, uint32_t *location)
{
	uint32_t raddr = ((uint32_t)addr - (uint32_t)location) & 0x03fffffc;
	return 0x48000000 | raddr;
}


/*
 * Fill in PLT
 */
void module_process_pre_arch(module_t *m)
{
	uint32_t *plt;
	uint32_t *_plt_ent;
	
	/* No lazy linking -- no pre-processing yet. */
	return;

	plt = m->dyn.plt_got;
	if (!plt) {
		/* Module has no PLT */
		return;
	}

	// PLT entries start here. However, each occupies 2 words
	_plt_ent = plt + 18;

	// By definition of the ppc ABI, there's 1:1 correspondence
	// between JMPREL entries and PLT entries
	unsigned plt_n = m->dyn.plt_rel_sz / sizeof(elf_rela_t);

	uint32_t *_plt_table;
	uint32_t *_plt_call;
	uint32_t *_plt_resolve;

	_plt_resolve = plt;
	_plt_call = plt + 6;
	_plt_table = plt + 18 + plt_n;

/* .PLTcall: */
	plt[6] = _ldis(11, __HA(_plt_table));	// ldis r11, .PLTtable@ha
	plt[7] = _lwz(11, __L(_plt_table), 11);	// lwz r11, .PLTtable@l(r11)
	plt[8] = _mtctr(11);			// mtctr r11
	plt[9] = _bctr();

/* .PLTi, i = 0..N-1 */
//	kputint(-4);
/*	for (i = 0; i < plt_n; ++i) {
		//_plt_table[i] == function address;
		plt[18+i] = _b(_plt_call, &plt[18+i]);	// b .PLTcall
	}*/
}

void rel_table_process(module_t *m, elf_rel_t *rt, size_t rt_size)
{
	/* Unused */
	(void)m; (void)rt; (void)rt_size;
}

/**
 * Process (fixup) all relocations in a relocation table.
 */
void rela_table_process(module_t *m, elf_rela_t *rt, size_t rt_size)
{
	int i;

	size_t rt_entries;
	size_t r_offset;
	elf_word r_info;
	unsigned rel_type;
	elf_word sym_idx;
	uintptr_t sym_addr;
	uintptr_t r_addend;
	
	elf_symbol_t *sym_table;
	elf_symbol_t *sym;
	uint32_t *r_ptr;
	uint16_t *r_ptr16;
	char *str_tab;
	
	elf_symbol_t *sym_def;
	module_t *dest;

	uint32_t *plt;
	uint32_t *_plt_table;
	uint32_t *_plt_ent;
	uint32_t plt_n;
	uint32_t pidx;
	uint32_t t_addr;
	uint32_t sym_size;

	plt = m->dyn.plt_got;
	plt_n = m->dyn.plt_rel_sz / sizeof(elf_rela_t);
	_plt_ent = plt+ 18;
	_plt_table = plt + 18 + plt_n;

	DPRINTF("parse relocation table\n");

	sym_table = m->dyn.sym_tab;
	rt_entries = rt_size / sizeof(elf_rela_t);
	str_tab = m->dyn.str_tab;

	DPRINTF("address: 0x%x, entries: %d\n", (uintptr_t)rt, rt_entries);
	
	for (i = 0; i < rt_entries; ++i) {
		DPRINTF("symbol %d: ", i);
		r_offset = rt[i].r_offset;
		r_info = rt[i].r_info;
		r_addend = rt[i].r_addend;

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
			sym_def = symbol_def_find(str_tab + sym->st_name,
			    m, &dest);
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

		switch (rel_type) {
		case R_PPC_ADDR16_LO:
			DPRINTF("fixup R_PPC_ADDR16_LO (#lo(s+a))\n");
			*r_ptr16 = (sym_addr + r_addend) & 0xffff;
			break;

		case R_PPC_ADDR16_HI:
			DPRINTF("fixup R_PPC_ADDR16_HI (#hi(s+a))\n");
			*r_ptr16 = (sym_addr + r_addend) >> 16;
			break;

		case R_PPC_ADDR16_HA:
			DPRINTF("fixup R_PPC_ADDR16_HA (#ha(s+a))\n");
			t_addr = sym_addr + r_addend;
			*r_ptr16 = (t_addr >> 16) + ((t_addr & 0x8000) ? 1 : 0);
			break;

		case R_PPC_JMP_SLOT:
			DPRINTF("fixup R_PPC_JMP_SLOT (b+v)\n");
			pidx = (r_ptr - _plt_ent) / 2;
			if (pidx >= plt_n) {
				DPRINTF("error: proc index out of range\n");
				exit(1);
			}
			plt[18+2*pidx] = _b((void *)sym_addr, &plt[18+2*pidx]);
			break;

		case R_PPC_ADDR32:
			DPRINTF("fixup R_PPC_ADDR32 (b+v+a)\n");
			*r_ptr = r_addend + sym_addr;
			break;

		case R_PPC_COPY:
			/*
			 * Copy symbol data from shared object to specified
			 * location.
			 */
			DPRINTF("fixup R_PPC_COPY (s)\n");
			sym_size = sym->st_size;
			if (sym_size != sym_def->st_size) {
				printf("warning: mismatched symbol sizes\n");
				/* Take the lower value. */
				if (sym_size > sym_def->st_size)
					sym_size = sym_def->st_size;
			}
			memcpy(r_ptr, (const void *)sym_addr, sym_size);
			break;
			
		case R_PPC_RELATIVE:
			DPRINTF("fixup R_PPC_RELATIVE (b+a)\n");
			*r_ptr = r_addend + m->bias;
			break;

		case R_PPC_REL24:
			DPRINTF("fixup R_PPC_REL24 (s+a-p)>>2\n");
			*r_ptr = (sym_addr + r_addend - (uint32_t)r_ptr) >> 2;
			break;

		case R_PPC_DTPMOD32:
			/*
			 * We can ignore this as long as the only module
			 * with TLS variables is libc.so.
			 */
			DPRINTF("Ignoring R_PPC_DTPMOD32\n");
			break;

		default:
			printf("Error: Unknown relocation type %d.\n",
			    rel_type);
			exit(1);
			break;
		}
	}

	/*
	 * Synchronize the used portion of PLT. This is necessary since
	 * we are writing instructions.
	 */
	smc_coherence(&plt[18], plt_n * 2 * sizeof(uint32_t));
}

/** @}
 */
