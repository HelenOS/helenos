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
#include <errno.h>
#include <loader/pcb.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>

#include <rtld/rtld.h>
#include <rtld/rtld_debug.h>
#include <rtld/dynamic.h>
#include <rtld/rtld_arch.h>
#include <rtld/module.h>

/** Create module for static executable.
 *
 * @param rtld Run-time dynamic linker
 * @param rmodule Place to store pointer to new module or @c NULL
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t module_create_static_exec(rtld_t *rtld, module_t **rmodule)
{
	module_t *module;

	module = calloc(1, sizeof(module_t));
	if (module == NULL)
		return ENOMEM;

	module->id = rtld_get_next_id(rtld);
	module->dyn.soname = "[program]";

	module->rtld = rtld;
	module->exec = true;
	module->local = true;

	module->tdata = &_tdata_start;
	module->tdata_size = &_tdata_end - &_tdata_start;
	module->tbss_size = &_tbss_end - &_tbss_start;
	module->tls_align = (uintptr_t)&_tls_alignment;

	list_append(&module->modules_link, &rtld->modules);

	if (rmodule != NULL)
		*rmodule = module;
	return EOK;
}

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

	/* jmp_rel table */
	if (m->dyn.jmp_rel != NULL) {
		DPRINTF("jmp_rel table\n");
		if (m->dyn.plt_rel == DT_REL) {
			DPRINTF("jmp_rel table type DT_REL\n");
			rel_table_process(m, m->dyn.jmp_rel, m->dyn.plt_rel_sz);
		} else {
			assert(m->dyn.plt_rel == DT_RELA);
			DPRINTF("jmp_rel table type DT_RELA\n");
			rela_table_process(m, m->dyn.jmp_rel, m->dyn.plt_rel_sz);
		}
	}

	/* rel table */
	if (m->dyn.rel != NULL) {
		DPRINTF("rel table\n");
		rel_table_process(m, m->dyn.rel, m->dyn.rel_sz);
	}

	/* rela table */
	if (m->dyn.rela != NULL) {
		DPRINTF("rela table\n");
		rela_table_process(m, m->dyn.rela, m->dyn.rela_sz);
	}

	m->relocated = true;
}

/** Find module structure by soname/pathname.
 *
 * Used primarily to see if a module has already been loaded.
 * Modules are compared according to their soname, i.e. possible
 * path components are ignored.
 */
module_t *module_find(rtld_t *rtld, const char *name)
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
	list_foreach(rtld->modules, modules_link, module_t, m) {
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
module_t *module_load(rtld_t *rtld, const char *name, mlflags_t flags)
{
	elf_finfo_t info;
	char name_buf[NAME_BUF_SIZE];
	module_t *m;
	int rc;

	m = calloc(1, sizeof(module_t));
	if (m == NULL) {
		printf("malloc failed\n");
		exit(1);
	}

	m->rtld = rtld;
	m->id = rtld_get_next_id(rtld);

	if ((flags & mlf_local) != 0)
		m->local = true;

	if (str_size(name) > NAME_BUF_SIZE - 2) {
		printf("soname too long. increase NAME_BUF_SIZE\n");
		exit(1);
	}

	/* Prepend soname with '/lib/' */
	str_cpy(name_buf, NAME_BUF_SIZE, "/lib/");
	str_cpy(name_buf + 5, NAME_BUF_SIZE - 5, name);

	/* FIXME: need to real allocation of address space */
	m->bias = rtld->next_bias;
	rtld->next_bias += 0x100000;

	DPRINTF("filename:'%s'\n", name_buf);
	DPRINTF("load '%s' at 0x%zx\n", name_buf, m->bias);

	rc = elf_load_file_name(name_buf, m->bias, ELDF_RW, &info);
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
	list_append(&m->modules_link, &rtld->modules);

	/* Copy TLS info */
	m->tdata = info.tls.tdata;
	m->tdata_size = info.tls.tdata_size;
	m->tbss_size = info.tls.tbss_size;
	m->tls_align = info.tls.tls_align;

	DPRINTF("tdata at %p size %zu, tbss size %zu\n",
	    m->tdata, m->tdata_size, m->tbss_size);

	return m;
}

/** Load all modules on which m (transitively) depends.
 */
void module_load_deps(module_t *m, mlflags_t flags)
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
			dm = module_find(m->rtld, dep_name);
			if (!dm) {
				dm = module_load(m->rtld, dep_name, flags);
				module_load_deps(dm, flags);
			}

			/* Save into deps table */
			m->deps[i++] = dm;
		}
		++dp;
	}
}

/** Find module structure by ID. */
module_t *module_by_id(rtld_t *rtld, unsigned long id)
{
	list_foreach(rtld->modules, modules_link, module_t, m) {
		if (m->id == id)
			return m;
	}

	return NULL;
}

/** Process relocations in modules.
 *
 * Processes relocations in @a start and all its dependencies.
 * Modules that have already been relocated are unaffected.
 *
 * @param	start	The module where to start from.
 */
void modules_process_relocs(rtld_t *rtld, module_t *start)
{
	list_foreach(rtld->modules, modules_link, module_t, m) {
		/* Skip rtld module, since it has already been processed */
		if (m != &rtld->rtld) {
			module_process_relocs(m);
		}
	}
}

void modules_process_tls(rtld_t *rtld)
{
#ifdef CONFIG_TLS_VARIANT_1
	list_foreach(rtld->modules, modules_link, module_t, m) {
		m->ioffs = rtld->tls_size;
		list_append(&m->imodules_link, &rtmd->imodules);
		rtld->tls_size += m->tdata_size + m->tbss_size;
	}
#else /* CONFIG_TLS_VARIANT_2 */
	size_t offs;

	list_foreach(rtld->modules, modules_link, module_t, m) {
		rtld->tls_size += m->tdata_size + m->tbss_size;
	}

	offs = 0;
	list_foreach(rtld->modules, modules_link, module_t, m) {
		offs += m->tdata_size + m->tbss_size;
		m->ioffs = rtld->tls_size - offs;
		list_append(&m->imodules_link, &rtld->imodules);
	}
#endif
}

/** Clear BFS tags of all modules.
 */
void modules_untag(rtld_t *rtld)
{
	list_foreach(rtld->modules, modules_link, module_t, m) {
		m->bfs_tag = false;
	}
}

/** @}
 */
