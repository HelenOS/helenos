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

/** @addtogroup nildummy
 * @{
 */

/** @file
 *  Dummy network interface layer module stub.
 *  @see module.c
 */

#include <async.h>
#include <stdio.h>
#include <err.h>

#include <ipc/ipc.h>
#include <ipc/services.h>

#include <net/modules.h>
#include <net_interface.h>
#include <net/packet.h>
#include <nil_local.h>

#include "nildummy.h"

int nil_module_start_standalone(async_client_conn_t client_connection)
{
	ERROR_DECLARE;
	
	async_set_client_connection(client_connection);
	int net_phone = net_connect_module();
	ERROR_PROPAGATE(pm_init());
	
	ipcarg_t phonehash;
	if (ERROR_OCCURRED(nil_initialize(net_phone))
	    || ERROR_OCCURRED(REGISTER_ME(SERVICE_NILDUMMY, &phonehash))){
		pm_destroy();
		return ERROR_CODE;
	}
	
	async_manager();
	
	pm_destroy();
	return EOK;
}

int nil_module_message_standalone(const char *name, ipc_callid_t callid,
    ipc_call_t *call, ipc_call_t *answer, int *answer_count)
{
	return nil_message_standalone(name, callid, call, answer, answer_count);
}

/** @}
 */
