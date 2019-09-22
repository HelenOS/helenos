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

/** @addtogroup libcppc32
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

static void plt_farcall_init(uint32_t *plt, uint32_t *);
static void plt_entry_init(uint32_t *, uint32_t *, uint32_t *, uintptr_t);
static uint32_t *plt_entry_ptr(uint32_t *, size_t);
static size_t plt_entry_index(size_t);
static uint16_t addr_ha(uint32_t);
static uint16_t addr_l(uint32_t);

void module_process_pre_arch(module_t *m)
{
	/* Unused */
}

/**
 * Process (fixup) all relocations in a relocation table.
 */
void rel_table_process(module_t *m, elf_rel_t *rt, size_t rt_size)
{
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
	uintptr_t sym_size;
	char *str_tab;

	elf_symbol_t *sym_def;
	module_t *dest;
	uint32_t *plt;
	uint32_t *plt_datawords;
	size_t jmp_slots;

	DPRINTF("Count jump slots.\n");

	rt_entries = rt_size / sizeof(elf_rela_t);

	jmp_slots = 0;
	for (i = 0; i < rt_entries; ++i) {
		r_info = rt[i].r_info;
		rel_type = ELF32_R_TYPE(r_info);

		if (rel_type == R_PPC_JMP_SLOT)
			++jmp_slots;
	}

	DPRINTF("Init farcall section\n");

	plt = (uint32_t *)m->dyn.plt_got;

	/* Table with addresses starts just after last PLT entry */
	plt_datawords = plt_entry_ptr(plt, jmp_slots);

	/* Init farcall section with reference to datawords table */
	plt_farcall_init(plt, plt_datawords);

	DPRINTF("parse relocation table\n");

	sym_table = m->dyn.sym_tab;
	str_tab = m->dyn.str_tab;

	DPRINTF("rel table address: 0x%zx, entries: %zd\n", (uintptr_t)rt, rt_entries);

	for (i = 0; i < rt_entries; ++i) {
#if 0
		DPRINTF("symbol %d: ", i);
#endif
		r_offset = rt[i].r_offset;
		r_info = rt[i].r_info;
		r_addend = rt[i].r_addend;

		sym_idx = ELF32_R_SYM(r_info);
		sym = &sym_table[sym_idx];

#if 0
		DPRINTF("name '%s', value 0x%x, size 0x%x\n",
		    str_tab + sym->st_name,
		    sym->st_value,
		    sym->st_size);
#endif
		rel_type = ELF32_R_TYPE(r_info);
		r_ptr = (uintptr_t *)(r_offset + m->bias);

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
		case R_PPC_ADDR32:
			DPRINTF("fixup R_PPC_ADDR32 (S+A)\n");
			DPRINTF("*0x%zx = 0x%zx\n", (uintptr_t)r_ptr, sym_addr);
			*r_ptr = sym_addr + r_addend;
			DPRINTF("OK\n");
			break;
		case R_PPC_REL24:
			DPRINTF("fixup R_PPC_REL24 ((S+A-P) >> 2)\n");
			DPRINTF("*0x%zx = 0x%zx\n", (uintptr_t)r_ptr,
			    (sym_addr + r_addend - (uintptr_t)r_ptr) >> 2);
			*r_ptr = (sym_addr + r_addend - (uintptr_t)r_ptr) >> 2;
			DPRINTF("OK\n");
			break;
		case R_PPC_COPY:
			/*
			 * Copy symbol data from shared object to specified
			 * location. Need to find the 'source', i.e. the
			 * other instance of the object than the one in the
			 * executable program.
			 */
			DPRINTF("fixup R_PPC_COPY (s)\n");

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
#if 0
				printf("Warning: Mismatched symbol sizes.\n");
#endif
				/* Take the lower value. */
				if (sym_size > sym_def->st_size)
					sym_size = sym_def->st_size;
			}

			memcpy(r_ptr, (const void *)sym_addr, sym_size);
			DPRINTF("OK\n");
			break;

		case R_PPC_JMP_SLOT:
			DPRINTF("fixup R_PPC_JMP_SLOT (S)\n");
			DPRINTF("r_offset=0x%zx r_addend=0x%zx\n",
			    r_offset, r_addend);

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

			DPRINTF("sym_addr = 0x%zx\n", sym_addr);
			DPRINTF("r_offset=0x%zx\n", r_offset);

			/*
			 * Fill PLT entry with jump to symbol address.
			 */
			plt_entry_init(plt, (uint32_t *)r_ptr, plt_datawords,
			    sym_addr);

			DPRINTF("OK\n");
			break;

		case R_PPC_RELATIVE:
			DPRINTF("fixup R_PPC_RELATIVE (B+A)\n");
			DPRINTF("*0x%zx = 0x%zx\n", (uintptr_t)r_ptr, m->bias + r_addend);
			*r_ptr = m->bias + r_addend;
			DPRINTF("OK\n");
			break;

		case R_PPC_DTPMOD32:
			DPRINTF("fixup R_PPC_DTPMOD32\n");
			DPRINTF("*0x%zx = 0x%zx\n", (uintptr_t)r_ptr, (size_t)dest->id);
			*r_ptr = dest->id;
			DPRINTF("OK\n");
			break;

		case R_PPC_DTPREL32:
			DPRINTF("fixup R_PPC_DTPREL32\n"); /* XXXXX */
			DPRINTF("*0x%zx = 0x%zx\n", (uintptr_t)r_ptr, sym_def->st_value);
			*r_ptr = sym_def->st_value + r_addend;
			DPRINTF("OK\n");
			break;

		default:
			printf("Error: Unknown relocation type %d\n",
			    rel_type);
			exit(1);
		}

	}
}

/** Init PLT farcall section. */
static void plt_farcall_init(uint32_t *plt, uint32_t *plt_datawords)
{
	uint16_t hi;
	uint16_t lo;
	int i;

	hi = addr_ha((uintptr_t)plt_datawords);
	lo = addr_l((uintptr_t)plt_datawords);

	plt[0] = 0x3d6b0000 | hi; /* addis %r11,% r11,. plt_datawords@ha */
	plt[1] = 0x816b0000 | lo; /* lwz %r11, .plt_datawords@l(%r11) */
	plt[2] = 0x7d6903a6;      /* mtctr %r11 */
	plt[3] = 0x4e800420;      /* bctr */
	plt[4] = 0x60000000;      /* nop */
	plt[5] = 0x60000000;      /* nop */

	smc_coherence(plt, 4 * 6);

	for (i = 0; i < 6; i++)
		DPRINTF("%p: farcall[%d] = %08zx\n", &plt[i], i, plt[i]);
}

/** Fill in PLT entry.
 *
 * Fill a PLT entry with PowerPC instructions to set table index and
 * jump to the farcall section. Fill table entry with target address.
 *
 * @param plt Pointer to PLT
 * @param plte Pointer to PLT entry to fill in
 * @param datawords Address table
 * @param ta Target address of the jump
 */
static void plt_entry_init(uint32_t *plt, uint32_t *plte, uint32_t *datawords,
    uintptr_t ta)
{
	size_t index;
	size_t woffset;
	uint16_t i4index;
	uint32_t btgt;

	DPRINTF("plt_entry_init(plt=%p, plte=%p, datawords=%p, ta=0z%zx\n",
	    plt, plte, datawords, ta);

	/* Entry offset in words */
	woffset = plte - plt;

	/* Entry index */
	index = plt_entry_index(woffset);

	/* This only works for the first 2048 entries */
	assert(index * 4 < 0x8000);
	i4index = 4 * index;

	/* Relative branch offset */
	btgt = ((uint8_t *)plt - (uint8_t *)&plte[1]) & 0x03ffffff;

	/* Write target address to table */
	datawords[index] = ta;
	DPRINTF("%p: datawords[%zu] = %08x\n", &datawords[index], index, ta);

	plte[0] = 0x39600000 | i4index; /* li %r11, 4 * index */
	plte[1] = 0x48000000 | btgt;    /* b .plt_farcall */

	DPRINTF("%p: plte[0] = %08zx\n", &plte[0], plte[0]);
	DPRINTF("%p: plte[1] = %08zx\n", &plte[1], plte[1]);

	smc_coherence(plte, 4 * 2);
}

/** Determine PLT entry address.
 *
 * @param plt Start of PLT
 * @param index PLT entry index
 * @return Pointer to PLT entry
 */
static uint32_t *plt_entry_ptr(uint32_t *plt, size_t index)
{
	if (index < 8192)
		return plt + 18 + 2 * index;
	else
		return plt + 18 + 2 * 8192 + 4 * (index - 8192);
}

/** Determine index of PLT entry from its word offset.
 *
 * @param woffset Offset of PLT entry in words
 * @return PLT entry index
 */
static size_t plt_entry_index(size_t woffset)
{
	assert(woffset >= 18);
	woffset -= 18;

	if (woffset < 2 * 8192) {
		assert((woffset & 0x1) == 0);
		return woffset / 2;
	} else {
		assert((woffset & 0x3) == 0);
		return (woffset - 2 * 8192) / 4;
	}
}

/** Determine high bits of address.
 *
 * The lower bits are determined by @c addr_l function. The lower bits
 * are considered to be a 16-bit signed integer.
 *
 * @param addr Address
 * @return Higher bits of address
 */
static uint16_t addr_ha(uint32_t addr)
{
	int32_t la;

	/* The lower part of the address is a signed 16-bit integer */
	la = (int16_t)(addr & 0xffff);

	/* Compute higher bits while compensating for the sign extension */
	return (addr - la) >> 16;
}

/** Determine lower bits of address.
 *
 * The lower bits are considered to be 16-bit signed integer/immediate
 * operand by the ISA, but we return them here as unsigned unmber so
 * it can be easily incorporated into an instruction opcode.
 *
 * @param addr Address
 * @return Lower bits of address cast as unsigned 16-bit integer
 */
static uint16_t addr_l(uint32_t addr)
{
	return (uint16_t) (addr & 0x0000ffff);
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
