/*
 * Copyright (c) 2011 Martin Decky
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <ns.h>
#include <ipc/ns.h>
#include <async.h>
#include <macros.h>
#include <errno.h>
#include "private/ns.h"

int service_register(sysarg_t service)
{
	async_exch_t *exch = async_exchange_begin(session_ns);
	int rc = async_connect_to_me(exch, service, 0, 0, NULL, NULL);
	async_exchange_end(exch);
	
	return rc;
}

async_sess_t *service_connect(exch_mgmt_t mgmt, services_t service, sysarg_t arg2,
    sysarg_t arg3)
{
	async_exch_t *exch = async_exchange_begin(session_ns);
	if (!exch)
		return NULL;
	
	async_sess_t *sess =
	    async_connect_me_to(mgmt, exch, service, arg2, arg3);
	async_exchange_end(exch);
	
	if (!sess)
		return NULL;
	
	/*
	 * FIXME Ugly hack to work around limitation of implementing
	 * parallel exchanges using multiple connections. Shift out
	 * first argument for non-initial connections.
	 */
	async_sess_args_set(sess, arg2, arg3, 0);
	
	return sess;
}

async_sess_t *service_connect_blocking(exch_mgmt_t mgmt, services_t service,
    sysarg_t arg2, sysarg_t arg3)
{
	async_exch_t *exch = async_exchange_begin(session_ns);
	if (!exch)
		return NULL;
	async_sess_t *sess =
	    async_connect_me_to_blocking(mgmt, exch, service, arg2, arg3);
	async_exchange_end(exch);
	
	if (!sess)
		return NULL;
	
	/*
	 * FIXME Ugly hack to work around limitation of implementing
	 * parallel exchanges using multiple connections. Shift out
	 * first argument for non-initial connections.
	 */
	async_sess_args_set(sess, arg2, arg3, 0);
	
	return sess;
}

/** Create bidirectional connection with a service
 *
 * @param[in] service         Service.
 * @param[in] arg1            First parameter.
 * @param[in] arg2            Second parameter.
 * @param[in] arg3            Third parameter.
 * @param[in] client_receiver Message receiver.
 *
 * @return Session to the service.
 * @return Other error codes as defined by async_connect_to_me().
 *
 */
async_sess_t *service_bind(services_t service, sysarg_t arg1, sysarg_t arg2,
    sysarg_t arg3, async_client_conn_t client_receiver)
{
	/* Connect to the needed service */
	async_sess_t *sess =
	    service_connect_blocking(EXCHANGE_SERIALIZE, service, 0, 0);
	if (sess != NULL) {
		/* Request callback connection */
		async_exch_t *exch = async_exchange_begin(sess);
		int rc = async_connect_to_me(exch, arg1, arg2, arg3,
		    client_receiver, NULL);
		async_exchange_end(exch);
		
		if (rc != EOK) {
			async_hangup(sess);
			errno = rc;
			return NULL;
		}
	}
	
	return sess;
}

int ns_ping(void)
{
	async_exch_t *exch = async_exchange_begin(session_ns);
	int rc = async_req_0_0(exch, NS_PING);
	async_exchange_end(exch);
	
	return rc;
}

int ns_intro(task_id_t id)
{
	async_exch_t *exch = async_exchange_begin(session_ns);
	int rc = async_req_2_0(exch, NS_ID_INTRO, LOWER32(id), UPPER32(id));
	async_exchange_end(exch);
	
	return rc;
}

/** @}
 */
