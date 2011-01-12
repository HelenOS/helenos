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

/** @addtogroup ip
 * @{
 */

/** @file
 * IP standalone module implementation.
 * Contains skeleton module functions mapping.
 * The functions are used by the module skeleton as module specific entry
 * points.
 *
 * @see module.c
 */

#include <async.h>
#include <stdio.h>
#include <ipc/ipc.h>
#include <ipc/services.h>
#include <errno.h>

#include <net/modules.h>
#include <net_interface.h>
#include <net/packet.h>
#include <il_local.h>

#include "ip.h"
#include "ip_module.h"

/** IP module global data. */
extern ip_globals_t ip_globals;

int
il_module_message(ipc_callid_t callid, ipc_call_t *call,
    ipc_call_t *answer, size_t *count)
{
	return ip_message_standalone(callid, call, answer, count);
}

int il_module_start(async_client_conn_t client_connection)
{
	sysarg_t phonehash;
	int rc;
	
	async_set_client_connection(client_connection);
	ip_globals.net_phone = net_connect_module();

	rc = pm_init();
	if (rc != EOK)
		return rc;
	
	rc = ip_initialize(client_connection);
	if (rc != EOK)
		goto out;
	
	rc = ipc_connect_to_me(PHONE_NS, SERVICE_IP, 0, 0, &phonehash);
	if (rc != EOK)
		goto out;
	
	async_manager();

out:
	pm_destroy();
	return rc;
}

/** @}
 */
