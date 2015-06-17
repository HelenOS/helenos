/*
 * Copyright (c) 2015 Jiri Svoboda
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
/** @file UDP API
 */

#include <errno.h>
#include <inet/endpoint.h>
#include <inet/udp.h>
#include <ipc/services.h>
#include <ipc/udp.h>
#include <loc.h>
#include <stdlib.h>

static void udp_cb_conn(ipc_callid_t, ipc_call_t *, void *);

static int udp_callback_create(udp_t *udp)
{
	async_exch_t *exch = async_exchange_begin(udp->sess);

	aid_t req = async_send_0(exch, UDP_CALLBACK_CREATE, NULL);
	int rc = async_connect_to_me(exch, 0, 0, 0, udp_cb_conn, udp);
	async_exchange_end(exch);

	if (rc != EOK)
		return rc;

	sysarg_t retval;
	async_wait_for(req, &retval);

	return retval;
}

int udp_create(udp_t **rudp)
{
	udp_t *udp;
	service_id_t udp_svcid;
	int rc;

	udp = calloc(1, sizeof(udp_t));
	if (udp == NULL) {
		rc = ENOMEM;
		goto error;
	}

	list_initialize(&udp->assoc);
	fibril_mutex_initialize(&udp->lock);
	fibril_condvar_initialize(&udp->cv);

	rc = loc_service_get_id(SERVICE_NAME_UDP, &udp_svcid,
	    IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	udp->sess = loc_service_connect(EXCHANGE_SERIALIZE, udp_svcid,
	    IPC_FLAG_BLOCKING);
	if (udp->sess == NULL) {
		rc = EIO;
		goto error;
	}

	rc = udp_callback_create(udp);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	*rudp = udp;
	return EOK;
error:
	free(udp);
	return rc;
}

void udp_destroy(udp_t *udp)
{
	if (udp == NULL)
		return;

	async_hangup(udp->sess);

	fibril_mutex_lock(&udp->lock);
	while (!udp->cb_done)
		fibril_condvar_wait(&udp->cv, &udp->lock);
	fibril_mutex_unlock(&udp->lock);

	free(udp);
}

int udp_assoc_create(udp_t *udp, inet_ep2_t *ep2, udp_cb_t *cb, void *arg,
    udp_assoc_t **rassoc)
{
	async_exch_t *exch;
	udp_assoc_t *assoc;
	ipc_call_t answer;

	assoc = calloc(1, sizeof(udp_assoc_t));
	if (assoc == NULL)
		return ENOMEM;

	exch = async_exchange_begin(udp->sess);
	aid_t req = async_send_0(exch, UDP_ASSOC_CREATE, &answer);
	sysarg_t rc = async_data_write_start(exch, (void *)ep2,
	    sizeof(inet_ep2_t));
	async_exchange_end(exch);

	if (rc != EOK) {
		sysarg_t rc_orig;
		async_wait_for(req, &rc_orig);
		if (rc_orig != EOK)
			rc = rc_orig;
		goto error;
	}

	async_wait_for(req, &rc);
	if (rc != EOK)
		goto error;

	assoc->udp = udp;
	assoc->id = IPC_GET_ARG1(answer);
	assoc->cb = cb;
	assoc->cb_arg = arg;

	list_append(&assoc->ludp, &udp->assoc);
	*rassoc = assoc;

	return EOK;
error:
	free(assoc);
	return (int) rc;
}

void udp_assoc_destroy(udp_assoc_t *assoc)
{
	async_exch_t *exch;

	if (assoc == NULL)
		return;

	list_remove(&assoc->ludp);

	exch = async_exchange_begin(assoc->udp->sess);
	sysarg_t rc = async_req_1_0(exch, UDP_ASSOC_DESTROY, assoc->id);
	async_exchange_end(exch);

	free(assoc);
	(void) rc;
}

int udp_assoc_send_msg(udp_assoc_t *assoc, inet_ep_t *dest, void *data,
    size_t bytes)
{
	async_exch_t *exch;

	exch = async_exchange_begin(assoc->udp->sess);
	aid_t req = async_send_1(exch, UDP_ASSOC_SEND_MSG, assoc->id, NULL);

	sysarg_t rc = async_data_write_start(exch, (void *)dest,
	    sizeof(inet_ep_t));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}

	rc = async_data_write_start(exch, data, bytes);
	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	async_wait_for(req, &rc);
	return rc;
}

void *udp_assoc_userptr(udp_assoc_t *assoc)
{
	return assoc->cb_arg;
}

size_t udp_rmsg_size(udp_rmsg_t *rmsg)
{
	return rmsg->size;
}

int udp_rmsg_read(udp_rmsg_t *rmsg, size_t off, void *buf, size_t bsize)
{
	async_exch_t *exch;
	ipc_call_t answer;

	exch = async_exchange_begin(rmsg->udp->sess);
	aid_t req = async_send_1(exch, UDP_RMSG_READ, off, &answer);
	int rc = async_data_read_start(exch, buf, bsize);
	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	sysarg_t retval;
	async_wait_for(req, &retval);
	if (retval != EOK) {
		return retval;
	}

	return EOK;
}

void udp_rmsg_remote_ep(udp_rmsg_t *rmsg, inet_ep_t *ep)
{
	*ep = rmsg->remote_ep;
}

uint8_t udp_rerr_type(udp_rerr_t *rerr)
{
	return 0;
}

uint8_t udp_rerr_code(udp_rerr_t *rerr)
{
	return 0;
}

static int udp_rmsg_info(udp_t *udp, udp_rmsg_t *rmsg)
{
	async_exch_t *exch;
	inet_ep_t ep;
	ipc_call_t answer;

	exch = async_exchange_begin(udp->sess);
	aid_t req = async_send_0(exch, UDP_RMSG_INFO, &answer);
	int rc = async_data_read_start(exch, &ep, sizeof(inet_ep_t));
	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	sysarg_t retval;
	async_wait_for(req, &retval);
	if (retval != EOK)
		return retval;

	rmsg->udp = udp;
	rmsg->assoc_id = IPC_GET_ARG1(answer);
	rmsg->size = IPC_GET_ARG2(answer);
	rmsg->remote_ep = ep;
	return EOK;
}

static int udp_rmsg_discard(udp_t *udp)
{
	async_exch_t *exch;

	exch = async_exchange_begin(udp->sess);
	sysarg_t rc = async_req_0_0(exch, UDP_RMSG_DISCARD);
	async_exchange_end(exch);

	return rc;
}

static int udp_assoc_get(udp_t *udp, sysarg_t id, udp_assoc_t **rassoc)
{
	list_foreach(udp->assoc, ludp, udp_assoc_t, assoc) {
		if (assoc->id == id) {
			*rassoc = assoc;
			return EOK;
		}
	}

	return EINVAL;
}

static void udp_ev_data(udp_t *udp, ipc_callid_t iid, ipc_call_t *icall)
{
	udp_rmsg_t rmsg;
	udp_assoc_t *assoc;
	int rc;

	while (true) {
		rc = udp_rmsg_info(udp, &rmsg);
		if (rc != EOK) {
			break;
		}

		rc = udp_assoc_get(udp, rmsg.assoc_id, &assoc);
		if (rc != EOK) {
			continue;
		}

		if (assoc->cb != NULL && assoc->cb->recv_msg != NULL)
			assoc->cb->recv_msg(assoc, &rmsg);

		rc = udp_rmsg_discard(udp);
		if (rc != EOK) {
			break;
		}
	}

	async_answer_0(iid, EOK);
}

static void udp_cb_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	udp_t *udp = (udp_t *)arg;

	async_answer_0(iid, EOK);

	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);

		if (!IPC_GET_IMETHOD(call)) {
			/* Hangup */
			goto out;
		}

		switch (IPC_GET_IMETHOD(call)) {
		case UDP_EV_DATA:
			udp_ev_data(udp, callid, &call);
			break;
		default:
			async_answer_0(callid, ENOTSUP);
			break;
		}
	}
out:
	fibril_mutex_lock(&udp->lock);
	udp->cb_done = true;
	fibril_mutex_unlock(&udp->lock);
	fibril_condvar_broadcast(&udp->cv);
}

/** @}
 */
