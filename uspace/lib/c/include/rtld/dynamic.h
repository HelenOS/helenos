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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_RTLD_DYNAMIC_H_
#define _LIBC_RTLD_DYNAMIC_H_

#include <stdbool.h>
#include <rtld/elf_dyn.h>
#include <libarch/rtld/dynamic.h>

/**
 * Holds the data extracted from an ELF Dynamic section.
 *
 * The data is already pre-processed: Pointers are adjusted
 * to their final run-time values by adding the load bias
 * and indices into the symbol table are converted to pointers.
 */
typedef struct dyn_info {
	/** Relocation table without explicit addends */
	void *rel;
	size_t rel_sz;
	size_t rel_ent;

	/** Relocation table with explicit addends */
	void *rela;
	size_t rela_sz;
	size_t rela_ent;

	/** PLT relocation table */
	void *jmp_rel;
	size_t plt_rel_sz;
	/** Type of relocations used for the PLT, either DT_REL or DT_RELA */
	int plt_rel;

	/** Pointer to PLT/GOT (processor-specific) */
	void *plt_got;

	/** Hash table */
	elf_word *hash;

	/** String table */
	char *str_tab;
	size_t str_sz;

	/** Symbol table */
	void *sym_tab;
	size_t sym_ent;

	void *init;		/**< Module initialization code */
	void *fini;		/**< Module cleanup code */

	const char *soname;	/**< Library identifier */
	char *rpath;		/**< Library search path list */

	bool symbolic;
	bool text_rel;
	bool bind_now;

	/* Assume for now that there's at most one needed library */
	char *needed;

	/** Pointer to the module's dynamic section */
	elf_dyn_t *dynamic;

	/** Architecture-specific info. */
	dyn_info_arch_t arch;
} dyn_info_t;

void dynamic_parse(elf_dyn_t *dyn_ptr, size_t bias, dyn_info_t *info);
void dyn_parse_arch(elf_dyn_t *dp, size_t bias, dyn_info_t *info);

#endif

/** @}
 */
