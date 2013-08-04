/*
 * Copyright (c) 2013 Martin Decky
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

#include <async.h>
#include <assert.h>
#include <errno.h>
#include <inet/inetping6.h>
#include <ipc/inet.h>
#include <ipc/services.h>
#include <loc.h>
#include <stdlib.h>
#include <str.h>

static void inetping6_cb_conn(ipc_callid_t, ipc_call_t *, void *);
static void inetping6_ev_recv(ipc_callid_t, ipc_call_t *);

static async_sess_t *inetping6_sess = NULL;
static inetping6_ev_ops_t *inetping6_ev_ops;

int inetping6_init(inetping6_ev_ops_t *ev_ops)
{
	assert(inetping6_sess == NULL);
	
	inetping6_ev_ops = ev_ops;
	
	service_id_t inetping6_svc;
	int rc = loc_service_get_id(SERVICE_NAME_INETPING6, &inetping6_svc,
	    IPC_FLAG_BLOCKING);
	if (rc != EOK)
		return ENOENT;
	
	inetping6_sess = loc_service_connect(EXCHANGE_SERIALIZE, inetping6_svc,
	    IPC_FLAG_BLOCKING);
	if (inetping6_sess == NULL)
		return ENOENT;
	
	async_exch_t *exch = async_exchange_begin(inetping6_sess);
	
	rc = async_connect_to_me(exch, 0, 0, 0, inetping6_cb_conn, NULL);
	async_exchange_end(exch);
	
	if (rc != EOK) {
		async_hangup(inetping6_sess);
		inetping6_sess = NULL;
		return rc;
	}
	
	return EOK;
}

int inetping6_send(inetping6_sdu_t *sdu)
{
	async_exch_t *exch = async_exchange_begin(inetping6_sess);
	
	ipc_call_t answer;
	aid_t req = async_send_1(exch, INETPING6_SEND, sdu->seq_no, &answer);
	
	int rc = async_data_write_start(exch, &sdu->src, sizeof(addr128_t));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}
	
	rc = async_data_write_start(exch, &sdu->dest, sizeof(addr128_t));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}
	
	rc = async_data_write_start(exch, sdu->data, sdu->size);
	
	async_exchange_end(exch);
	
	if (rc != EOK) {
		async_forget(req);
		return rc;
	}
	
	sysarg_t retval;
	async_wait_for(req, &retval);
	
	return (int) retval;
}

int inetping6_get_srcaddr(addr128_t remote, addr128_t local)
{
	async_exch_t *exch = async_exchange_begin(inetping6_sess);
	
	ipc_call_t answer;
	aid_t req = async_send_0(exch, INETPING6_GET_SRCADDR, &answer);
	
	int rc = async_data_write_start(exch, remote, sizeof(addr128_t));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}
	
	ipc_call_t answer_local;
	aid_t req_local = async_data_read(exch, local, sizeof(addr128_t),
	    &answer_local);
	
	async_exchange_end(exch);
	
	sysarg_t retval_local;
	async_wait_for(req_local, &retval_local);
	
	if (retval_local != EOK) {
		async_forget(req);
		return (int) retval_local;
	}
	
	sysarg_t retval;
	async_wait_for(req, &retval);
	
	return (int) retval;
}

static void inetping6_ev_recv(ipc_callid_t iid, ipc_call_t *icall)
{
	inetping6_sdu_t sdu;
	
	sdu.seq_no = IPC_GET_ARG1(*icall);
	
	ipc_callid_t callid;
	size_t size;
	if (!async_data_write_receive(&callid, &size)) {
		async_answer_0(callid, EREFUSED);
		async_answer_0(iid, EREFUSED);
		return;
	}
	
	if (size != sizeof(addr128_t)) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}
	
	int rc = async_data_write_finalize(callid, &sdu.src, size);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
		return;
	}
	
	if (!async_data_write_receive(&callid, &size)) {
		async_answer_0(callid, EREFUSED);
		async_answer_0(iid, EREFUSED);
		return;
	}
	
	if (size != sizeof(addr128_t)) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}
	
	rc = async_data_write_finalize(callid, &sdu.dest, size);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
		return;
	}
	
	rc = async_data_write_accept(&sdu.data, false, 0, 0, 0, &sdu.size);
	if (rc != EOK) {
		async_answer_0(iid, rc);
		return;
	}
	
	rc = inetping6_ev_ops->recv(&sdu);
	free(sdu.data);
	async_answer_0(iid, rc);
}

static void inetping6_cb_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		if (!IPC_GET_IMETHOD(call)) {
			/* TODO: Handle hangup */
			return;
		}
		
		switch (IPC_GET_IMETHOD(call)) {
		case INETPING6_EV_RECV:
			inetping6_ev_recv(callid, &call);
			break;
		default:
			async_answer_0(callid, ENOTSUP);
		}
	}
}

/** @}
 */
