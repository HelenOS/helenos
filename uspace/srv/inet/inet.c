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
 * @brief Internet Protocol service
 */

#include <adt/list.h>
#include <async.h>
#include <errno.h>
#include <fibril_synch.h>
#include <io/log.h>
#include <ipc/inet.h>
#include <ipc/services.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#define NAME "inet"

/** Inet Client */
typedef struct {
	async_sess_t *sess;
	uint8_t protocol;
	link_t client_list;
} inet_client_t;

static void inet_client_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg);

static FIBRIL_MUTEX_INITIALIZE(client_list_lock);
static LIST_INITIALIZE(client_list);

static int inet_init(void)
{
	service_id_t sid;
	int rc;

	log_msg(LVL_DEBUG, "inet_init()");

	async_set_client_connection(inet_client_conn);

	rc = loc_server_register(NAME);
	if (rc != EOK) {
		log_msg(LVL_ERROR, "Failed registering server (%d).", rc);
		return EEXIST;
	}

	rc = loc_service_register(SERVICE_NAME_INET, &sid);
	if (rc != EOK) {
		log_msg(LVL_ERROR, "Failed registering service (%d).", rc);
		return EEXIST;
	}

	return EOK;
}

static void inet_callback_create(inet_client_t *client, ipc_callid_t callid,
    ipc_call_t *call)
{
	log_msg(LVL_DEBUG, "inet_callback_create()");

	async_sess_t *sess = async_callback_receive(EXCHANGE_SERIALIZE);
	if (sess == NULL) {
		async_answer_0(callid, ENOMEM);
		return;
	}

	client->sess = sess;
	async_answer_0(callid, EOK);
}

static void inet_get_srcaddr(inet_client_t *client, ipc_callid_t callid,
    ipc_call_t *call)
{
	log_msg(LVL_DEBUG, "inet_get_srcaddr()");

	async_answer_0(callid, ENOTSUP);
}

static void inet_send(inet_client_t *client, ipc_callid_t callid,
    ipc_call_t *call)
{
	uint32_t src_ipv4;
	uint32_t dest_ipv4;
	uint8_t tos;
	uint8_t ttl;
	int df;
	void *data;
	size_t size;
	int rc;

	log_msg(LVL_DEBUG, "inet_send()");

	src_ipv4 = IPC_GET_ARG1(*call);
	dest_ipv4 = IPC_GET_ARG2(*call);
	tos = IPC_GET_ARG3(*call);
	ttl = IPC_GET_ARG4(*call);
	df = IPC_GET_ARG5(*call);

	(void)src_ipv4;
	(void)dest_ipv4;
	(void)tos;
	(void)ttl;
	(void)df;

	rc = async_data_write_accept(&data, false, 0, 0, 0, &size);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return;
	}

	free(data);
	async_answer_0(callid, ENOTSUP);
}

static void inet_set_proto(inet_client_t *client, ipc_callid_t callid,
    ipc_call_t *call)
{
	sysarg_t proto;

	proto = IPC_GET_ARG1(*call);
	log_msg(LVL_DEBUG, "inet_set_proto(%lu)", (unsigned long) proto);

	if (proto > UINT8_MAX) {
		async_answer_0(callid, EINVAL);
		return;
	}

	client->protocol = proto;
	async_answer_0(callid, EOK);
}

static void inet_client_init(inet_client_t *client)
{
	client->sess = NULL;

	fibril_mutex_lock(&client_list_lock);
	list_append(&client->client_list, &client_list);
	fibril_mutex_unlock(&client_list_lock);
}

static void inet_client_fini(inet_client_t *client)
{
	async_hangup(client->sess);
	client->sess = NULL;

	fibril_mutex_lock(&client_list_lock);
	list_remove(&client->client_list);
	fibril_mutex_unlock(&client_list_lock);
}

static void inet_client_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	inet_client_t client;

	log_msg(LVL_DEBUG, "inet_client_conn()");

	/* Accept the connection */
	async_answer_0(iid, EOK);

	inet_client_init(&client);

	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		sysarg_t method = IPC_GET_IMETHOD(call);

		if (!method) {
			/* The other side has hung up */
			async_answer_0(callid, EOK);
			return;
		}

		switch (method) {
		case INET_CALLBACK_CREATE:
			inet_callback_create(&client, callid, &call);
			break;
		case INET_GET_SRCADDR:
			inet_get_srcaddr(&client, callid, &call);
			break;
		case INET_SEND:
			inet_send(&client, callid, &call);
			break;
		case INET_SET_PROTO:
			inet_set_proto(&client, callid, &call);
			break;
		default:
			async_answer_0(callid, EINVAL);
		}
	}

	inet_client_fini(&client);
}

int main(int argc, char *argv[])
{
	int rc;

	printf(NAME ": HelenOS Internet Protocol service");

	if (log_init(NAME, LVL_DEBUG) != EOK) {
		printf(NAME ": Failed to initialize logging.");
		return 1;
	}

	rc = inet_init();
	if (rc != EOK)
		return 1;

	printf(NAME ": Accepting connections.\n");
	task_retval(0);
	async_manager();

	/* Not reached */
	return 0;
}

/** @}
 */
