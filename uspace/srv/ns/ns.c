/*
 * Copyright (c) 2006 Ondrej Palkovsky
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

/** @addtogroup ns
 * @{
 */

/**
 * @file  ns.c
 * @brief Naming service for HelenOS IPC.
 */

#include <abi/ipc/methods.h>
#include <async.h>
#include <ipc/ns.h>
#include <ipc/services.h>
#include <abi/ipc/interfaces.h>
#include <stdio.h>
#include <errno.h>
#include <macros.h>
#include "ns.h"
#include "service.h"
#include "clonable.h"
#include "task.h"

static void ns_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	ipc_call_t call;
	ipc_callid_t callid;
	iface_t iface;
	service_t service;

	iface = IPC_GET_ARG1(*icall);
	service = IPC_GET_ARG2(*icall);
	if (service != 0) {
		/*
		 * Client requests to be connected to a service.
		 */
		if (service_clonable(service)) {
			connect_to_clonable(service, iface, icall, iid);
		} else {
			connect_to_service(service, iface, icall, iid);
		}
		return;
	}

	async_answer_0(iid, EOK);

	while (true) {
		process_pending_conn();

		callid = async_get_call(&call);
		if (!IPC_GET_IMETHOD(call))
			break;

		task_id_t id;
		errno_t retval;

		service_t service;
		sysarg_t phone;

		switch (IPC_GET_IMETHOD(call)) {
		case NS_REGISTER:
			service = IPC_GET_ARG1(call);
			phone = IPC_GET_ARG5(call);

			/*
			 * Server requests service registration.
			 */
			if (service_clonable(service)) {
				register_clonable(service, phone, &call, callid);
				continue;
			} else {
				retval = register_service(service, phone, &call);
			}

			break;
		case NS_PING:
			retval = EOK;
			break;
		case NS_TASK_WAIT:
			id = (task_id_t)
			    MERGE_LOUP32(IPC_GET_ARG1(call), IPC_GET_ARG2(call));
			wait_for_task(id, &call, callid);
			continue;
		case NS_ID_INTRO:
			retval = ns_task_id_intro(&call);
			break;
		case NS_RETVAL:
			retval = ns_task_retval(&call);
			break;
		default:
			printf("ns: method not supported\n");
			retval = ENOTSUP;
			break;
		}

		async_answer_0(callid, retval);
	}

	(void) ns_task_disconnect(&call);
}

int main(int argc, char **argv)
{
	printf("%s: HelenOS IPC Naming Service\n", NAME);

	errno_t rc = service_init();
	if (rc != EOK)
		return rc;

	rc = clonable_init();
	if (rc != EOK)
		return rc;

	rc = task_init();
	if (rc != EOK)
		return rc;

	async_set_fallback_port_handler(ns_connection, NULL);

	printf("%s: Accepting connections\n", NAME);
	async_manager();

	/* Not reached */
	return 0;
}

/**
 * @}
 */
