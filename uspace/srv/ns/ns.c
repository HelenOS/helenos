/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

static void ns_connection(ipc_call_t *icall, void *arg)
{
	ipc_call_t call;
	iface_t iface;
	service_t service;

	iface = ipc_get_arg1(icall);
	service = ipc_get_arg2(icall);
	if (service != 0) {
		/*
		 * Client requests to be connected to a service.
		 */
		if (ns_service_is_clonable(service, iface)) {
			ns_clonable_forward(service, iface, icall);
		} else {
			ns_service_forward(service, iface, icall);
		}

		return;
	}

	async_accept_0(icall);

	while (true) {
		ns_pending_conn_process();

		async_get_call(&call);
		if (!ipc_get_imethod(&call))
			break;

		task_id_t id;
		errno_t retval;

		service_t service;

		switch (ipc_get_imethod(&call)) {
		case NS_REGISTER:
			service = ipc_get_arg1(&call);
			iface = ipc_get_arg2(&call);

			/*
			 * Server requests service registration.
			 */
			if (ns_service_is_clonable(service, iface)) {
				ns_clonable_register(&call);
				continue;
			} else {
				retval = ns_service_register(service, iface);
			}

			break;
		case NS_REGISTER_BROKER:
			service = ipc_get_arg1(&call);
			retval = ns_service_register_broker(service);
			break;
		case NS_PING:
			retval = EOK;
			break;
		case NS_TASK_WAIT:
			id = (task_id_t)
			    MERGE_LOUP32(ipc_get_arg1(&call), ipc_get_arg2(&call));
			wait_for_task(id, &call);
			continue;
		case NS_ID_INTRO:
			retval = ns_task_id_intro(&call);
			break;
		case NS_RETVAL:
			retval = ns_task_retval(&call);
			break;
		default:
			printf("%s: Method not supported (%" PRIun ")\n",
			    NAME, ipc_get_imethod(&call));
			retval = ENOTSUP;
			break;
		}

		async_answer_0(&call, retval);
	}

	(void) ns_task_disconnect(&call);
	async_answer_0(&call, EOK);
}

int main(int argc, char **argv)
{
	printf("%s: HelenOS IPC Naming Service\n", NAME);

	errno_t rc = ns_service_init();
	if (rc != EOK)
		return rc;

	rc = ns_clonable_init();
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
