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

#include "private/taskman.h"

static async_sess_t *session_ns = NULL;

static async_exch_t *ns_exchange_begin(void)
{
	/* Lazily connect to our NS */
	if (session_ns == NULL) {
		session_ns = taskman_session_ns();
	}

	async_exch_t *exch = async_exchange_begin(session_ns);
	return exch;
}

static void ns_exchange_end(async_exch_t *exch)
{
	async_exchange_end(exch);
}

/*
 * XXX ns does not know about session_primary, so we create an extra session for
 * actual communicaton
 */
static async_sess_t *sess_primary = NULL;

errno_t service_register(service_t service, iface_t iface,
    async_port_handler_t handler, void *data)
{
	port_id_t port;
	errno_t rc = async_create_port(iface, handler, data, &port);
	if (rc != EOK)
		return rc;

	async_exch_t *exch = ns_exchange_begin();

	ipc_call_t answer;
	aid_t req = async_send_2(exch, NS_REGISTER, service, iface, &answer);
	rc = async_connect_to_me(exch, iface, service, 0);

	ns_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);
	return rc;
}

errno_t service_register_broker(service_t service, async_port_handler_t handler,
    void *data)
{
	async_set_fallback_port_handler(handler, data);

	async_sess_t *sess = get_session_primary();
	if (sess == NULL)
		return EIO;

	async_exch_t *exch = async_exchange_begin(sess);

	ipc_call_t answer;
	aid_t req = async_send_1(exch, NS_REGISTER_BROKER, service, &answer);
	errno_t rc = async_connect_to_me(exch, INTERFACE_ANY, service, 0);

	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);
	return rc;
}

async_sess_t *service_connect(service_t service, iface_t iface, sysarg_t arg3)
{
	async_exch_t *exch = ns_exchange_begin();
	if (exch == NULL)
		return NULL;

	async_sess_t *sess =
	    async_connect_me_to(exch, iface, service, arg3);
	ns_exchange_end(exch);

	if (sess == NULL)
		return NULL;

	/*
	 * FIXME Ugly hack to work around limitation of implementing
	 * parallel exchanges using multiple connections. Shift out
	 * first argument for non-initial connections.
	 */
	async_sess_args_set(sess, iface, arg3, 0);

	return csess;
}

async_sess_t *service_connect_blocking(service_t service, iface_t iface,
    sysarg_t arg3)
{
	async_exch_t *exch = ns_exchange_begin();
	async_sess_t *sess =
	    async_connect_me_to_blocking(exch, iface, service, arg3);
	ns_exchange_end(exch);

	if (sess == NULL)
		return NULL;

	/*
	 * FIXME Ugly hack to work around limitation of implementing
	 * parallel exchanges using multiple connections. Shift out
	 * first argument for non-initial connections.
	 */
	async_sess_args_set(sess, iface, arg3, 0);

	return sess;
}

errno_t ns_ping(void)
{
	async_exch_t *exch = ns_exchange_begin(sess);
	errno_t rc = async_req_0_0(exch, NS_PING);
	ns_exchange_end(exch);

	return rc;
}

/** @}
 */
