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
 *  Character string to module map.
 */

#ifndef __NET_MODULES_MAP_H__
#define __NET_MODULES_MAP_H__

#include <task.h>

#include <ipc/services.h>

#include "../modules.h"

#include "generic_char_map.h"

/** Type definition of the module structure.
 *  @see module_struct
 */
typedef struct module_struct	module_t;

/** Type definition of the module structure pointer.
 *  @see module_struct
 */
typedef module_t *				module_ref;

/** Module map.
 *  Sorted by module names.
 *  @see generic_char_map.h
 */
GENERIC_CHAR_MAP_DECLARE(modules, module_t)

/** Module structure.
 */
struct	module_struct{
	/** Module task identifier if running.
	 */
	task_id_t task_id;
	/** Module service identifier.
	 */
	services_t service;
	/** Module phone if running and connected.
	 */
	int phone;
	/** Usage counter.
	 */
	int usage;
	/** Module name.
	 */
	const char * name;
	/** Module full path filename.
	 */
	const char * filename;
	/** Connecting function.
	 */
	connect_module_t * connect_module;
};

/** Adds module to the module map.
 *  @param[out] module The module structure added.
 *  @param[in] modules The module map.
 *  @param[in] name The module name.
 *  @param[in] filename The full path filename.
 *  @param[in] service The module service.
 *  @param[in] task_id The module current task identifier. Zero (0) means not running.
 *  @param[in] connect_module The module connecting function.
 *  @returns EOK on success.
 *  @returns ENOMEM if there is not enough memory left.
 */
int add_module(module_ref * module, modules_ref modules, const char * name, const char * filename, services_t service, task_id_t task_id, connect_module_t * connect_module);

/** Searches and returns the specified module.
 *  If the module is not running, the module filaname is spawned.
 *  If the module is not connected, the connect_function is called.
 *  @param[in] modules The module map.
 *  @param[in] name The module name.
 *  @returns The running module found. It does not have to be connected.
 *  @returns NULL if there is no such module.
 */
module_ref get_running_module(modules_ref modules, char * name);

/** Starts the given module.
 *  @param[in] fname The module full or relative path filename.
 *  @returns The new module task identifier on success.
 *  @returns 0 if there is no such module.
 */
task_id_t spawn(const char * fname);

#endif

/** @}
 */
