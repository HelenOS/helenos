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
#include <async.h>
#include <ipc/services.h>
#include <sys/typefmt.h>
#include <task.h>
#include <event.h>
#include <macros.h>
#include <errno.h>
#include <str_error.h>

#define NAME  "taskmon"

static void fault_event(ipc_callid_t callid, ipc_call_t *call)
{
	const char *fname;
	char *s_taskid;
	int rc;

	task_id_t taskid;
	uintptr_t thread;

	taskid = MERGE_LOUP32(IPC_GET_ARG1(*call), IPC_GET_ARG2(*call));
	thread = IPC_GET_ARG3(*call);

	if (asprintf(&s_taskid, "%" PRIu64, taskid) < 0) {
		printf("Memory allocation failed.\n");
		return;
	}

	printf(NAME ": Task %" PRIu64 " fault in thread %p.\n", taskid,
	    (void *) thread);

	fname = "/app/taskdump";

#ifdef CONFIG_WRITE_CORE_FILES
	char *dump_fname;

	if (asprintf(&dump_fname, "/data/core%" PRIu64, taskid) < 0) {
		printf("Memory allocation failed.\n");
		return;
	}

	printf(NAME ": Executing %s -c %s -t %s\n", fname, dump_fname, s_taskid);
	rc = task_spawnl(NULL, fname, fname, "-c", dump_fname, "-t", s_taskid,
	    NULL);
#else
	printf(NAME ": Executing %s -t %s\n", fname, s_taskid);
	rc = task_spawnl(NULL, fname, fname, "-t", s_taskid, NULL);
#endif
	if (rc != EOK) {
		printf("%s: Error spawning %s (%s).\n", NAME, fname,
		    str_error(rc));
	}
}

int main(int argc, char *argv[])
{
	printf("%s: Task Monitoring Service\n", NAME);
	
	if (event_subscribe(EVENT_FAULT, 0) != EOK) {
		printf("%s: Error registering fault notifications.\n", NAME);
		return -1;
	}
	
	async_set_interrupt_received(fault_event);
	task_retval(0);
	async_manager();
	
	return 0;
}

/** @}
 */
