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
#include <sys/types.h>
#include <types/inetping.h>
#include <net/socket_codes.h>
#include "icmp.h"
#include "icmp_std.h"
#include "inetsrv.h"
#include "inetping.h"

static FIBRIL_MUTEX_INITIALIZE(client_list_lock);
static LIST_INITIALIZE(client_list);

/** Last used session identifier. Protected by @c client_list_lock */
static uint16_t inetping_ident = 0;

static int inetping_send(inetping_client_t *client, inetping_sdu_t *sdu)
{
	return icmp_ping_send(client->ident, sdu);
}

static int inetping_get_srcaddr(inetping_client_t *client, addr32_t remote,
    addr32_t *local)
{
	inet_addr_t remote_addr;
	inet_addr_set(remote, &remote_addr);
	
	inet_addr_t local_addr;
	int rc = inet_get_srcaddr(&remote_addr, ICMP_TOS, &local_addr);
	if (rc != EOK)
		return rc;
	
	ip_ver_t ver = inet_addr_get(&local_addr, local, NULL);
	if (ver != ip_v4)
		return EINVAL;
	
	return EOK;
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

int inetping_recv(uint16_t ident, inetping_sdu_t *sdu)
{
	inetping_client_t *client = inetping_client_find(ident);
	if (client == NULL) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Unknown ICMP ident. Dropping.");
		return ENOENT;
	}
	
	async_exch_t *exch = async_exchange_begin(client->sess);
	
	ipc_call_t answer;
	aid_t req = async_send_3(exch, INETPING_EV_RECV, (sysarg_t) sdu->src,
	    (sysarg_t) sdu->dest, sdu->seq_no, &answer);
	int rc = async_data_write_start(exch, sdu->data, sdu->size);
	
	async_exchange_end(exch);
	
	if (rc != EOK) {
		async_forget(req);
		return rc;
	}
	
	sysarg_t retval;
	async_wait_for(req, &retval);
	
	return (int) retval;
}

static void inetping_send_srv(inetping_client_t *client, ipc_callid_t callid,
    ipc_call_t *call)
{
	inetping_sdu_t sdu;
	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetping_send_srv()");

	rc = async_data_write_accept((void **) &sdu.data, false, 0, 0, 0,
	    &sdu.size);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return;
	}

	sdu.src = IPC_GET_ARG1(*call);
	sdu.dest = IPC_GET_ARG2(*call);
	sdu.seq_no = IPC_GET_ARG3(*call);

	rc = inetping_send(client, &sdu);
	free(sdu.data);

	async_answer_0(callid, rc);
}

static void inetping_get_srcaddr_srv(inetping_client_t *client,
    ipc_callid_t callid, ipc_call_t *call)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetping_get_srcaddr_srv()");
	
	uint32_t remote = IPC_GET_ARG1(*call);
	uint32_t local = 0;
	
	int rc = inetping_get_srcaddr(client, remote, &local);
	async_answer_1(callid, rc, (sysarg_t) local);
}

static int inetping_client_init(inetping_client_t *client)
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

void inetping_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetping_conn()");
	
	/* Accept the connection */
	async_answer_0(iid, EOK);
	
	inetping_client_t client;
	int rc = inetping_client_init(&client);
	if (rc != EOK)
		return;
	
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
		case INETPING_SEND:
			inetping_send_srv(&client, callid, &call);
			break;
		case INETPING_GET_SRCADDR:
			inetping_get_srcaddr_srv(&client, callid, &call);
			break;
		default:
			async_answer_0(callid, EINVAL);
		}
	}
	
	inetping_client_fini(&client);
}

/** @}
 */
