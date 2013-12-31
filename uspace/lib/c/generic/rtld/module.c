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

#include <adt/list.h>
#include <elf/elf_load.h>
#include <fcntl.h>
#include <loader/pcb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <rtld/rtld.h>
#include <rtld/rtld_debug.h>
#include <rtld/dynamic.h>
#include <rtld/rtld_arch.h>
#include <rtld/module.h>

/** (Eagerly) process all relocation tables in a module.
 *
 * Currently works as if LD_BIND_NOW was specified.
 */
void module_process_relocs(module_t *m)
{
	DPRINTF("module_process_relocs('%s')\n", m->dyn.soname);

	/* Do not relocate twice. */
	if (m->relocated) return;

	module_process_pre_arch(m);

	if (m->dyn.plt_rel == DT_REL) {
		DPRINTF("table type DT_REL\n");
		if (m->dyn.rel != NULL) {
			DPRINTF("non-empty\n");
			rel_table_process(m, m->dyn.rel, m->dyn.rel_sz);
		}
		/* FIXME: this seems wrong */
		if (m->dyn.jmp_rel != NULL) {
		DPRINTF("table type jmp-rel\n");
			DPRINTF("non-empty\n");
			rel_table_process(m, m->dyn.jmp_rel, m->dyn.plt_rel_sz);
		}
	} else { /* (m->dyn.plt_rel == DT_RELA) */
		DPRINTF("table type DT_RELA\n");
		if (m->dyn.rela != NULL) {
			DPRINTF("non-empty\n");
			rela_table_process(m, m->dyn.rela, m->dyn.rela_sz);
		}
	}

	m->relocated = true;
}

/** Find module structure by soname/pathname.
 *
 * Used primarily to see if a module has already been loaded.
 * Modules are compared according to their soname, i.e. possible
 * path components are ignored.
 */
module_t *module_find(const char *name)
{
	const char *p, *soname;

	DPRINTF("module_find('%s')\n", name);

	/*
	 * If name contains slashes, treat it as a pathname and
	 * construct soname by chopping off the path. Otherwise
	 * treat it as soname.
	 */
	p = str_rchr(name, '/');
	soname = p ? (p + 1) : name;

	/* Traverse list of all modules. Not extremely fast, but simple */
	list_foreach(runtime_env->modules, modules_link, module_t, m) {
		DPRINTF("m = %p\n", m);
		if (str_cmp(m->dyn.soname, soname) == 0) {
			return m; /* Found */
		}
	}
	
	return NULL; /* Not found */
}

#define NAME_BUF_SIZE 64

/** Load a module.
 *
 * Currently this trivially tries to load '/<name>'.
 */
module_t *module_load(const char *name)
{
	elf_info_t info;
	char name_buf[NAME_BUF_SIZE];
	module_t *m;
	int rc;
	
	m = malloc(sizeof(module_t));
	if (!m) {
		printf("malloc failed\n");
		exit(1);
	}

	if (str_size(name) > NAME_BUF_SIZE - 2) {
		printf("soname too long. increase NAME_BUF_SIZE\n");
		exit(1);
	}

	/* Prepend soname with '/lib/' */
	str_cpy(name_buf, NAME_BUF_SIZE, "/lib/");
	str_cpy(name_buf + 5, NAME_BUF_SIZE - 5, name);

	/* FIXME: need to real allocation of address space */
	m->bias = runtime_env->next_bias;
	runtime_env->next_bias += 0x100000;

	DPRINTF("filename:'%s'\n", name_buf);
	DPRINTF("load '%s' at 0x%x\n", name_buf, m->bias);

	rc = elf_load_file(name_buf, m->bias, ELDF_RW, &info);
	if (rc != EE_OK) {
		printf("Failed to load '%s'\n", name_buf);
		exit(1);
	}

	if (info.dynamic == NULL) {
		printf("Error: '%s' is not a dynamically-linked object.\n",
		    name_buf);
		exit(1);
	}

	/* Pending relocation. */
	m->relocated = false;

	DPRINTF("parse dynamic section\n");
	/* Parse ELF .dynamic section. Store info to m->dyn. */
	dynamic_parse(info.dynamic, m->bias, &m->dyn);

	/* Insert into the list of loaded modules */
	list_append(&m->modules_link, &runtime_env->modules);

	return m;
}

/** Load all modules on which m (transitively) depends.
 */
void module_load_deps(module_t *m)
{
	elf_dyn_t *dp;
	char *dep_name;
	module_t *dm;
	size_t n, i;

	DPRINTF("module_load_deps('%s')\n", m->dyn.soname);

	/* Count direct dependencies */
	
	dp = m->dyn.dynamic;
	n = 0;

	while (dp->d_tag != DT_NULL) {
		if (dp->d_tag == DT_NEEDED) ++n;
		++dp;
	}

	/* Create an array of pointers to direct dependencies */

	m->n_deps = n;

	if (n == 0) {
		/* There are no dependencies, so we are done. */
		m->deps = NULL;
		return;
	}

	m->deps = malloc(n * sizeof(module_t *));
	if (!m->deps) {
		printf("malloc failed\n");
		exit(1);
	}

	i = 0; /* Current dependency index */
	dp = m->dyn.dynamic;

	while (dp->d_tag != DT_NULL) {
		if (dp->d_tag == DT_NEEDED) {
			dep_name = m->dyn.str_tab + dp->d_un.d_val;

			DPRINTF("%s needs %s\n", m->dyn.soname, dep_name);
			dm = module_find(dep_name);
			if (!dm) {
				dm = module_load(dep_name);
				module_load_deps(dm);
			}

			/* Save into deps table */
			m->deps[i++] = dm;
		}
		++dp;
	}
}

/** Process relocations in modules.
 *
 * Processes relocations in @a start and all its dependencies.
 * Modules that have already been relocated are unaffected.
 *
 * @param	start	The module where to start from.
 */
void modules_process_relocs(module_t *start)
{
	list_foreach(runtime_env->modules, modules_link, module_t, m) {
		/* Skip rtld, since it has already been processed */
		if (m != &runtime_env->rtld) {
			module_process_relocs(m);
		}
	}
}

/** Clear BFS tags of all modules.
 */
void modules_untag(void)
{
	list_foreach(runtime_env->modules, modules_link, module_t, m) {
		m->bfs_tag = false;
	}
}

/** @}
 */
