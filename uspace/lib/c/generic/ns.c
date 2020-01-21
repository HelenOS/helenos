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

/*
 * XXX ns does not know about session_ns, so we create an extra session for
 * actual communicaton
 */
static async_sess_t *sess_ns = NULL;

errno_t service_register(service_t service, iface_t iface,
    async_port_handler_t handler, void *data)
{
	errno_t rc;
	async_sess_t *sess = ns_session_get(&rc);
	if (sess == NULL)
		return rc;

	port_id_t port;
	rc = async_create_port(iface, handler, data, &port);
	if (rc != EOK)
		return rc;

	async_exch_t *exch = async_exchange_begin(sess);

	ipc_call_t answer;
	aid_t req = async_send_2(exch, NS_REGISTER, service, iface, &answer);
	rc = async_connect_to_me(exch, iface, service, 0);

	async_exchange_end(exch);

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

	errno_t rc;
	async_sess_t *sess = ns_session_get(&rc);
	if (sess == NULL)
		return rc;

	async_exch_t *exch = async_exchange_begin(sess);

	ipc_call_t answer;
	aid_t req = async_send_1(exch, NS_REGISTER_BROKER, service, &answer);
	rc = async_connect_to_me(exch, INTERFACE_ANY, service, 0);

	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);
	return rc;
}

/** Connect to a singleton service.
 *
 * @param service Singleton service ID.
 * @param iface   Interface to connect to.
 * @param arg3    Custom connection argument.
 * @param rc      Placeholder for return code. Unused if NULL.
 *
 * @return New session on success or NULL on error.
 *
 */
async_sess_t *service_connect(service_t service, iface_t iface, sysarg_t arg3,
    errno_t *rc)
{
	async_sess_t *sess = ns_session_get(rc);
	if (sess == NULL)
		return NULL;

	async_exch_t *exch = async_exchange_begin(sess);
	if (exch == NULL)
		return NULL;

	async_sess_t *csess =
	    async_connect_me_to(exch, iface, service, arg3, rc);
	async_exchange_end(exch);

	if (csess == NULL)
		return NULL;

	/*
	 * FIXME Ugly hack to work around limitation of implementing
	 * parallel exchanges using multiple connections. Shift out
	 * first argument for non-initial connections.
	 */
	async_sess_args_set(csess, iface, arg3, 0);

	return csess;
}

/** Wait and connect to a singleton service.
 *
 * @param service Singleton service ID.
 * @param iface   Interface to connect to.
 * @param arg3    Custom connection argument.
 * @param rc      Placeholder for return code. Unused if NULL.
 *
 * @return New session on success or NULL on error.
 *
 */
async_sess_t *service_connect_blocking(service_t service, iface_t iface,
    sysarg_t arg3, errno_t *rc)
{
	async_sess_t *sess = ns_session_get(rc);
	if (sess == NULL)
		return NULL;

	async_exch_t *exch = async_exchange_begin(sess);
	async_sess_t *csess =
	    async_connect_me_to_blocking(exch, iface, service, arg3, rc);
	async_exchange_end(exch);

	if (csess == NULL)
		return NULL;

	/*
	 * FIXME Ugly hack to work around limitation of implementing
	 * parallel exchanges using multiple connections. Shift out
	 * first argument for non-initial connections.
	 */
	async_sess_args_set(csess, iface, arg3, 0);

	return csess;
}

errno_t ns_ping(void)
{
	errno_t rc;
	async_sess_t *sess = ns_session_get(&rc);
	if (sess == NULL)
		return rc;

	async_exch_t *exch = async_exchange_begin(sess);
	rc = async_req_0_0(exch, NS_PING);
	async_exchange_end(exch);

	return rc;
}

errno_t ns_intro(task_id_t id)
{
	errno_t rc;
	async_sess_t *sess = ns_session_get(&rc);
	if (sess == NULL)
		return EIO;

	async_exch_t *exch = async_exchange_begin(sess);
	rc = async_req_2_0(exch, NS_ID_INTRO, LOWER32(id), UPPER32(id));
	async_exchange_end(exch);

	return rc;
}

async_sess_t *ns_session_get(errno_t *rc)
{
	async_exch_t *exch;

	if (sess_ns == NULL) {
		exch = async_exchange_begin(&session_ns);
		sess_ns = async_connect_me_to(exch, 0, 0, 0, rc);
		async_exchange_end(exch);
		if (sess_ns == NULL)
			return NULL;
	}

	return sess_ns;
}

/** @}
 */
