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
#include <inttypes.h>
#include <str.h>

#include <rtld/elf_dyn.h>
#include <rtld/dynamic.h>
#include <rtld/rtld.h>
#include <rtld/rtld_debug.h>

void dynamic_parse(elf_dyn_t *dyn_ptr, size_t bias, dyn_info_t *info)
{
	elf_dyn_t *dp = dyn_ptr;

	void *d_ptr;
	elf_word d_val;

	elf_word soname_idx;
	elf_word rpath_idx;

	DPRINTF("memset\n");
	memset(info, 0, sizeof(dyn_info_t));

	soname_idx = 0;
	rpath_idx = 0;

	DPRINTF("pass 1\n");
	while (dp->d_tag != DT_NULL) {
		d_ptr = (void *)((uint8_t *)dp->d_un.d_ptr + bias);
		d_val = dp->d_un.d_val;
		DPRINTF("tag=%u ptr=0x%x val=%u\n", (unsigned)dp->d_tag,
			(unsigned)d_ptr, (unsigned)d_val);

		switch (dp->d_tag) {

		case DT_PLTRELSZ:	info->plt_rel_sz = d_val; break;
		case DT_PLTGOT:		info->plt_got = d_ptr; break;
		case DT_HASH:		info->hash = d_ptr; break;
		case DT_STRTAB:		info->str_tab = d_ptr; break;
		case DT_SYMTAB:		info->sym_tab = d_ptr; break;
		case DT_RELA:		info->rela = d_ptr; break;
		case DT_RELASZ:		info->rela_sz = d_val; break;
		case DT_RELAENT:	info->rela_ent = d_val; break;
		case DT_STRSZ:		info->str_sz = d_val; break;
		case DT_SYMENT:		info->sym_ent = d_val; break;
		case DT_INIT:		info->init = d_ptr; break;
		case DT_FINI:		info->fini = d_ptr; break;
		case DT_SONAME:		soname_idx = d_val; break;
		case DT_RPATH:		rpath_idx = d_val; break;
		case DT_SYMBOLIC:	info->symbolic = true; break;
		case DT_REL:		info->rel = d_ptr; break;
		case DT_RELSZ:		info->rel_sz = d_val; break;
		case DT_RELENT:		info->rel_ent = d_val; break;
		case DT_PLTREL:		info->plt_rel = d_val; break;
		case DT_TEXTREL:	info->text_rel = true; break;
		case DT_JMPREL:		info->jmp_rel = d_ptr; break;
		case DT_BIND_NOW:	info->bind_now = true; break;

		default:
			if (dp->d_tag >= DT_LOPROC && dp->d_tag <= DT_HIPROC)
				dyn_parse_arch(dp, bias, info);
			break;
		}

		++dp;
	}

	info->soname = info->str_tab + soname_idx;
	info->rpath = info->str_tab + rpath_idx;

	/* This will be useful for parsing dependencies later */
	info->dynamic = dyn_ptr;

	DPRINTF("str_tab=0x%" PRIxPTR ", soname_idx=0x%x, soname=0x%" PRIxPTR "\n",
		(uintptr_t)info->soname, soname_idx, (uintptr_t)info->soname);
	DPRINTF("soname='%s'\n", info->soname);
	DPRINTF("rpath='%s'\n", info->rpath);
	DPRINTF("hash=0x%" PRIxPTR "\n", (uintptr_t)info->hash);
	DPRINTF("dt_rela=0x%" PRIxPTR "\n", (uintptr_t)info->rela);
	DPRINTF("dt_rela_sz=0x%" PRIxPTR "\n", (uintptr_t)info->rela_sz);
	DPRINTF("dt_rel=0x%" PRIxPTR "\n", (uintptr_t)info->rel);
	DPRINTF("dt_rel_sz=0x%" PRIxPTR "\n", (uintptr_t)info->rel_sz);

	/*
	 * Now that we have a pointer to the string table,
	 * we can parse DT_NEEDED fields (which contain offsets into it).
	 */

	DPRINTF("pass 2\n");
	dp = dyn_ptr;
	while (dp->d_tag != DT_NULL) {
		d_val = dp->d_un.d_val;

		switch (dp->d_tag) {
		case DT_NEEDED:
			/* Assume just for now there's only one dependency */
			info->needed = info->str_tab + d_val;
			DPRINTF("needed:'%s'\n", info->needed);
			break;

		default: break;
		}

		++dp;
	}
}

/** @}
 */
