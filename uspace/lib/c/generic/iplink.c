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
 * @brief IP link client stub
 */

#include <async.h>
#include <assert.h>
#include <errno.h>
#include <inet/iplink.h>
#include <ipc/iplink.h>
#include <ipc/services.h>
#include <loc.h>
#include <stdlib.h>

static void iplink_cb_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg);

int iplink_open(async_sess_t *sess, iplink_ev_ops_t *ev_ops,
    iplink_t **riplink)
{
	iplink_t *iplink = calloc(1, sizeof(iplink_t));
	if (iplink == NULL)
		return ENOMEM;
	
	iplink->sess = sess;
	iplink->ev_ops = ev_ops;
	
	async_exch_t *exch = async_exchange_begin(sess);
	
	int rc = async_connect_to_me(exch, 0, 0, 0, iplink_cb_conn, iplink);
	async_exchange_end(exch);
	
	if (rc != EOK)
		goto error;
	
	*riplink = iplink;
	return EOK;
	
error:
	if (iplink != NULL)
		free(iplink);
	
	return rc;
}

void iplink_close(iplink_t *iplink)
{
	/* XXX Synchronize with iplink_cb_conn */
	free(iplink);
}

int iplink_send(iplink_t *iplink, iplink_sdu_t *sdu)
{
	async_exch_t *exch = async_exchange_begin(iplink->sess);

	ipc_call_t answer;
	aid_t req = async_send_2(exch, IPLINK_SEND, sdu->lsrc.ipv4,
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

int iplink_get_mtu(iplink_t *iplink, size_t *rmtu)
{
	sysarg_t mtu;
	async_exch_t *exch = async_exchange_begin(iplink->sess);

	int rc = async_req_0_1(exch, IPLINK_GET_MTU, &mtu);
	async_exchange_end(exch);

	if (rc != EOK)
		return rc;

	*rmtu = mtu;
	return EOK;
}

int iplink_addr_add(iplink_t *iplink, iplink_addr_t *addr)
{
	async_exch_t *exch = async_exchange_begin(iplink->sess);

	int rc = async_req_1_0(exch, IPLINK_ADDR_ADD, (sysarg_t)addr->ipv4);
	async_exchange_end(exch);

	return rc;
}

int iplink_addr_remove(iplink_t *iplink, iplink_addr_t *addr)
{
	async_exch_t *exch = async_exchange_begin(iplink->sess);

	int rc = async_req_1_0(exch, IPLINK_ADDR_REMOVE, (sysarg_t)addr->ipv4);
	async_exchange_end(exch);

	return rc;
}

static void iplink_ev_recv(iplink_t *iplink, ipc_callid_t callid,
    ipc_call_t *call)
{
	int rc;
	iplink_sdu_t sdu;

	sdu.lsrc.ipv4 = IPC_GET_ARG1(*call);
	sdu.ldest.ipv4 = IPC_GET_ARG2(*call);

	rc = async_data_write_accept(&sdu.data, false, 0, 0, 0, &sdu.size);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return;
	}

	rc = iplink->ev_ops->recv(iplink, &sdu);
	free(sdu.data);
	async_answer_0(callid, rc);
}

static void iplink_cb_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	iplink_t *iplink = (iplink_t *)arg;

	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);

		if (!IPC_GET_IMETHOD(call)) {
			/* TODO: Handle hangup */
			return;
		}

		switch (IPC_GET_IMETHOD(call)) {
		case IPLINK_EV_RECV:
			iplink_ev_recv(iplink, callid, &call);
			break;
		default:
			async_answer_0(callid, ENOTSUP);
		}
	}
}

/** @}
 */
