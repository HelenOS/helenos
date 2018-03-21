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
#include <str_error.h>
#include <io/log.h>
#include <ipc/dnsr.h>
#include <ipc/services.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <task.h>

#include "dns_msg.h"
#include "dns_std.h"
#include "query.h"
#include "transport.h"

#define NAME  "dnsres"

static void dnsr_client_conn(cap_call_handle_t, ipc_call_t *, void *);

static errno_t dnsr_init(void)
{
	errno_t rc;
	log_msg(LOG_DEFAULT, LVL_DEBUG, "dnsr_init()");

	rc = transport_init();
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed initializing transport.");
		return EIO;
	}

	async_set_fallback_port_handler(dnsr_client_conn, NULL);

	rc = loc_server_register(NAME);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering server: %s.", str_error(rc));
		transport_fini();
		return EEXIST;
	}

	service_id_t sid;
	rc = loc_service_register(SERVICE_NAME_DNSR, &sid);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering service: %s.", str_error(rc));
		transport_fini();
		return EEXIST;
	}

	return EOK;
}

static void dnsr_name2host_srv(dnsr_client_t *client, cap_call_handle_t icall_handle,
    ipc_call_t *icall)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_get_srvaddr_srv()");

	ip_ver_t ver = IPC_GET_ARG1(*icall);

	char *name;
	errno_t rc = async_data_write_accept((void **) &name, true, 0,
	    DNS_NAME_MAX_SIZE, 0, NULL);
	if (rc != EOK) {
		async_answer_0(icall_handle, rc);
		return;
	}

	dns_host_info_t *hinfo;
	rc = dns_name2host(name, &hinfo, ver);
	if (rc != EOK) {
		async_answer_0(icall_handle, rc);
		return;
	}

	cap_call_handle_t chandle;
	size_t size;
	if (!async_data_read_receive(&chandle, &size)) {
		async_answer_0(chandle, EREFUSED);
		async_answer_0(icall_handle, EREFUSED);
		return;
	}

	if (size != sizeof(inet_addr_t)) {
		async_answer_0(chandle, EINVAL);
		async_answer_0(icall_handle, EINVAL);
		return;
	}

	rc = async_data_read_finalize(chandle, &hinfo->addr, size);
	if (rc != EOK) {
		async_answer_0(chandle, rc);
		async_answer_0(icall_handle, rc);
		return;
	}

	if (!async_data_read_receive(&chandle, &size)) {
		async_answer_0(chandle, EREFUSED);
		async_answer_0(icall_handle, EREFUSED);
		return;
	}

	size_t act_size = str_size(hinfo->cname);
	if (act_size > size) {
		async_answer_0(chandle, EINVAL);
		async_answer_0(icall_handle, EINVAL);
		return;
	}

	rc = async_data_read_finalize(chandle, hinfo->cname, act_size);
	if (rc != EOK)
		async_answer_0(chandle, rc);

	async_answer_0(icall_handle, rc);

	dns_hostinfo_destroy(hinfo);
}

static void dnsr_get_srvaddr_srv(dnsr_client_t *client, cap_call_handle_t icall_handle,
    ipc_call_t *icall)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_get_srvaddr_srv()");

	cap_call_handle_t chandle;
	size_t size;
	if (!async_data_read_receive(&chandle, &size)) {
		async_answer_0(chandle, EREFUSED);
		async_answer_0(icall_handle, EREFUSED);
		return;
	}

	if (size != sizeof(inet_addr_t)) {
		async_answer_0(chandle, EINVAL);
		async_answer_0(icall_handle, EINVAL);
		return;
	}

	// FIXME locking

	errno_t rc = async_data_read_finalize(chandle, &dns_server_addr, size);
	if (rc != EOK)
		async_answer_0(chandle, rc);

	async_answer_0(icall_handle, rc);
}

static void dnsr_set_srvaddr_srv(dnsr_client_t *client, cap_call_handle_t icall_handle,
    ipc_call_t *icall)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "dnsr_set_srvaddr_srv()");

	cap_call_handle_t chandle;
	size_t size;
	if (!async_data_write_receive(&chandle, &size)) {
		async_answer_0(chandle, EREFUSED);
		async_answer_0(icall_handle, EREFUSED);
		return;
	}

	if (size != sizeof(inet_addr_t)) {
		async_answer_0(chandle, EINVAL);
		async_answer_0(icall_handle, EINVAL);
		return;
	}

	// FIXME locking

	errno_t rc = async_data_write_finalize(chandle, &dns_server_addr, size);
	if (rc != EOK) {
		async_answer_0(chandle, rc);
		async_answer_0(icall_handle, rc);
	}

	async_answer_0(icall_handle, rc);
}

static void dnsr_client_conn(cap_call_handle_t icall_handle, ipc_call_t *icall, void *arg)
{
	dnsr_client_t client;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "dnsr_conn()");

	/* Accept the connection */
	async_answer_0(icall_handle, EOK);

	while (true) {
		ipc_call_t call;
		cap_call_handle_t chandle = async_get_call(&call);
		sysarg_t method = IPC_GET_IMETHOD(call);

		if (!method) {
			/* The other side has hung up */
			async_answer_0(chandle, EOK);
			return;
		}

		switch (method) {
		case DNSR_NAME2HOST:
			dnsr_name2host_srv(&client, chandle, &call);
			break;
		case DNSR_GET_SRVADDR:
			dnsr_get_srvaddr_srv(&client, chandle, &call);
			break;
		case DNSR_SET_SRVADDR:
			dnsr_set_srvaddr_srv(&client, chandle, &call);
			break;
		default:
			async_answer_0(chandle, EINVAL);
		}
	}
}

int main(int argc, char *argv[])
{
	errno_t rc;

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
