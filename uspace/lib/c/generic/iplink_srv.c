/*
 * Copyright (c) 2012 Jiri Svoboda
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
/**
 * @file
 * @brief IP link server stub
 */
#include <errno.h>
#include <ipc/iplink.h>
#include <stdlib.h>
#include <sys/types.h>

#include <inet/iplink_srv.h>

static void iplink_get_mtu_srv(iplink_srv_t *srv, ipc_callid_t callid,
    ipc_call_t *call)
{
	int rc;
	size_t mtu;

	rc = srv->ops->get_mtu(srv, &mtu);
	async_answer_1(callid, rc, mtu);
}

static void iplink_addr_add_srv(iplink_srv_t *srv, ipc_callid_t callid,
    ipc_call_t *call)
{
	int rc;
	iplink_srv_addr_t addr;

	addr.ipv4 = IPC_GET_ARG1(*call);

	rc = srv->ops->addr_add(srv, &addr);
	async_answer_0(callid, rc);
}

static void iplink_addr_remove_srv(iplink_srv_t *srv, ipc_callid_t callid,
    ipc_call_t *call)
{
	int rc;
	iplink_srv_addr_t addr;

	addr.ipv4 = IPC_GET_ARG1(*call);

	rc = srv->ops->addr_remove(srv, &addr);
	async_answer_0(callid, rc);
}

static void iplink_send_srv(iplink_srv_t *srv, ipc_callid_t callid,
    ipc_call_t *call)
{
	iplink_srv_sdu_t sdu;
	int rc;

	sdu.lsrc.ipv4 = IPC_GET_ARG1(*call);
	sdu.ldest.ipv4 = IPC_GET_ARG2(*call);

	rc = async_data_write_accept(&sdu.data, false, 0, 0, 0, &sdu.size);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return;
	}

	rc = srv->ops->send(srv, &sdu);
	free(sdu.data);
	async_answer_0(callid, rc);
}

void iplink_srv_init(iplink_srv_t *srv)
{
	fibril_mutex_initialize(&srv->lock);
	srv->connected = false;
	srv->ops = NULL;
	srv->arg = NULL;
	srv->client_sess = NULL;
}

int iplink_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	iplink_srv_t *srv = (iplink_srv_t *)arg;
	int rc;

	fibril_mutex_lock(&srv->lock);
	if (srv->connected) {
		fibril_mutex_unlock(&srv->lock);
		async_answer_0(iid, EBUSY);
		return EBUSY;
	}

	srv->connected = true;
	fibril_mutex_unlock(&srv->lock);

	/* Accept the connection */
	async_answer_0(iid, EOK);

	async_sess_t *sess = async_callback_receive(EXCHANGE_SERIALIZE);
	if (sess == NULL)
		return ENOMEM;

	srv->client_sess = sess;

	rc = srv->ops->open(srv);
	if (rc != EOK)
		return rc;

	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		sysarg_t method = IPC_GET_IMETHOD(call);

		if (!method) {
			/* The other side has hung up */
			async_answer_0(callid, EOK);
			break;
		}

		switch (method) {
		case IPLINK_GET_MTU:
			iplink_get_mtu_srv(srv, callid, &call);
			break;
		case IPLINK_SEND:
			iplink_send_srv(srv, callid, &call);
			break;
		case IPLINK_ADDR_ADD:
			iplink_addr_add_srv(srv, callid, &call);
			break;
		case IPLINK_ADDR_REMOVE:
			iplink_addr_remove_srv(srv, callid, &call);
			break;
		default:
			async_answer_0(callid, EINVAL);
		}
	}

	return srv->ops->close(srv);
}

int iplink_ev_recv(iplink_srv_t *srv, iplink_srv_sdu_t *sdu)
{
	if (srv->client_sess == NULL)
		return EIO;

	async_exch_t *exch = async_exchange_begin(srv->client_sess);

	ipc_call_t answer;
	aid_t req = async_send_2(exch, IPLINK_EV_RECV, sdu->lsrc.ipv4,
	    sdu->ldest.ipv4, &answer);
	int rc = async_data_write_start(exch, sdu->data, sdu->size);
	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	sysarg_t retval;
	async_wait_for(req, &retval);
	if (retval != EOK)
		return retval;

	return EOK;
}

/** @}
 */
