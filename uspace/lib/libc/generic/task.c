/*
 * Copyright (c) 2006 Jakub Jermar
 * Copyright (c) 2008 Jiri Svoboda
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

#include <task.h>
#include <ipc/ipc.h>
#include <ipc/loader.h>
#include <libc.h>
#include <string.h>
#include <stdlib.h>
#include <async.h>
#include <errno.h>
#include <vfs/vfs.h>

task_id_t task_get_id(void)
{
	task_id_t task_id;

	(void) __SYSCALL1(SYS_TASK_GET_ID, (sysarg_t) &task_id);

	return task_id;
}

static int task_spawn_loader(void)
{
	int phone_id, rc;

	rc = __SYSCALL1(SYS_PROGRAM_SPAWN_LOADER, (sysarg_t) &phone_id);
	if (rc != 0)
		return rc;

	return phone_id;
}

static int loader_set_args(int phone_id, const char *argv[])
{
	aid_t req;
	ipc_call_t answer;
	ipcarg_t rc;

	const char **ap;
	char *dp;
	char *arg_buf;
	size_t buffer_size;
	size_t len;

	/* 
	 * Serialize the arguments into a single array. First
	 * compute size of the buffer needed.
	 */
	ap = argv;
	buffer_size = 0;
	while (*ap != NULL) {
		buffer_size += strlen(*ap) + 1;
		++ap;
	}

	arg_buf = malloc(buffer_size);
	if (arg_buf == NULL) return ENOMEM;

	/* Now fill the buffer with null-terminated argument strings */
	ap = argv;
	dp = arg_buf;
	while (*ap != NULL) {
		strcpy(dp, *ap);
		dp += strlen(*ap) + 1;

		++ap;
	}

	/* Send serialized arguments to the loader */

	req = async_send_0(phone_id, LOADER_SET_ARGS, &answer);
	rc = ipc_data_write_start(phone_id, (void *)arg_buf, buffer_size);
	if (rc != EOK) {
		async_wait_for(req, NULL);
		return rc;
	}

	async_wait_for(req, &rc);
	if (rc != EOK) return rc;

	/* Free temporary buffer */
	free(arg_buf);

	return EOK;
}

/** Create a new task by running an executable from VFS.
 *
 * @param path	pathname of the binary to execute
 * @param argv	command-line arguments
 * @return	ID of the newly created task or zero on error.
 */
task_id_t task_spawn(const char *path, char *const argv[])
{
	int phone_id;
	ipc_call_t answer;
	aid_t req;
	int rc;
	ipcarg_t retval;

	char *pa;
	size_t pa_len;
	task_id_t task_id;

	pa = absolutize(path, &pa_len);
	if (!pa)
		return 0;

	/* Spawn a program loader */	
	phone_id = task_spawn_loader();
	if (phone_id < 0)
		return 0;

	/*
	 * Say hello so that the loader knows the incoming connection's
	 * phone hash.
	 */
	rc = async_req_0_0(phone_id, LOADER_HELLO);
	if (rc != EOK)
		return 0;

	/* Get task ID. */
	req = async_send_0(phone_id, LOADER_GET_TASKID, &answer);
	rc = ipc_data_read_start(phone_id, &task_id, sizeof(task_id));
	if (rc != EOK) {
		async_wait_for(req, NULL);
		goto error;
	}

	async_wait_for(req, &retval);
	if (retval != EOK)
		goto error;

	/* Send program pathname */
	req = async_send_0(phone_id, LOADER_SET_PATHNAME, &answer);
	rc = ipc_data_write_start(phone_id, (void *)pa, pa_len);
	if (rc != EOK) {
		async_wait_for(req, NULL);
		return 1;
	}

	async_wait_for(req, &retval);
	if (retval != EOK)
		goto error;

	/* Send arguments */
	rc = loader_set_args(phone_id, argv);
	if (rc != EOK)
		goto error;

	/* Request loader to start the program */	
	rc = async_req_0_0(phone_id, LOADER_RUN);
	if (rc != EOK)
		goto error;

	/* Success */
	ipc_hangup(phone_id);
	return task_id;

	/* Error exit */
error:
	ipc_hangup(phone_id);
	return 0;
}

/** @}
 */
