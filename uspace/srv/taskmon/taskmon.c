/*
 * Copyright (c) 2010 Jiri Svoboda
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

/** @addtogroup taskmon
 * @brief
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <ipc/ipc.h>
#include <async.h>
#include <ipc/services.h>
#include <task.h>
#include <event.h>
#include <macros.h>
#include <errno.h>

#define NAME  "taskmon"

static void fault_event(ipc_callid_t callid, ipc_call_t *call)
{
	char *argv[5];
	char *fname;
	char *s_taskid;

	task_id_t taskid;
	uintptr_t thread;

	taskid = MERGE_LOUP32(IPC_GET_ARG1(*call), IPC_GET_ARG2(*call));
	thread = IPC_GET_ARG3(*call);

	if (asprintf(&s_taskid, "%lld", taskid) < 0) {
		printf("Memory allocation failed.\n");
		return;
	}

	printf(NAME ": Task %lld fault in thread 0x%lx.\n", taskid, thread);

	argv[0] = fname = "/app/taskdump";

#ifdef CONFIG_VERBOSE_DUMPS
	argv[1] = "-m";
	argv[2] = "-t";
	argv[3] = s_taskid;
	argv[4] = NULL;

	printf(NAME ": Executing %s %s %s %s\n", argv[0], argv[1], argv[2],
	    argv[3]);
#else
	argv[1] = "-t";
	argv[2] = s_taskid;
	argv[3] = NULL;

	printf(NAME ": Executing %s %s %s\n", argv[0], argv[1], argv[2]);
#endif

	if (!task_spawn(fname, argv))
		printf(NAME ": Error spawning taskdump.\n", fname);
}

int main(int argc, char *argv[])
{
	printf(NAME ": Task Monitoring Service\n");

	if (event_subscribe(EVENT_FAULT, 0) != EOK) {
		printf(NAME ": Error registering fault notifications.\n");
		return -1;
	}

	async_set_interrupt_received(fault_event);
	async_manager();

	return 0;
}

/** @}
 */
