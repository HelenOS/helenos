/*
 * Copyright (c) 2015 Michal Koutny
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <errno.h>
#include <ipc/taskman.h>
#include <macros.h>
#include <task.h>

#include "private/task.h"

static task_event_handler_t task_event_handler = NULL;

static void taskman_task_event(ipc_callid_t iid, ipc_call_t *icall)
{
	task_id_t tid = (task_id_t)
	    MERGE_LOUP32(IPC_GET_ARG1(*icall), IPC_GET_ARG2(*icall));
	int flags = IPC_GET_ARG3(*icall);
	task_exit_t texit = IPC_GET_ARG4(*icall);
	int retval = IPC_GET_ARG5(*icall);

	task_event_handler(tid, flags, texit, retval);

	async_answer_0(iid, EOK);
}

static void taskman_event_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	/* Accept connection */
	async_answer_0(iid, EOK);

	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);

		if (!IPC_GET_IMETHOD(call)) {
			/* Hangup, TODO explain or handle differntly */
			break;
		}

		switch (IPC_GET_IMETHOD(call)) {
		case TASKMAN_EV_TASK:
			taskman_task_event(callid, &call);
			break;
		default:
			async_answer_0(callid, ENOTSUP);
			break;
		}
	}
}

/**
 * Blocks, calls handler in another fibril
 */
int task_register_event_handler(task_event_handler_t handler)
{
	/*
	 * so far support assign once, modification cannot be naïve due to
	 * races
	 */
	assert(task_event_handler == NULL);
	assert(handler != NULL); /* no support for "unregistration" */

	task_event_handler = handler;

	async_exch_t *exch = taskman_exchange_begin();
	aid_t req = async_send_0(exch, TASKMAN_EVENT_CALLBACK, NULL);

	int rc = async_connect_to_me(exch, 0, 0, 0, taskman_event_conn, NULL);
	taskman_exchange_end(exch);

	if (rc != EOK) {
		return rc;
	}

	sysarg_t retval;
	async_wait_for(req, &retval);
	return retval;
}

/** @}
 */
