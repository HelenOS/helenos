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

/** @addtogroup inet
 * @{
 */
/**
 * @file
 * @brief
 */

#include <async.h>
#include <errno.h>
#include <fibril_synch.h>
#include <io/log.h>
#include <ipc/inet.h>
#include <loc.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <types/inetping.h>
#include "icmp.h"
#include "icmpv6.h"
#include "icmp_std.h"
#include "inetsrv.h"
#include "inetping.h"

static FIBRIL_MUTEX_INITIALIZE(client_list_lock);
static LIST_INITIALIZE(client_list);

/** Last used session identifier. Protected by @c client_list_lock */
static uint16_t inetping_ident = 0;

static errno_t inetping_send(inetping_client_t *client, inetping_sdu_t *sdu)
{
	if (sdu->src.version != sdu->dest.version)
		return EINVAL;

	switch (sdu->src.version) {
	case ip_v4:
		return icmp_ping_send(client->ident, sdu);
	case ip_v6:
		return icmpv6_ping_send(client->ident, sdu);
	default:
		return EINVAL;
	}
}

static errno_t inetping_get_srcaddr(inetping_client_t *client,
    inet_addr_t *remote, inet_addr_t *local)
{
	return inet_get_srcaddr(remote, ICMP_TOS, local);
}

static inetping_client_t *inetping_client_find(uint16_t ident)
{
	fibril_mutex_lock(&client_list_lock);

	list_foreach(client_list, client_list, inetping_client_t, client) {
		if (client->ident == ident) {
			fibril_mutex_unlock(&client_list_lock);
			return client;
		}
	}

	fibril_mutex_unlock(&client_list_lock);
	return NULL;
}

errno_t inetping_recv(uint16_t ident, inetping_sdu_t *sdu)
{
	inetping_client_t *client = inetping_client_find(ident);
	if (client == NULL) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Unknown ICMP ident. Dropping.");
		return ENOENT;
	}

	async_exch_t *exch = async_exchange_begin(client->sess);

	ipc_call_t answer;
	aid_t req = async_send_1(exch, INETPING_EV_RECV, sdu->seq_no, &answer);

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

static void inetping_send_srv(inetping_client_t *client, ipc_call_t *icall)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetping_send_srv()");

	inetping_sdu_t sdu;
	errno_t rc;

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

	rc = async_data_write_finalize(&call, &sdu.src, size);
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

	rc = async_data_write_accept((void **) &sdu.data, false, 0, 0, 0,
	    &sdu.size);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	rc = inetping_send(client, &sdu);
	free(sdu.data);

	async_answer_0(icall, rc);
}

static void inetping_get_srcaddr_srv(inetping_client_t *client, ipc_call_t *icall)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetping_get_srcaddr_srv()");

	ipc_call_t call;
	size_t size;

	inet_addr_t local;
	inet_addr_t remote;

	if (!async_data_write_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(remote)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	errno_t rc = async_data_write_finalize(&call, &remote, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	rc = inetping_get_srcaddr(client, &remote, &local);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(local)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_read_finalize(&call, &local, size);
	if (rc != EOK)
		async_answer_0(&call, rc);

	async_answer_0(icall, rc);
}

static errno_t inetping_client_init(inetping_client_t *client)
{
	async_sess_t *sess = async_callback_receive(EXCHANGE_SERIALIZE);
	if (sess == NULL)
		return ENOMEM;

	client->sess = sess;
	link_initialize(&client->client_list);

	fibril_mutex_lock(&client_list_lock);
	client->ident = ++inetping_ident;
	list_append(&client->client_list, &client_list);
	fibril_mutex_unlock(&client_list_lock);

	return EOK;
}

static void inetping_client_fini(inetping_client_t *client)
{
	async_hangup(client->sess);
	client->sess = NULL;

	fibril_mutex_lock(&client_list_lock);
	list_remove(&client->client_list);
	fibril_mutex_unlock(&client_list_lock);
}

void inetping_conn(ipc_call_t *icall, void *arg)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetping_conn()");

	/* Accept the connection */
	async_accept_0(icall);

	inetping_client_t client;
	errno_t rc = inetping_client_init(&client);
	if (rc != EOK)
		return;

	while (true) {
		ipc_call_t call;
		async_get_call(&call);
		sysarg_t method = ipc_get_imethod(&call);

		if (!method) {
			/* The other side has hung up */
			async_answer_0(&call, EOK);
			break;
		}

		switch (method) {
		case INETPING_SEND:
			inetping_send_srv(&client, &call);
			break;
		case INETPING_GET_SRCADDR:
			inetping_get_srcaddr_srv(&client, &call);
			break;
		default:
			async_answer_0(&call, EINVAL);
		}
	}

	inetping_client_fini(&client);
}

/** @}
 */
