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

static void client_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	ipc_call_t call;
	ipcarg_t phonehash = icall->in_phone_hash;
	int retval;
	int i;

	printf("Connected phone: %P, accepting\n", icall->in_phone_hash);
	ipc_answer_0(iid, EOK);
	for (i = 0; i < 1024; i++)
		if (!connections[i]) {
			connections[i] = phonehash;
			break;
		}
	
	while (1) {
		callid = async_get_call(&call);
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			printf("Phone (%P) hung up.\n", phonehash);
			retval = 0;
			break;
		default:
			printf("Received message from %P: %X\n", phonehash,
			    callid);
			for (i = 0; i < 1024; i++)
				if (!callids[i]) {
					callids[i] = callid;
					break;
				}
			continue;
		}
		ipc_answer_0(callid, retval);
	}
}

char * test_register(bool quiet)
{
	int i;
	
	async_set_client_connection(client_connection);

	for (i = IPC_TEST_START; i < IPC_TEST_START + 10; i++) {
		ipcarg_t phonead;
		int res = ipc_connect_to_me(PHONE_NS, i, 0, &phonead);
		if (!res)
			break;
		printf("Failed registering as %d..:%d\n", i, res);
	}
	printf("Registered as service: %d\n", i);
	myservice = i;
	
	return NULL;
}
