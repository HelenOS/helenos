/*
 * Copyright (c) 2013 Jiri Svoboda
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
#include <inet/inetping.h>
#include <ipc/inet.h>
#include <ipc/services.h>
#include <loc.h>
#include <stdlib.h>
#include <str.h>

static void inetping_cb_conn(cap_call_handle_t, ipc_call_t *, void *);
static void inetping_ev_recv(cap_call_handle_t, ipc_call_t *);

static async_sess_t *inetping_sess = NULL;
static inetping_ev_ops_t *inetping_ev_ops;

errno_t inetping_init(inetping_ev_ops_t *ev_ops)
{
	service_id_t inetping_svc;
	errno_t rc;

	assert(inetping_sess == NULL);

	inetping_ev_ops = ev_ops;

	rc = loc_service_get_id(SERVICE_NAME_INET, &inetping_svc,
	    IPC_FLAG_BLOCKING);
	if (rc != EOK)
		return ENOENT;

	inetping_sess = loc_service_connect(inetping_svc, INTERFACE_INETPING,
	    IPC_FLAG_BLOCKING);
	if (inetping_sess == NULL)
		return ENOENT;

	async_exch_t *exch = async_exchange_begin(inetping_sess);

	port_id_t port;
	rc = async_create_callback_port(exch, INTERFACE_INETPING_CB, 0, 0,
	    inetping_cb_conn, NULL, &port);

	async_exchange_end(exch);

	if (rc != EOK) {
		async_hangup(inetping_sess);
		inetping_sess = NULL;
		return rc;
	}

	return EOK;
}

errno_t inetping_send(inetping_sdu_t *sdu)
{
	async_exch_t *exch = async_exchange_begin(inetping_sess);

	ipc_call_t answer;
	aid_t req = async_send_1(exch, INETPING_SEND, sdu->seq_no, &answer);

	errno_t rc = async_data_write_start(exch, &sdu->src, sizeof(sdu->src));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}

	rc = async_data_write_start(exch, &sdu->dest, sizeof(sdu->dest));
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

	errno_t retval;
	async_wait_for(req, &retval);

	return retval;
}

errno_t inetping_get_srcaddr(const inet_addr_t *remote, inet_addr_t *local)
{
	async_exch_t *exch = async_exchange_begin(inetping_sess);

	ipc_call_t answer;
	aid_t req = async_send_0(exch, INETPING_GET_SRCADDR, &answer);

	errno_t rc = async_data_write_start(exch, remote, sizeof(*remote));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}

	ipc_call_t answer_local;
	aid_t req_local = async_data_read(exch, local, sizeof(*local),
	    &answer_local);

	async_exchange_end(exch);

	errno_t retval_local;
	async_wait_for(req_local, &retval_local);

	if (retval_local != EOK) {
		async_forget(req);
		return retval_local;
	}

	errno_t retval;
	async_wait_for(req, &retval);

	return retval;
}

static void inetping_ev_recv(cap_call_handle_t icall_handle, ipc_call_t *icall)
{
	inetping_sdu_t sdu;

	sdu.seq_no = IPC_GET_ARG1(*icall);

	cap_call_handle_t chandle;
	size_t size;
	if (!async_data_write_receive(&chandle, &size)) {
		async_answer_0(chandle, EREFUSED);
		async_answer_0(icall_handle, EREFUSED);
		return;
	}

	if (size != sizeof(sdu.src)) {
		async_answer_0(chandle, EINVAL);
		async_answer_0(icall_handle, EINVAL);
		return;
	}

	errno_t rc = async_data_write_finalize(chandle, &sdu.src, size);
	if (rc != EOK) {
		async_answer_0(chandle, rc);
		async_answer_0(icall_handle, rc);
		return;
	}

	if (!async_data_write_receive(&chandle, &size)) {
		async_answer_0(chandle, EREFUSED);
		async_answer_0(icall_handle, EREFUSED);
		return;
	}

	if (size != sizeof(sdu.dest)) {
		async_answer_0(chandle, EINVAL);
		async_answer_0(icall_handle, EINVAL);
		return;
	}

	rc = async_data_write_finalize(chandle, &sdu.dest, size);
	if (rc != EOK) {
		async_answer_0(chandle, rc);
		async_answer_0(icall_handle, rc);
		return;
	}

	rc = async_data_write_accept(&sdu.data, false, 0, 0, 0, &sdu.size);
	if (rc != EOK) {
		async_answer_0(icall_handle, rc);
		return;
	}

	rc = inetping_ev_ops->recv(&sdu);
	free(sdu.data);
	async_answer_0(icall_handle, rc);
}

static void inetping_cb_conn(cap_call_handle_t icall_handle, ipc_call_t *icall, void *arg)
{
	while (true) {
		ipc_call_t call;
		cap_call_handle_t chandle = async_get_call(&call);

		if (!IPC_GET_IMETHOD(call)) {
			/* TODO: Handle hangup */
			return;
		}

		switch (IPC_GET_IMETHOD(call)) {
		case INETPING_EV_RECV:
			inetping_ev_recv(chandle, &call);
			break;
		default:
			async_answer_0(chandle, ENOTSUP);
		}
	}
}

/** @}
 */
