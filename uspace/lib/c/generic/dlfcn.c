/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup rtld
 * @brief
 * @{
 */
/**
 * @file
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <dlfcn.h>

#ifdef CONFIG_RTLD

#include <rtld/module.h>
#include <rtld/rtld.h>
#include <rtld/rtld_arch.h>
#include <rtld/symbol.h>

void *dlopen(const char *path, int flag)
{
	module_t *m;

	m = module_find(runtime_env, path);
	if (m == NULL) {
		m = module_load(runtime_env, path, mlf_local);
		if (m == NULL) {
			return NULL;
		}

		if (module_load_deps(m, mlf_local) != EOK) {
			return NULL;
		}

		/* Now relocate. */
		module_process_relocs(m);
	}

	return (void *) m;
}

/*
 * @note Symbols with NULL values are not accounted for.
 */
void *dlsym(void *mod, const char *sym_name)
{
	elf_symbol_t *sd;
	module_t *sm;

	sd = symbol_bfs_find(sym_name, (module_t *) mod, &sm);
	if (sd != NULL) {
		if (elf_st_type(sd->st_info) == STT_FUNC)
			return func_get_addr(sd, sm);
		else
			return symbol_get_addr(sd, sm, __tcb_get());
	}

	return NULL;
}

#else /* CONFIG_RTLD not defined */

void *dlopen(const char *path, int flag)
{
	return NULL;
}

void *dlsym(void *mod, const char *sym_name)
{
	return NULL;
}

#endif

/** @}
 */
