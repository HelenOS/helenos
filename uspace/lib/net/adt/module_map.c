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

/** @addtogroup net
 *  @{
 */

/** @file
 *  Character string to module map implementation.
 */

#include <malloc.h>
#include <task.h>
#include <unistd.h>

#include <ipc/services.h>

#include <net_err.h>
#include <net_modules.h>

#include <adt/generic_char_map.h>
#include <adt/module_map.h>

GENERIC_CHAR_MAP_IMPLEMENT(modules, module_t)

int add_module(module_ref * module, modules_ref modules, const char * name, const char * filename, services_t service, task_id_t task_id, connect_module_t connect_module){
	ERROR_DECLARE;

	module_ref tmp_module;

	tmp_module = (module_ref) malloc(sizeof(module_t));
	if(! tmp_module){
		return ENOMEM;
	}
	tmp_module->task_id = task_id;
	tmp_module->phone = 0;
	tmp_module->usage = 0;
	tmp_module->name = name;
	tmp_module->filename = filename;
	tmp_module->service = service;
	tmp_module->connect_module = connect_module;
	if(ERROR_OCCURRED(modules_add(modules, tmp_module->name, 0, tmp_module))){
		free(tmp_module);
		return ERROR_CODE;
	}
	if(module){
		*module = tmp_module;
	}
	return EOK;
}

module_ref get_running_module(modules_ref modules, char * name){
	module_ref module;

	module = modules_find(modules, name, 0);
	if(! module){
		return NULL;
	}
	if(! module->task_id){
		module->task_id = spawn(module->filename);
		if(! module->task_id){
			return NULL;
		}
	}
	if(! module->phone){
		module->phone = module->connect_module(module->service);
	}
	return module;
}

task_id_t spawn(const char * fname){
	const char * argv[2];
	task_id_t res;

	argv[0] = fname;
	argv[1] = NULL;
	res = task_spawn(fname, argv);

	return res;
}

/** @}
 */
