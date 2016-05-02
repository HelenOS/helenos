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

#include <errno.h>
#include <rtld/module.h>
#include <rtld/rtld.h>
#include <rtld/rtld_debug.h>
#include <stdlib.h>

rtld_t *runtime_env;
static rtld_t rt_env_static;
static module_t prog_mod;

/** Initialize the runtime linker for use in a statically-linked executable. */
void rtld_init_static(void)
{
	runtime_env = &rt_env_static;
	list_initialize(&runtime_env->modules);
	runtime_env->next_bias = 0x2000000;
	runtime_env->program = NULL;
}

/** Initialize and process a dynamically linked executable.
 *
 * @param p_info Program info
 * @return EOK on success or non-zero error code
 */
int rtld_prog_process(elf_finfo_t *p_info, rtld_t **rre)
{
	rtld_t *env;

	DPRINTF("Load dynamically linked program.\n");

	/* Allocate new RTLD environment to pass to the loaded program */
	env = calloc(1, sizeof(rtld_t));
	if (env == NULL)
		return ENOMEM;

	/*
	 * First we need to process dynamic sections of the executable
	 * program and insert it into the module graph.
	 */

	DPRINTF("Parse program .dynamic section at %p\n", p_info->dynamic);
	dynamic_parse(p_info->dynamic, 0, &prog_mod.dyn);
	prog_mod.bias = 0;
	prog_mod.dyn.soname = "[program]";
	prog_mod.rtld = env;
	prog_mod.exec = true;
	prog_mod.local = false;

	/* Initialize list of loaded modules */
	list_initialize(&env->modules);
	list_append(&prog_mod.modules_link, &env->modules);

	/* Pointer to program module. Used as root of the module graph. */
	env->program = &prog_mod;

	/* Work around non-existent memory space allocation. */
	env->next_bias = 0x1000000;

	/*
	 * Now we can continue with loading all other modules.
	 */

	DPRINTF("Load all program dependencies\n");
	module_load_deps(&prog_mod, 0);

	/*
	 * Now relocate/link all modules together.
	 */

	/* Process relocations in all modules */
	DPRINTF("Relocate all modules\n");
	modules_process_relocs(env, &prog_mod);

	*rre = env;
	return EOK;
}

/** @}
 */
