/*
 * Copyright (c) 2018 Jiri Svoboda
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

#include <async.h>
#include <errno.h>
#include <str_error.h>
#include <ipc/ipc_test.h>
#include <ipc/services.h>
#include <loc.h>
#include <mem.h>
#include <stdio.h>
#include <task.h>

#define NAME  "ipc-test"

static service_id_t svc_id;

static void ipc_test_connection(ipc_call_t *icall, void *arg)
{
	/* Accept connection */
	async_accept_0(icall);

	while (true) {
		ipc_call_t call;
		async_get_call(&call);

		if (!IPC_GET_IMETHOD(call)) {
			async_answer_0(&call, EOK);
			break;
		}

		switch (IPC_GET_IMETHOD(call)) {
		case IPC_TEST_PING:
			async_answer_0(&call, EOK);
			break;
		default:
			async_answer_0(&call, ENOTSUP);
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	errno_t rc;

	printf("%s: IPC test service\n", NAME);
	async_set_fallback_port_handler(ipc_test_connection, NULL);

	rc = loc_server_register(NAME);
	if (rc != EOK) {
		printf("%s: Failed registering server. (%s)\n", NAME,
		    str_error(rc));
		return rc;
	}

	rc = loc_service_register(SERVICE_NAME_IPC_TEST, &svc_id);
	if (rc != EOK) {
		printf("%s: Failed registering service. (%s)\n", NAME,
		    str_error(rc));
		return rc;
	}

	printf("%s: Accepting connections\n", NAME);
	task_retval(0);
	async_manager();

	/* Not reached */
	return 0;
}
