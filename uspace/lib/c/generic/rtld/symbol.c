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

#include <elf/elf.h>
#include <rtld/rtld.h>
#include <rtld/rtld_debug.h>
#include <rtld/symbol.h>

/*
 * Hash tables are 32-bit (elf_word) even for 64-bit ELF files.
 */
static elf_word elf_hash(const unsigned char *name)
{
	elf_word h = 0, g;

	while (*name) {
		h = (h << 4) + *name++;
		g = h & 0xf0000000;
		if (g != 0) h ^= g >> 24;
		h &= ~g;
	}

	return h;
}

static elf_symbol_t *def_find_in_module(const char *name, module_t *m)
{
	elf_symbol_t *sym_table;
	elf_symbol_t *s, *sym;
	elf_word nbucket;
	/*elf_word nchain;*/
	elf_word i;
	char *s_name;
	elf_word bucket;

	DPRINTF("def_find_in_module('%s', %s)\n", name, m->dyn.soname);

	sym_table = m->dyn.sym_tab;
	nbucket = m->dyn.hash[0];
	/*nchain = m->dyn.hash[1]; XXX Use to check HT range*/

	bucket = elf_hash((unsigned char *)name) % nbucket;
	i = m->dyn.hash[2 + bucket];

	sym = NULL;
	while (i != STN_UNDEF) {
		s = &sym_table[i];
		s_name = m->dyn.str_tab + s->st_name;

		if (str_cmp(name, s_name) == 0) {
			sym = s;
			break;
		}

		i = m->dyn.hash[2 + nbucket + i];
	}

	if (!sym)
		return NULL;	/* Not found */

	if (sym->st_shndx == SHN_UNDEF) {
		/* Not a definition */
		return NULL;
	}

	return sym; /* Found */
}

/** Find the definition of a symbol in a module and its deps.
 *
 * Search the module dependency graph is breadth-first, beginning
 * from the module @a start. Thus, @start and all its dependencies
 * get searched.
 *
 * @param name		Name of the symbol to search for.
 * @param start		Module in which to start the search..
 * @param mod		(output) Will be filled with a pointer to the module 
 *			that contains the symbol.
 */
elf_symbol_t *symbol_bfs_find(const char *name, module_t *start, module_t **mod)
{
	module_t *m, *dm;
	elf_symbol_t *sym, *s;
	list_t queue;
	size_t i;

	/*
	 * Do a BFS using the queue_link and bfs_tag fields.
	 * Vertices (modules) are tagged the moment they are inserted
	 * into the queue. This prevents from visiting the same vertex
	 * more times in case of circular dependencies.
	 */

	/* Mark all vertices (modules) as unvisited */	
	modules_untag();

	/* Insert root (the program) into the queue and tag it */
	list_initialize(&queue);
	start->bfs_tag = true;
	list_append(&start->queue_link, &queue);

	/* If the symbol is found, it will be stored in 'sym' */
	sym = NULL;

	/* While queue is not empty */
	while (!list_empty(&queue)) {
		/* Pop first element from the queue */
		m = list_get_instance(list_first(&queue), module_t, queue_link);
		list_remove(&m->queue_link);

		s = def_find_in_module(name, m);
		if (s != NULL) {
			/* Symbol found */
			sym = s;
			*mod = m;
			break;
		}

		/*
		 * Insert m's untagged dependencies into the queue
		 * and tag them.
		 */
		for (i = 0; i < m->n_deps; ++i) {
			dm = m->deps[i];

			if (dm->bfs_tag == false) {
				dm->bfs_tag = true;
				list_append(&dm->queue_link, &queue);
			}
		}
	}

	/* Empty the queue so that we leave it in a clean state */
	while (!list_empty(&queue))
		list_remove(list_first(&queue));

	if (!sym) {
		return NULL; /* Not found */
	}

	return sym; /* Symbol found */
}


/** Find the definition of a symbol..
 *
 * By definition in System V ABI, if module origin has the flag DT_SYMBOLIC,
 * origin is searched first. Otherwise, or if the symbol hasn't been found,
 * the module dependency graph is searched breadth-first, beginning
 * from the executable program.
 *
 * @param name		Name of the symbol to search for.
 * @param origin	Module in which the dependency originates.
 * @param mod		(output) Will be filled with a pointer to the module 
 *			that contains the symbol.
 */
elf_symbol_t *symbol_def_find(const char *name, module_t *origin, module_t **mod)
{
	elf_symbol_t *s;

	if (origin->dyn.symbolic) {
		/* 
		 * Origin module has a DT_SYMBOLIC flag.
		 * Try this module first
		 */
		 s = def_find_in_module(name, origin);
		 if (s != NULL) {
			/* Found */
			*mod = origin;
			return s;
		 }
	}

	/* Not DT_SYMBOLIC or no match. Now try other locations. */

	if (runtime_env->program) {
		/* Program is dynamic -- start with program as root. */
		return symbol_bfs_find(name, runtime_env->program, mod);
	} else {
		/* Program is static -- start with @a origin as root. */
		return symbol_bfs_find(name, origin, mod);
	}
}

void *symbol_get_addr(elf_symbol_t *sym, module_t *m)
{
	if (sym->st_shndx == SHN_ABS) {
		/* Do not add bias to absolute symbols */
		return (void *) sym->st_value;
	} else {
		return (void *) (sym->st_value + m->bias);
	}
}

/** @}
 */
