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

/** @addtogroup dnsres
 * @{
 */
/**
 * @file
 */

#include <async.h>
#include <errno.h>
#include <io/log.h>
#include <ipc/dnsr.h>
#include <ipc/services.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>
#include <task.h>

#include "dns_msg.h"
#include "dns_std.h"
#include "query.h"
#include "transport.h"

#define NAME  "dnsres"

static void dnsr_client_conn(ipc_callid_t, ipc_call_t *, void *);

static int dnsr_init(void)
{
	int rc;
	log_msg(LOG_DEFAULT, LVL_DEBUG, "dnsr_init()");

	rc = transport_init();
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed initializing tarnsport.");
		return EIO;
	}

	async_set_client_connection(dnsr_client_conn);

	rc = loc_server_register(NAME);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering server (%d).", rc);
		transport_fini();
		return EEXIST;
	}

	service_id_t sid;
	rc = loc_service_register(SERVICE_NAME_DNSR, &sid);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering service (%d).", rc);
		transport_fini();
		return EEXIST;
	}

	return EOK;
}

static void dnsr_name2host_srv(dnsr_client_t *client, ipc_callid_t callid,
    ipc_call_t *call)
{
	char *name;
	dns_host_info_t *hinfo;
	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_get_srvaddr_srv()");

	rc = async_data_write_accept((void **) &name, true, 0,
	    DNS_NAME_MAX_SIZE, 0, NULL);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return;
	}

	rc = dns_name2host(name, &hinfo);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return;
	}

	async_answer_1(callid, EOK, hinfo->addr.ipv4);

	dns_hostinfo_destroy(hinfo);
}

static void dnsr_get_srvaddr_srv(dnsr_client_t *client, ipc_callid_t callid,
    ipc_call_t *call)
{
//	inet_addr_t remote;
//	uint8_t tos;
//	inet_addr_t local;
//	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_get_srvaddr_srv()");

/*	remote.ipv4 = IPC_GET_ARG1(*call);
	tos = IPC_GET_ARG2(*call);
	local.ipv4 = 0;

	rc = inet_get_srcaddr(&remote, tos, &local);*/
	async_answer_1(callid, EOK, 0x01020304);
}

static void dnsr_set_srvaddr_srv(dnsr_client_t *client, ipc_callid_t callid,
    ipc_call_t *call)
{
//	inet_addr_t naddr;
//	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "dnsr_set_srvaddr_srv()");

//	naddr.ipv4 = IPC_GET_ARG1(*call);

	/*rc = inet_get_srcaddr(&remote, tos, &local);*/
	async_answer_0(callid, EOK);
}

static void dnsr_client_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	dnsr_client_t client;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "dnsr_conn()");

	/* Accept the connection */
	async_answer_0(iid, EOK);

//	inet_client_init(&client);

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
		case DNSR_NAME2HOST:
			dnsr_name2host_srv(&client, callid, &call);
			break;
		case DNSR_GET_SRVADDR:
			dnsr_get_srvaddr_srv(&client, callid, &call);
			break;
		case DNSR_SET_SRVADDR:
			dnsr_set_srvaddr_srv(&client, callid, &call);
			break;
		default:
			async_answer_0(callid, EINVAL);
		}
	}

//	inet_client_fini(&client);
}

int main(int argc, char *argv[])
{
	int rc;

	printf("%s: DNS Resolution Service\n", NAME);

	if (log_init(NAME) != EOK) {
		printf(NAME ": Failed to initialize logging.\n");
		return 1;
	}

	rc = dnsr_init();
	if (rc != EOK)
		return 1;

	printf(NAME ": Accepting connections.\n");
	task_retval(0);
	async_manager();

	return 0;
}

/** @}
 */
