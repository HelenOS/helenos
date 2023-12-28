/*
 * Copyright (c) 2023 Jiri Svoboda
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

/** @addtogroup udp
 * @{
 */

/**
 * @file HelenOS service implementation
 */

#include <async.h>
#include <errno.h>
#include <inet/endpoint.h>
#include <io/log.h>
#include <ipc/services.h>
#include <ipc/udp.h>
#include <loc.h>
#include <macros.h>
#include <stdlib.h>

#include "assoc.h"
#include "cassoc.h"
#include "msg.h"
#include "service.h"
#include "udp_type.h"

#define NAME "udp"

/** Maximum message size */
#define MAX_MSG_SIZE DATA_XFER_LIMIT

static void udp_recv_msg_cassoc(void *, inet_ep2_t *, udp_msg_t *);

/** Callbacks to tie us to association layer */
static udp_assoc_cb_t udp_cassoc_cb = {
	.recv_msg = udp_recv_msg_cassoc
};

/** Send 'data' event to client.
 *
 * @param client Client
 */
static void udp_ev_data(udp_client_t *client)
{
	async_exch_t *exch;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_ev_data()");

	exch = async_exchange_begin(client->sess);
	aid_t req = async_send_0(exch, UDP_EV_DATA, NULL);
	async_exchange_end(exch);

	async_forget(req);
}

/** Message received on client association.
 *
 * Used as udp_assoc_cb.recv_msg callback.
 *
 * @param arg Callback argument, client association
 * @param epp Endpoint pair where message was received
 * @param msg Message
 */
static void udp_recv_msg_cassoc(void *arg, inet_ep2_t *epp, udp_msg_t *msg)
{
	udp_cassoc_t *cassoc = (udp_cassoc_t *) arg;

	udp_cassoc_queue_msg(cassoc, epp, msg);
	udp_ev_data(cassoc->client);
}

/** Create association.
 *
 * Handle client request to create association (with parameters unmarshalled).
 *
 * @param client    UDP client
 * @param epp       Endpoint pair
 * @param rassoc_id Place to store ID of new association
 *
 * @return EOK on success or an error code
 */
static errno_t udp_assoc_create_impl(udp_client_t *client, inet_ep2_t *epp,
    sysarg_t *rassoc_id)
{
	udp_assoc_t *assoc;
	udp_cassoc_t *cassoc;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_assoc_create_impl");

	assoc = udp_assoc_new(epp, NULL, NULL);
	if (assoc == NULL)
		return EIO;

	if (epp->local_link != 0)
		udp_assoc_set_iplink(assoc, epp->local_link);

	rc = udp_cassoc_create(client, assoc, &cassoc);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		udp_assoc_delete(assoc);
		return ENOMEM;
	}

	assoc->cb = &udp_cassoc_cb;
	assoc->cb_arg = cassoc;

	rc = udp_assoc_add(assoc);
	if (rc != EOK) {
		udp_cassoc_destroy(cassoc);
		udp_assoc_delete(assoc);
		return rc;
	}

	*rassoc_id = cassoc->id;
	return EOK;
}

/** Destroy association.
 *
 * Handle client request to destroy association (with parameters unmarshalled).
 *
 * @param client   UDP client
 * @param assoc_id Association ID
 * @return EOK on success, ENOENT if no such association is found
 */
static errno_t udp_assoc_destroy_impl(udp_client_t *client, sysarg_t assoc_id)
{
	udp_cassoc_t *cassoc;
	errno_t rc;

	rc = udp_cassoc_get(client, assoc_id, &cassoc);
	if (rc != EOK) {
		assert(rc == ENOENT);
		return ENOENT;
	}

	udp_assoc_remove(cassoc->assoc);
	udp_assoc_reset(cassoc->assoc);
	udp_assoc_delete(cassoc->assoc);
	udp_cassoc_destroy(cassoc);
	return EOK;
}

/** Set association sending messages with no local address.
 *
 * Handle client request to set nolocal flag (with parameters unmarshalled).
 *
 * @param client   UDP client
 * @param assoc_id Association ID
 * @return EOK on success, ENOENT if no such association is found
 */
static errno_t udp_assoc_set_nolocal_impl(udp_client_t *client, sysarg_t assoc_id)
{
	udp_cassoc_t *cassoc;
	errno_t rc;

	rc = udp_cassoc_get(client, assoc_id, &cassoc);
	if (rc != EOK) {
		assert(rc == ENOENT);
		return ENOENT;
	}

	log_msg(LOG_DEFAULT, LVL_NOTE, "Setting nolocal to true");
	cassoc->assoc->nolocal = true;
	return EOK;
}

/** Send message via association.
 *
 * Handle client request to send message (with parameters unmarshalled).
 *
 * @param client   UDP client
 * @param assoc_id Association ID
 * @param dest     Destination endpoint or @c NULL to use the default from
 *                 association
 * @param data     Message data
 * @param size     Message size
 *
 * @return EOK on success or an error code
 */
static errno_t udp_assoc_send_msg_impl(udp_client_t *client, sysarg_t assoc_id,
    inet_ep_t *dest, void *data, size_t size)
{
	udp_msg_t msg;
	udp_cassoc_t *cassoc;
	errno_t rc;

	rc = udp_cassoc_get(client, assoc_id, &cassoc);
	if (rc != EOK)
		return rc;

	msg.data = data;
	msg.data_size = size;
	rc = udp_assoc_send(cassoc->assoc, dest, &msg);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Create callback session.
 *
 * Handle client request to create callback session.
 *
 * @param client UDP client
 * @param icall  Async request data
 *
 */
static void udp_callback_create_srv(udp_client_t *client, ipc_call_t *icall)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_callback_create_srv()");

	async_sess_t *sess = async_callback_receive(EXCHANGE_SERIALIZE);
	if (sess == NULL) {
		async_answer_0(icall, ENOMEM);
		return;
	}

	client->sess = sess;
	async_answer_0(icall, EOK);
}

/** Create association.
 *
 * Handle client request to create association.
 *
 * @param client UDP client
 * @param icall  Async request data
 *
 */
static void udp_assoc_create_srv(udp_client_t *client, ipc_call_t *icall)
{
	ipc_call_t call;
	size_t size;
	inet_ep2_t epp;
	sysarg_t assoc_id;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_assoc_create_srv()");

	if (!async_data_write_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(inet_ep2_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_write_finalize(&call, &epp, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	rc = udp_assoc_create_impl(client, &epp, &assoc_id);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	async_answer_1(icall, EOK, assoc_id);
}

/** Destroy association.
 *
 * Handle client request to destroy association.
 *
 * @param client UDP client
 * @param icall  Async request data
 *
 */
static void udp_assoc_destroy_srv(udp_client_t *client, ipc_call_t *icall)
{
	sysarg_t assoc_id;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_assoc_destroy_srv()");

	assoc_id = ipc_get_arg1(icall);
	rc = udp_assoc_destroy_impl(client, assoc_id);
	async_answer_0(icall, rc);
}

/** Set association with no local address.
 *
 * Handle client request to set no local address flag.
 *
 * @param client UDP client
 * @param icall  Async request data
 *
 */
static void udp_assoc_set_nolocal_srv(udp_client_t *client, ipc_call_t *icall)
{
	sysarg_t assoc_id;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "udp_assoc_set_nolocal_srv()");

	assoc_id = ipc_get_arg1(icall);
	rc = udp_assoc_set_nolocal_impl(client, assoc_id);
	async_answer_0(icall, rc);
}

/** Send message via association.
 *
 * Handle client request to send message.
 *
 * @param client UDP client
 * @param icall  Async request data
 *
 */
static void udp_assoc_send_msg_srv(udp_client_t *client, ipc_call_t *icall)
{
	ipc_call_t call;
	size_t size;
	inet_ep_t dest;
	sysarg_t assoc_id;
	void *data;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_assoc_send_msg_srv()");

	/* Receive dest */

	if (!async_data_write_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(inet_ep_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_write_finalize(&call, &dest, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	/* Receive message data */

	if (!async_data_write_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size > MAX_MSG_SIZE) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	data = malloc(size);
	if (data == NULL) {
		async_answer_0(&call, ENOMEM);
		async_answer_0(icall, ENOMEM);
	}

	rc = async_data_write_finalize(&call, data, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		free(data);
		return;
	}

	assoc_id = ipc_get_arg1(icall);

	rc = udp_assoc_send_msg_impl(client, assoc_id, &dest, data, size);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		free(data);
		return;
	}

	async_answer_0(icall, EOK);
	free(data);
}

/** Get next received message.
 *
 * @param client UDP Client
 * @return Pointer to queue entry for next received message
 */
static udp_crcv_queue_entry_t *udp_rmsg_get_next(udp_client_t *client)
{
	link_t *link;

	link = list_first(&client->crcv_queue);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, udp_crcv_queue_entry_t, link);
}

/** Get info on first received message.
 *
 * Handle client request to get information on received message.
 *
 * @param client UDP client
 * @param icall  Async request data
 *
 */
static void udp_rmsg_info_srv(udp_client_t *client, ipc_call_t *icall)
{
	ipc_call_t call;
	size_t size;
	udp_crcv_queue_entry_t *enext;
	sysarg_t assoc_id;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_rmsg_info_srv()");
	enext = udp_rmsg_get_next(client);

	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (enext == NULL) {
		async_answer_0(&call, ENOENT);
		async_answer_0(icall, ENOENT);
		return;
	}

	rc = async_data_read_finalize(&call, &enext->epp.remote,
	    max(size, (size_t)sizeof(inet_ep_t)));
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	assoc_id = enext->cassoc->id;
	size = enext->msg->data_size;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_rmsg_info_srv(): assoc_id=%zu, "
	    "size=%zu", assoc_id, size);
	async_answer_2(icall, EOK, assoc_id, size);
}

/** Read data from first received message.
 *
 * Handle client request to read data from first received message.
 *
 * @param client UDP client
 * @param icall  Async request data
 *
 */
static void udp_rmsg_read_srv(udp_client_t *client, ipc_call_t *icall)
{
	ipc_call_t call;
	size_t msg_size;
	udp_crcv_queue_entry_t *enext;
	void *data;
	size_t size;
	size_t off;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_rmsg_read_srv()");
	off = ipc_get_arg1(icall);

	enext = udp_rmsg_get_next(client);

	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (enext == NULL) {
		async_answer_0(&call, ENOENT);
		async_answer_0(icall, ENOENT);
		return;
	}

	data = enext->msg->data + off;
	msg_size = enext->msg->data_size;

	if (off > msg_size) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_read_finalize(&call, data, min(msg_size - off, size));
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	async_answer_0(icall, EOK);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_rmsg_read_srv(): OK");
}

/** Discard first received message.
 *
 * Handle client request to discard first received message, advancing
 * to the next one.
 *
 * @param client UDP client
 * @param icall  Async request data
 *
 */
static void udp_rmsg_discard_srv(udp_client_t *client, ipc_call_t *icall)
{
	udp_crcv_queue_entry_t *enext;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_rmsg_discard_srv()");

	enext = udp_rmsg_get_next(client);
	if (enext == NULL) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "usg_rmsg_discard_srv: enext==NULL");
		async_answer_0(icall, ENOENT);
		return;
	}

	list_remove(&enext->link);
	udp_msg_delete(enext->msg);
	free(enext);
	async_answer_0(icall, EOK);
}

/** Handle UDP client connection.
 *
 * @param icall Connect call data
 * @param arg   Connection argument
 *
 */
static void udp_client_conn(ipc_call_t *icall, void *arg)
{
	udp_client_t client;
	unsigned long n;

	/* Accept the connection */
	async_accept_0(icall);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_client_conn()");

	client.sess = NULL;
	list_initialize(&client.cassoc);
	list_initialize(&client.crcv_queue);

	while (true) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_client_conn: wait req");
		ipc_call_t call;
		async_get_call(&call);
		sysarg_t method = ipc_get_imethod(&call);

		log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_client_conn: method=%d",
		    (int)method);
		if (!method) {
			/* The other side has hung up */
			async_answer_0(&call, EOK);
			break;
		}

		switch (method) {
		case UDP_CALLBACK_CREATE:
			udp_callback_create_srv(&client, &call);
			break;
		case UDP_ASSOC_CREATE:
			udp_assoc_create_srv(&client, &call);
			break;
		case UDP_ASSOC_DESTROY:
			udp_assoc_destroy_srv(&client, &call);
			break;
		case UDP_ASSOC_SET_NOLOCAL:
			udp_assoc_set_nolocal_srv(&client, &call);
			break;
		case UDP_ASSOC_SEND_MSG:
			udp_assoc_send_msg_srv(&client, &call);
			break;
		case UDP_RMSG_INFO:
			udp_rmsg_info_srv(&client, &call);
			break;
		case UDP_RMSG_READ:
			udp_rmsg_read_srv(&client, &call);
			break;
		case UDP_RMSG_DISCARD:
			udp_rmsg_discard_srv(&client, &call);
			break;
		default:
			async_answer_0(&call, ENOTSUP);
			break;
		}
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_client_conn: terminated");

	n = list_count(&client.cassoc);
	if (n != 0) {
		log_msg(LOG_DEFAULT, LVL_WARN, "udp_client_conn: "
		    "Client with %lu active associations closed session.", n);
		/* XXX Clean up */
	}

	/* XXX Clean up client receive queue */

	if (client.sess != NULL)
		async_hangup(client.sess);
}

/** Initialize UDP service.
 *
 * @return EOK on success or an error code.
 */
errno_t udp_service_init(void)
{
	errno_t rc;
	service_id_t sid;
	loc_srv_t *srv;

	async_set_fallback_port_handler(udp_client_conn, NULL);

	rc = loc_server_register(NAME, &srv);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering server.");
		return EIO;
	}

	rc = loc_service_register(srv, SERVICE_NAME_UDP, &sid);
	if (rc != EOK) {
		loc_server_unregister(srv);
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering service.");
		return EIO;
	}

	return EOK;
}

/**
 * @}
 */
