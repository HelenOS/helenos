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

/** @addtogroup netif
 *  @{
 */

/** @file
 *  Wrapper for the bundled network interface and network interface layer module.
 *  Distributes messages and initializes both module parts.
 */

#include <async.h>
#include <ipc/ipc.h>

#include <net_messages.h>
#include <packet/packet.h>
#include <nil_module.h>
#include <netif_nil_bundle.h>
#include <netif.h>

/** Distributes the messages between the module parts.
 *  @param[in] callid The message identifier.
 *  @param[in] call The message parameters.
 *  @param[out] answer The message answer parameters.
 *  @param[out] answer_count The last parameter for the actual answer in the answer parameter.
 *  @returns EOK on success.
 *  @returns ENOTSUP if the message is not known.
 *  @returns Other error codes as defined for each specific module message function.
 */
int netif_nil_module_message(ipc_callid_t callid, ipc_call_t * call, ipc_call_t * answer, int * answer_count){
	if(IS_NET_NIL_MESSAGE(call) || (IPC_GET_METHOD(*call) == IPC_M_CONNECT_TO_ME)){
		return nil_message(callid, call, answer, answer_count);
	}else{
		return netif_message(callid, call, answer, answer_count);
	}
}

/** Starts the bundle network interface module.
 *  Initializes the client connection serving function, initializes both module parts, registers the module service and starts the async manager, processing IPC messages in an infinite loop.
 *  @param[in] client_connection The client connection processing function. The module skeleton propagates its own one.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for each specific module message function.
 */
int netif_nil_module_start(async_client_conn_t client_connection){
	ERROR_DECLARE;

	ERROR_PROPAGATE(netif_init_module(client_connection));
	if(ERROR_OCCURRED(nil_initialize(netif_globals.net_phone))){
		pm_destroy();
		return ERROR_CODE;
	}
	return netif_run_module();
}

/** @}
 */
