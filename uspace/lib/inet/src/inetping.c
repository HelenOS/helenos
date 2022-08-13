/*
 * SPDX-FileCopyrightText: 2013 Jiri Svoboda
 * SPDX-FileCopyrightText: 2013 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

static void inetping_cb_conn(ipc_call_t *, void *);
static void inetping_ev_recv(ipc_call_t *);

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

static void inetping_ev_recv(ipc_call_t *icall)
{
	inetping_sdu_t sdu;

	sdu.seq_no = ipc_get_arg1(icall);

	ipc_call_t call;
	size_t size;
	if (!async_data_write_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(sdu.src)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	errno_t rc = async_data_write_finalize(&call, &sdu.src, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	if (!async_data_write_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(sdu.dest)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_write_finalize(&call, &sdu.dest, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	rc = async_data_write_accept(&sdu.data, false, 0, 0, 0, &sdu.size);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	rc = inetping_ev_ops->recv(&sdu);
	free(sdu.data);
	async_answer_0(icall, rc);
}

static void inetping_cb_conn(ipc_call_t *icall, void *arg)
{
	while (true) {
		ipc_call_t call;
		async_get_call(&call);

		if (!ipc_get_imethod(&call)) {
			async_answer_0(&call, EOK);
			return;
		}

		switch (ipc_get_imethod(&call)) {
		case INETPING_EV_RECV:
			inetping_ev_recv(&call);
			break;
		default:
			async_answer_0(&call, ENOTSUP);
		}
	}
}

/** @}
 */
