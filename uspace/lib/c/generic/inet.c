/*
 * Copyright (c) 2013 Jiri Svoboda
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
#include <inet/inet.h>
#include <ipc/inet.h>
#include <ipc/services.h>
#include <loc.h>
#include <stdlib.h>

static void inet_cb_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg);

static async_sess_t *inet_sess = NULL;
static inet_ev_ops_t *inet_ev_ops = NULL;
static uint8_t inet_protocol = 0;

static errno_t inet_callback_create(void)
{
	async_exch_t *exch = async_exchange_begin(inet_sess);
	
	ipc_call_t answer;
	aid_t req = async_send_0(exch, INET_CALLBACK_CREATE, &answer);
	
	port_id_t port;
	errno_t rc = async_create_callback_port(exch, INTERFACE_INET_CB, 0, 0,
	    inet_cb_conn, NULL, &port);
	
	async_exchange_end(exch);
	
	if (rc != EOK)
		return rc;
	
	errno_t retval;
	async_wait_for(req, &retval);
	
	return retval;
}

static errno_t inet_set_proto(uint8_t protocol)
{
	async_exch_t *exch = async_exchange_begin(inet_sess);
	errno_t rc = async_req_1_0(exch, INET_SET_PROTO, protocol);
	async_exchange_end(exch);
	
	return rc;
}

errno_t inet_init(uint8_t protocol, inet_ev_ops_t *ev_ops)
{
	service_id_t inet_svc;
	errno_t rc;

	assert(inet_sess == NULL);
	assert(inet_ev_ops == NULL);
	assert(inet_protocol == 0);
	
	rc = loc_service_get_id(SERVICE_NAME_INET, &inet_svc,
	    IPC_FLAG_BLOCKING);
	if (rc != EOK)
		return ENOENT;
	
	inet_sess = loc_service_connect(inet_svc, INTERFACE_INET,
	    IPC_FLAG_BLOCKING);
	if (inet_sess == NULL)
		return ENOENT;
	
	if (inet_set_proto(protocol) != EOK) {
		async_hangup(inet_sess);
		inet_sess = NULL;
		return EIO;
	}
	
	if (inet_callback_create() != EOK) {
		async_hangup(inet_sess);
		inet_sess = NULL;
		return EIO;
	}
	
	inet_protocol = protocol;
	inet_ev_ops = ev_ops;

	return EOK;
}

errno_t inet_send(inet_dgram_t *dgram, uint8_t ttl, inet_df_t df)
{
	async_exch_t *exch = async_exchange_begin(inet_sess);
	
	ipc_call_t answer;
	aid_t req = async_send_4(exch, INET_SEND, dgram->iplink, dgram->tos,
	    ttl, df, &answer);
	
	errno_t rc = async_data_write_start(exch, &dgram->src, sizeof(inet_addr_t));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}
	
	rc = async_data_write_start(exch, &dgram->dest, sizeof(inet_addr_t));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}
	
	rc = async_data_write_start(exch, dgram->data, dgram->size);
	
	async_exchange_end(exch);
	
	if (rc != EOK) {
		async_forget(req);
		return rc;
	}
	
	errno_t retval;
	async_wait_for(req, &retval);
	
	return retval;
}

errno_t inet_get_srcaddr(inet_addr_t *remote, uint8_t tos, inet_addr_t *local)
{
	async_exch_t *exch = async_exchange_begin(inet_sess);
	
	ipc_call_t answer;
	aid_t req = async_send_1(exch, INET_GET_SRCADDR, tos, &answer);
	
	errno_t rc = async_data_write_start(exch, remote, sizeof(inet_addr_t));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}
	
	rc = async_data_read_start(exch, local, sizeof(inet_addr_t));
	
	async_exchange_end(exch);
	
	if (rc != EOK) {
		async_forget(req);
		return rc;
	}
	
	errno_t retval;
	async_wait_for(req, &retval);
	
	return retval;
}

static void inet_ev_recv(ipc_callid_t iid, ipc_call_t *icall)
{
	inet_dgram_t dgram;
	
	dgram.tos = IPC_GET_ARG1(*icall);
	dgram.iplink = IPC_GET_ARG2(*icall);
	
	ipc_callid_t callid;
	size_t size;
	if (!async_data_write_receive(&callid, &size)) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}
	
	if (size != sizeof(inet_addr_t)) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}
	
	errno_t rc = async_data_write_finalize(callid, &dgram.src, size);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
		return;
	}
	
	if (!async_data_write_receive(&callid, &size)) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}
	
	if (size != sizeof(inet_addr_t)) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}
	
	rc = async_data_write_finalize(callid, &dgram.dest, size);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
		return;
	}
	
	rc = async_data_write_accept(&dgram.data, false, 0, 0, 0, &dgram.size);
	if (rc != EOK) {
		async_answer_0(iid, rc);
		return;
	}
	
	rc = inet_ev_ops->recv(&dgram);
	free(dgram.data);
	async_answer_0(iid, rc);
}

static void inet_cb_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		if (!IPC_GET_IMETHOD(call)) {
			/* TODO: Handle hangup */
			return;
		}
		
		switch (IPC_GET_IMETHOD(call)) {
		case INET_EV_RECV:
			inet_ev_recv(callid, &call);
			break;
		default:
			async_answer_0(callid, ENOTSUP);
		}
	}
}

/** @}
 */
