/*
 * Copyright (C) 2006 Ondrej Palkovsky
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

#include <ipc/ipc.h>
#include <ipc/ns.h>
#include <print.h>
#include <proc/thread.h>
#include <arch.h>
#include <panic.h>

static answerbox_t ns_answerbox;

/**   */
static void ns_thread(void *data)
{
	call_t *call;

	printf("Name service started.\n");
	while (1) {
		call = ipc_wait_for_call(&ns_answerbox, 0);
		switch (IPC_GET_METHOD(call->data)) {
		case NS_PING:
			printf("Ping.\n");
			IPC_SET_RETVAL(call->data, 0);
			break;
		default:
			printf("Unsupported name service call.\n");
			IPC_SET_RETVAL(call->data, -1);
		}
		ipc_answer(&ns_answerbox, call);
	}
}

/** Name service initialization and start
 *
 * This must be started before any task that communicates with name service
 */
void ns_start(void)
{
	thread_t *t;

	ipc_answerbox_init(&ns_answerbox);
	ipc_phone_0 = &ns_answerbox;
	
	if ((t = thread_create(ns_thread, NULL, TASK, 0)))
		thread_ready(t);
	else
		panic("thread_create/phonecompany");
}
