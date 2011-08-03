/*
 * Copyright (c) 2009 Lukas Mejdrech
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

/** @addtogroup libnet
 * @{
 */

/** @file
 * Character string to module map implementation.
 */

#include <malloc.h>
#include <task.h>
#include <unistd.h>
#include <errno.h>
#include <net/modules.h>
#include <adt/generic_char_map.h>
#include <adt/module_map.h>

GENERIC_CHAR_MAP_IMPLEMENT(modules, module_t)

/** Add module to the module map.
 *
 * @param[out] module         Module structure added.
 * @param[in]  modules        Module map.
 * @param[in]  name           Module name.
 * @param[in]  filename       Full path filename.
 * @param[in]  service        Module service.
 * @param[in]  task_id        Module current task identifier.
 *                            Zero means not running.
 * @param[in]  connect_module Module connecting function.
 *
 * @return EOK on success.
 * @return ENOMEM if there is not enough memory left.
 *
 */
int add_module(module_t **module, modules_t *modules, const uint8_t *name,
    const uint8_t *filename, services_t service, task_id_t task_id,
    connect_module_t connect_module)
{
	module_t *tmp_module;
	int rc;
	
	tmp_module = (module_t *) malloc(sizeof(module_t));
	if (!tmp_module)
		return ENOMEM;
	
	tmp_module->task_id = task_id;
	tmp_module->sess = NULL;
	tmp_module->usage = 0;
	tmp_module->name = name;
	tmp_module->filename = filename;
	tmp_module->service = service;
	tmp_module->connect_module = connect_module;
	
	rc = modules_add(modules, tmp_module->name, 0, tmp_module);
	if (rc != EOK) {
		free(tmp_module);
		return rc;
	}
	
	if (module)
		*module = tmp_module;
	
	return EOK;
}

/** Search and return the specified module.
 *
 * If the module is not running, the module filaname is spawned.
 * If the module is not connected, the connect_function is called.
 *
 * @param[in] modules Module map.
 * @param[in] name    Module name.
 *
 * @return The running module found. It does not have to be
 *         connected.
 * @return NULL if there is no such module.
 *
 */
module_t *get_running_module(modules_t *modules, uint8_t *name)
{
	module_t *module = modules_find(modules, name, 0);
	if (!module)
		return NULL;
	
	if (!module->task_id) {
		module->task_id = net_spawn(module->filename);
		if (!module->task_id)
			return NULL;
	}
	
	if (!module->sess)
		module->sess = module->connect_module(module->service);
	
	return module;
}

/** Start the given module.
 *
 * @param[in] fname The module full or relative path filename.
 *
 * @return The new module task identifier on success.
 * @return Zero if there is no such module.
 *
 */
task_id_t net_spawn(const uint8_t *fname)
{
	task_id_t id;
	int rc;
	
	rc = task_spawnl(&id, (const char *) fname, (const char *) fname, NULL);
	if (rc != EOK)
		return 0;
	
	return id;
}

/** @}
 */
