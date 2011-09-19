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
 * Character string to module map.
 */

#ifndef LIBNET_MODULES_MAP_H_
#define LIBNET_MODULES_MAP_H_

#include <task.h>
#include <async.h>
#include <net/modules.h>
#include <adt/generic_char_map.h>

/** Type definition of the module structure.
 * @see module_struct
 */
typedef struct module_struct module_t;

/** Module map.
 * Sorted by module names.
 * @see generic_char_map.h
 */
GENERIC_CHAR_MAP_DECLARE(modules, module_t)

/** Module structure. */
struct module_struct {
	/** Module task identifier if running. */
	task_id_t task_id;
	/** Module service identifier. */
	services_t service;
	/** Module session if running and connected. */
	async_sess_t *sess;
	/** Usage counter. */
	int usage;
	/** Module name. */
	const uint8_t *name;
	/** Module full path filename. */
	const uint8_t *filename;
	/** Connecting function. */
	connect_module_t *connect_module;
};

extern int add_module(module_t **, modules_t *, const uint8_t *,
    const uint8_t *, services_t, task_id_t, connect_module_t *);
extern module_t *get_running_module(modules_t *, uint8_t *);
extern task_id_t net_spawn(const uint8_t *);

#endif

/** @}
 */
