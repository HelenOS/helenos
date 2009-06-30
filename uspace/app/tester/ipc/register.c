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

#include <stdio.h>
#include <unistd.h>
#include <async.h>
#include <errno.h>
#include "../tester.h"

#define MAX_CONNECTIONS  50

static int connections[MAX_CONNECTIONS];

static void client_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	unsigned int i;
	
	TPRINTF("Connected phone %#x accepting\n", icall->in_phone_hash);
	ipc_answer_0(iid, EOK);
	for (i = 0; i < MAX_CONNECTIONS; i++) {
		if (!connections[i]) {
			connections[i] = icall->in_phone_hash;
			break;
		}
	}
	
	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		int retval;
		
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			TPRINTF("Phone %#x hung up\n", icall->in_phone_hash);
			retval = 0;
			break;
		case IPC_TEST_METHOD:
			TPRINTF("Received well known message from %#x: %#x\n",
			    icall->in_phone_hash, callid);
			ipc_answer_0(callid, EOK);
			break;
		default:
			TPRINTF("Received unknown message from %#x: %#x\n",
			    icall->in_phone_hash, callid);
			ipc_answer_0(callid, ENOENT);
			break;
		}
	}
}

char *test_register(void)
{
	async_set_client_connection(client_connection);
	
	ipcarg_t phonead;
	int res = ipc_connect_to_me(PHONE_NS, IPC_TEST_SERVICE, 0, 0, &phonead);
	if (res != 0)
		return "Failed registering IPC service";
	
	TPRINTF("Registered as service %u, accepting connections\n", IPC_TEST_SERVICE);
	async_manager();
	
	return NULL;
}
