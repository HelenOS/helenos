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

/** @addtogroup libinet
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

static void udp_cb_conn(ipc_call_t *, void *);

/** Create callback connection from UDP service.
 *
 * @param udp UDP service
 * @return EOK on success or an error code
 */
static errno_t udp_callback_create(udp_t *udp)
{
	async_exch_t *exch = async_exchange_begin(udp->sess);

	aid_t req = async_send_0(exch, UDP_CALLBACK_CREATE, NULL);

	port_id_t port;
	errno_t rc = async_create_callback_port(exch, INTERFACE_UDP_CB, 0, 0,
	    udp_cb_conn, udp, &port);

	async_exchange_end(exch);

	if (rc != EOK)
		return rc;

	errno_t retval;
	async_wait_for(req, &retval);

	return retval;
}

/** Create UDP client instance.
 *
 * @param  rudp Place to store pointer to new UDP client
 * @return EOK on success, ENOMEM if out of memory, EIO if service
 *         cannot be contacted
 */
errno_t udp_create(udp_t **rudp)
{
	udp_t *udp;
	service_id_t udp_svcid;
	errno_t rc;

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

	udp->sess = loc_service_connect(udp_svcid, INTERFACE_UDP,
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

/** Destroy UDP client instance.
 *
 * @param udp UDP client
 */
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

/** Create new UDP association.
 *
 * Create a UDP association that allows sending and receiving messages.
 *
 * @a epp may specify remote address and port, in which case only messages
 * from that remote endpoint will be received. Also, that remote endpoint
 * is used as default when @c NULL is passed as destination to
 * udp_assoc_send_msg.
 *
 * @a epp may specify a local link or address. If it does not, the association
 * will listen on all local links/addresses. If @a epp does not specify
 * a local port number, a free dynamic port number will be allocated.
 *
 * The caller is informed about incoming data by invoking @a cb->recv_msg
 *
 * @param udp    UDP client
 * @param epp    Internet endpoint pair
 * @param cb     Callbacks
 * @param arg    Argument to callbacks
 * @param rassoc Place to store pointer to new association
 *
 * @return EOK on success or an error code.
 */
errno_t udp_assoc_create(udp_t *udp, inet_ep2_t *epp, udp_cb_t *cb, void *arg,
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
	errno_t rc = async_data_write_start(exch, (void *)epp,
	    sizeof(inet_ep2_t));
	async_exchange_end(exch);

	if (rc != EOK) {
		errno_t rc_orig;
		async_wait_for(req, &rc_orig);
		if (rc_orig != EOK)
			rc = rc_orig;
		goto error;
	}

	async_wait_for(req, &rc);
	if (rc != EOK)
		goto error;

	assoc->udp = udp;
	assoc->id = ipc_get_arg1(&answer);
	assoc->cb = cb;
	assoc->cb_arg = arg;

	list_append(&assoc->ludp, &udp->assoc);
	*rassoc = assoc;

	return EOK;
error:
	free(assoc);
	return (errno_t) rc;
}

/** Destroy UDP association.
 *
 * Destroy UDP association. The caller should destroy all associations
 * he created before destroying the UDP client and before terminating.
 *
 * @param assoc UDP association
 */
void udp_assoc_destroy(udp_assoc_t *assoc)
{
	async_exch_t *exch;

	if (assoc == NULL)
		return;

	list_remove(&assoc->ludp);

	exch = async_exchange_begin(assoc->udp->sess);
	errno_t rc = async_req_1_0(exch, UDP_ASSOC_DESTROY, assoc->id);
	async_exchange_end(exch);

	free(assoc);
	(void) rc;
}

/** Set UDP association sending messages with no local address
 *
 * @param assoc Association
 * @param flags Flags
 */
errno_t udp_assoc_set_nolocal(udp_assoc_t *assoc)
{
	async_exch_t *exch;

	exch = async_exchange_begin(assoc->udp->sess);
	errno_t rc = async_req_1_0(exch, UDP_ASSOC_SET_NOLOCAL, assoc->id);
	async_exchange_end(exch);

	return rc;
}

/** Send message via UDP association.
 *
 * @param assoc Association
 * @param dest	Destination endpoint or @c NULL to use association's remote ep.
 * @param data	Message data
 * @param bytes Message size in bytes
 *
 * @return EOK on success or an error code
 */
errno_t udp_assoc_send_msg(udp_assoc_t *assoc, inet_ep_t *dest, void *data,
    size_t bytes)
{
	async_exch_t *exch;
	inet_ep_t ddest;

	exch = async_exchange_begin(assoc->udp->sess);
	aid_t req = async_send_1(exch, UDP_ASSOC_SEND_MSG, assoc->id, NULL);

	/* If dest is null, use default destination */
	if (dest == NULL) {
		inet_ep_init(&ddest);
		dest = &ddest;
	}

	errno_t rc = async_data_write_start(exch, (void *)dest,
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

/** Get the user/callback argument for an association.
 *
 * @param assoc UDP association
 * @return User argument associated with association
 */
void *udp_assoc_userptr(udp_assoc_t *assoc)
{
	return assoc->cb_arg;
}

/** Get size of received message in bytes.
 *
 * Assuming jumbo messages can be received, the caller first needs to determine
 * the size of the received message by calling this function, then they can
 * read the message piece-wise using udp_rmsg_read().
 *
 * @param rmsg Received message
 * @return Size of received message in bytes
 */
size_t udp_rmsg_size(udp_rmsg_t *rmsg)
{
	return rmsg->size;
}

/** Read part of received message.
 *
 * @param rmsg  Received message
 * @param off   Start offset
 * @param buf   Buffer for storing data
 * @param bsize Buffer size
 *
 * @return EOK on success or an error code.
 */
errno_t udp_rmsg_read(udp_rmsg_t *rmsg, size_t off, void *buf, size_t bsize)
{
	async_exch_t *exch;
	ipc_call_t answer;

	exch = async_exchange_begin(rmsg->udp->sess);
	aid_t req = async_send_1(exch, UDP_RMSG_READ, off, &answer);
	errno_t rc = async_data_read_start(exch, buf, bsize);
	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);
	if (retval != EOK) {
		return retval;
	}

	return EOK;
}

/** Get remote endpoint of received message.
 *
 * Place the remote endpoint (the one from which the message was supposedly
 * sent) to @a ep.
 *
 * @param rmsg Received message
 * @param ep   Place to store remote endpoint
 */
void udp_rmsg_remote_ep(udp_rmsg_t *rmsg, inet_ep_t *ep)
{
	*ep = rmsg->remote_ep;
}

/** Get type of received ICMP error message.
 *
 * @param rerr Received error message
 * @return Error message type
 */
uint8_t udp_rerr_type(udp_rerr_t *rerr)
{
	return 0;
}

/** Get code of received ICMP error message.
 *
 * @param rerr Received error message
 * @return Error message code
 */
uint8_t udp_rerr_code(udp_rerr_t *rerr)
{
	return 0;
}

/** Get information about the next received message from UDP service.
 *
 * @param udp  UDP client
 * @param rmsg Place to store message information
 *
 * @return EOK on success or an error code
 */
static errno_t udp_rmsg_info(udp_t *udp, udp_rmsg_t *rmsg)
{
	async_exch_t *exch;
	inet_ep_t ep;
	ipc_call_t answer;

	exch = async_exchange_begin(udp->sess);
	aid_t req = async_send_0(exch, UDP_RMSG_INFO, &answer);
	errno_t rc = async_data_read_start(exch, &ep, sizeof(inet_ep_t));
	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);
	if (retval != EOK)
		return retval;

	rmsg->udp = udp;
	rmsg->assoc_id = ipc_get_arg1(&answer);
	rmsg->size = ipc_get_arg2(&answer);
	rmsg->remote_ep = ep;
	return EOK;
}

/** Discard next received message in UDP service.
 *
 * @param udp UDP client
 * @return EOK on success or an error code
 */
static errno_t udp_rmsg_discard(udp_t *udp)
{
	async_exch_t *exch;

	exch = async_exchange_begin(udp->sess);
	errno_t rc = async_req_0_0(exch, UDP_RMSG_DISCARD);
	async_exchange_end(exch);

	return rc;
}

/** Get association based on its ID.
 *
 * @param udp    UDP client
 * @param id     Association ID
 * @param rassoc Place to store pointer to association
 *
 * @return EOK on success, EINVAL if no association with the given ID exists
 */
static errno_t udp_assoc_get(udp_t *udp, sysarg_t id, udp_assoc_t **rassoc)
{
	list_foreach(udp->assoc, ludp, udp_assoc_t, assoc) {
		if (assoc->id == id) {
			*rassoc = assoc;
			return EOK;
		}
	}

	return EINVAL;
}

/** Handle 'data' event, i.e. some message(s) arrived.
 *
 * For each received message, get information about it, call @c recv_msg
 * callback and discard it.
 *
 * @param udp   UDP client
 * @param icall IPC message
 *
 */
static void udp_ev_data(udp_t *udp, ipc_call_t *icall)
{
	udp_rmsg_t rmsg;
	udp_assoc_t *assoc;
	errno_t rc;

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

	async_answer_0(icall, EOK);
}

/** UDP service callback connection.
 *
 * @param icall Connect message
 * @param arg   Argument, UDP client
 *
 */
static void udp_cb_conn(ipc_call_t *icall, void *arg)
{
	udp_t *udp = (udp_t *)arg;

	while (true) {
		ipc_call_t call;
		async_get_call(&call);

		if (!ipc_get_imethod(&call)) {
			/* Hangup */
			async_answer_0(&call, EOK);
			goto out;
		}

		switch (ipc_get_imethod(&call)) {
		case UDP_EV_DATA:
			udp_ev_data(udp, &call);
			break;
		default:
			async_answer_0(&call, ENOTSUP);
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
