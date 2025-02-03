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
#include <rtld/module.h>
#include <rtld/rtld.h>
#include <rtld/rtld_debug.h>
#include <stdlib.h>
#include <str.h>

rtld_t *runtime_env;

/** Initialize and process an executable.
 *
 * @param p_info Program info
 * @return EOK on success or non-zero error code
 */
errno_t rtld_prog_process(elf_finfo_t *p_info, rtld_t **rre)
{
	rtld_t *env;
	bool is_dynamic = p_info->dynamic != NULL;
	DPRINTF("rtld_prog_process\n");

	/* Allocate new RTLD environment to pass to the loaded program */
	env = calloc(1, sizeof(rtld_t));
	if (env == NULL)
		return ENOMEM;

	list_initialize(&env->modules);
	list_initialize(&env->imodules);
	env->next_id = 1;

	module_t *module;
	errno_t rc = module_create_entrypoint(p_info, env, &module);
	if (rc != EOK) {
		free(env);
		return rc;
	}

	/* Pointer to program module. Used as root of the module graph. */
	env->program = module;

	/*
	 * Now we can continue with loading all other modules.
	 */

	if (is_dynamic) {
		DPRINTF("Load all program dependencies\n");
		rc = module_load_deps(module, 0);
		if (rc != EOK) {
			free(module);
			free(env);
			return rc;
		}
	}

	/* Compute static TLS size */
	modules_process_tls(env);

	/*
	 * Now relocate/link all modules together.
	 */

	if (is_dynamic) {
		/* Process relocations in all modules */
		DPRINTF("Relocate all modules\n");
		modules_process_relocs(env, module);
	}

	if (rre != NULL)
		*rre = env;
	return EOK;
}

/** Create TLS (Thread Local Storage) data structures.
 *
 * @return Pointer to TCB.
 */
tcb_t *rtld_tls_make(rtld_t *rtld)
{
	tcb_t *tcb;
	void **dtv;
	size_t nmods;
	size_t i;

	tcb = tls_alloc_arch(rtld->tls_size, rtld->tls_align);
	if (tcb == NULL)
		return NULL;

	/** Allocate dynamic thread vector */
	nmods = list_count(&rtld->imodules);
	dtv = malloc((nmods + 1) * sizeof(void *));
	if (dtv == NULL) {
		tls_free(tcb);
		return NULL;
	}

	/*
	 * We define generation number to be equal to vector length.
	 * We start with a vector covering the initially loaded modules.
	 */
	DTV_GN(dtv) = nmods;

	/*
	 * Copy thread local data from the initialization images of initial
	 * modules. Zero out thread-local uninitialized data.
	 */

	i = 1;
	list_foreach(rtld->imodules, imodules_link, module_t, m) {
		assert(i++ == m->id);

		dtv[m->id] = (void *) tcb + m->tpoff;

		assert(((uintptr_t) dtv[m->id]) % m->tls_align == 0);

		if (m->tdata)
			memcpy(dtv[m->id], m->tdata, m->tdata_size);

		memset(dtv[m->id] + m->tdata_size, 0, m->tbss_size);
	}

	tcb->dtv = dtv;
	return tcb;
}

unsigned long rtld_get_next_id(rtld_t *rtld)
{
	return rtld->next_id++;
}

/** Get address of thread-local variable.
 *
 * @param rtld RTLD instance
 * @param tcb TCB of the thread whose instance to return
 * @param mod_id Module ID
 * @param offset Offset within TLS block of the module
 *
 * @return Address of thread-local variable
 */
void *rtld_tls_get_addr(rtld_t *rtld, tcb_t *tcb, unsigned long mod_id,
    unsigned long offset)
{
	module_t *m;
	size_t dtv_len;
	void *tls_block;

	dtv_len = DTV_GN(tcb->dtv);
	if (dtv_len < mod_id) {
		/* Vector is short */

		tcb->dtv = realloc(tcb->dtv, (1 + mod_id) * sizeof(void *));
		/* XXX This can fail if OOM */
		assert(tcb->dtv != NULL);
		/* Zero out new part of vector */
		memset(tcb->dtv + (1 + dtv_len), 0, (mod_id - dtv_len) *
		    sizeof(void *));
	}

	if (tcb->dtv[mod_id] == NULL) {
		/* TLS block is not allocated */

		m = module_by_id(rtld, mod_id);
		assert(m != NULL);
		/* Should not be initial module, those have TLS pre-allocated */
		assert(!link_used(&m->imodules_link));

		tls_block = memalign(m->tls_align, m->tdata_size + m->tbss_size);
		/* XXX This can fail if OOM */
		assert(tls_block != NULL);

		/* Copy tdata */
		memcpy(tls_block, m->tdata, m->tdata_size);
		/* Zero out tbss */
		memset(tls_block + m->tdata_size, 0, m->tbss_size);

		tcb->dtv[mod_id] = tls_block;
	}

	return (uint8_t *)(tcb->dtv[mod_id]) + offset;
}

/** @}
 */
