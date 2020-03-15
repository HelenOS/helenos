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
