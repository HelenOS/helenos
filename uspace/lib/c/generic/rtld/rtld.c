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
	module_t *prog;

	DPRINTF("Load dynamically linked program.\n");

	/* Allocate new RTLD environment to pass to the loaded program */
	env = calloc(1, sizeof(rtld_t));
	if (env == NULL)
		return ENOMEM;

	env->next_id = 1;

	prog = calloc(1, sizeof(module_t));
	if (prog == NULL) {
		free(env);
		return ENOMEM;
	}

	/*
	 * First we need to process dynamic sections of the executable
	 * program and insert it into the module graph.
	 */

	DPRINTF("Parse program .dynamic section at %p\n", p_info->dynamic);
	dynamic_parse(p_info->dynamic, 0, &prog->dyn);
	prog->bias = 0;
	prog->dyn.soname = "[program]";
	prog->rtld = env;
	prog->id = rtld_get_next_id(env);
	prog->exec = true;
	prog->local = false;

	prog->tdata = p_info->tls.tdata;
	prog->tdata_size = p_info->tls.tdata_size;
	prog->tbss_size = p_info->tls.tbss_size;

	printf("prog tdata at %p size %zu, tbss size %zu\n",
	    prog->tdata, prog->tdata_size, prog->tbss_size);

	/* Initialize list of loaded modules */
	list_initialize(&env->modules);
	list_append(&prog->modules_link, &env->modules);

	/* Pointer to program module. Used as root of the module graph. */
	env->program = prog;

	/* Work around non-existent memory space allocation. */
	env->next_bias = 0x1000000;

	/*
	 * Now we can continue with loading all other modules.
	 */

	DPRINTF("Load all program dependencies\n");
	module_load_deps(prog, 0);

	/*
	 * Now relocate/link all modules together.
	 */

	/* Process relocations in all modules */
	DPRINTF("Relocate all modules\n");
	modules_process_relocs(env, prog);

	modules_process_tls(env);

	*rre = env;
	return EOK;
}

/** Create TLS (Thread Local Storage) data structures.
 *
 * @return Pointer to TCB.
 */
tcb_t *rtld_tls_make(rtld_t *rtld)
{
	void *data;
	tcb_t *tcb;
	size_t offset;

	tcb = tls_alloc_arch(&data, rtld->tls_size);
	if (tcb == NULL)
		return NULL;

	/*
	 * Copy thread local data from the modules' initialization images.
	 * Zero out thread-local uninitialized data.
	 */

	offset = 0;
	list_foreach(rtld->modules, modules_link, module_t, m) {
		assert(offset + m->tdata_size + m->tbss_size <= rtld->tls_size);
		memcpy(data + offset, m->tdata, m->tdata_size);
		offset += m->tdata_size;
		memset(data + offset, 0, m->tbss_size);
		offset += m->tbss_size;
	}

	return tcb;
}

unsigned long rtld_get_next_id(rtld_t *rtld)
{
	return rtld->next_id++;
}

void *rtld_tls_get_addr(rtld_t *rtld, void *tls, unsigned long mod_id,
    unsigned long offset)
{
	module_t *m;

	m = module_by_id(rtld, mod_id);
	assert(m != NULL);

	return tls + m->ioffs + offset;
}


/** @}
 */
