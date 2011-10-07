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
 * @{
 */

/** @file
 * Wrapper for the standalone networking module.
 */

#include <str.h>
#include <adt/measured_strings.h>
#include <adt/module_map.h>
#include <ipc/net.h>
#include <errno.h>
#include <ip_interface.h>
#include "net.h"
#include "packet_server.h"

/** Networking module global data. */
extern net_globals_t net_globals;

/** Initialize the networking module for the chosen subsystem build type.
 *
 *  @param[in] client_connection The client connection processing function.
 *                               The module skeleton propagates its own one.
 *
 *  @return EOK on success.
 *  @return ENOMEM if there is not enough memory left.
 *
 */
int net_initialize_build(async_client_conn_t client_connection)
{
	int rc;
	
	task_id_t task_id = net_spawn((uint8_t *) IP_FILENAME);
	if (!task_id)
		return EINVAL;
	
	rc = add_module(NULL, &net_globals.modules, (uint8_t *) IP_NAME,
	    (uint8_t *) IP_FILENAME, SERVICE_IP, task_id, ip_connect_module);
	if (rc != EOK)
		return rc;
	
	if (!net_spawn((uint8_t *) "/srv/icmp"))
		return EINVAL;
	
	if (!net_spawn((uint8_t *) "/srv/udp"))
		return EINVAL;
	
	if (!net_spawn((uint8_t *) "/srv/tcp"))
		return EINVAL;
	
	return EOK;
}

/** Process the module message.
 *
 * Distribute the message to the right module.
 *
 * @param[in]  callid       The message identifier.
 * @param[in]  call         The message parameters.
 * @param[out] answer       The message answer parameters.
 * @param[out] answer_count The last parameter for the actual answer in
 *                          the answer parameter.
 *
 * @return EOK on success.
 * @return ENOTSUP if the message is not known.
 * @return Other error codes.
 *
 */
int net_module_message(ipc_callid_t callid, ipc_call_t *call,
    ipc_call_t *answer, size_t *count)
{
	if (IS_NET_PACKET_MESSAGE(*call))
		return packet_server_message(callid, call, answer, count);
	
	return net_message(callid, call, answer, count);
}

/** @}
 */
