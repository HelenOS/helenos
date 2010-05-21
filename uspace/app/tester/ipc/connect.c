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
#include <atomic.h>
#include "../tester.h"

static atomic_t finish;

static void callback(void *priv, int retval, ipc_call_t *data)
{
	atomic_set(&finish, 1);
}

const char *test_connect(void)
{
	TPRINTF("Connecting to %u...", IPC_TEST_SERVICE);
	int phone = ipc_connect_me_to(PHONE_NS, IPC_TEST_SERVICE, 0, 0);
	if (phone > 0) {
		TPRINTF("phoneid %d\n", phone);
	} else {
		TPRINTF("\n");
		return "ipc_connect_me_to() failed";
	}
	
	printf("Sending synchronous message...\n");
	int retval = ipc_call_sync_0_0(phone, IPC_TEST_METHOD);
	TPRINTF("Received response to synchronous message\n");
	
	TPRINTF("Sending asynchronous message...\n");
	atomic_set(&finish, 0);
	ipc_call_async_0(phone, IPC_TEST_METHOD, NULL, callback, 1);
	while (atomic_get(&finish) != 1)
		TPRINTF(".");
	TPRINTF("Received response to asynchronous message\n");
	
	TPRINTF("Hanging up...");
	retval = ipc_hangup(phone);
	if (retval == 0) {
		TPRINTF("OK\n");
	} else {
		TPRINTF("\n");
		return "ipc_hangup() failed";
	}
	
	return NULL;
}
