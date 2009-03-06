/*
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

#include <ipc/ipc.h>
#include <ipc/loader.h>
#include <ipc/services.h>
#include <libc.h>
#include <task.h>
#include <string.h>
#include <stdlib.h>
#include <async.h>
#include <errno.h>
#include <vfs/vfs.h>
#include <loader/loader.h>

/** Connect to a new program loader.
 *
 * Spawns a new program loader task and returns the connection structure.
 * @param name	Symbolic name to set on the newly created task.
 * @return	Pointer to the loader connection structure (should be
 *		de-allocated using free() after use).
 */
int loader_spawn(const char *name)
{
	return __SYSCALL2(SYS_PROGRAM_SPAWN_LOADER,
	    (sysarg_t) name, strlen(name));
}

loader_t *loader_connect(void)
{
	loader_t *ldr;
	int phone_id;

	phone_id = ipc_connect_me_to_blocking(PHONE_NS, SERVICE_LOAD, 0, 0);
	if (phone_id < 0)
		return NULL;

	ldr = malloc(sizeof(loader_t));
	if (ldr == NULL)
		return NULL;

	ldr->phone_id = phone_id;
	return ldr;	
}

/** Get ID of the new task.
 *
 * Retrieves the ID of the new task from the loader.
 *
 * @param ldr		Loader connection structure.
 * @param task_id	Points to a variable where the ID should be stored.
 * @return		Zero on success or negative error code.
 */
int loader_get_task_id(loader_t *ldr, task_id_t *task_id)
{
	ipc_call_t answer;
	aid_t req;
	int rc;
	ipcarg_t retval;

	/* Get task ID. */
	req = async_send_0(ldr->phone_id, LOADER_GET_TASKID, &answer);
	rc = ipc_data_read_start(ldr->phone_id, task_id, sizeof(task_id_t));
	if (rc != EOK) {
		async_wait_for(req, NULL);
		return rc;
	}

	async_wait_for(req, &retval);
	return (int)retval;
}

/** Set pathname of the program to load.
 *
 * Sets the name of the program file to load. The name can be relative
 * to the current working directory (it will be absolutized before
 * sending to the loader).
 *
 * @param ldr		Loader connection structure.
 * @param path		Pathname of the program file.
 * @return		Zero on success or negative error code.
 */
int loader_set_pathname(loader_t *ldr, const char *path)
{
	ipc_call_t answer;
	aid_t req;
	int rc;
	ipcarg_t retval;

	char *pa;
	size_t pa_len;

	pa = absolutize(path, &pa_len);
	if (!pa)
		return 0;

	/* Send program pathname */
	req = async_send_0(ldr->phone_id, LOADER_SET_PATHNAME, &answer);
	rc = ipc_data_write_start(ldr->phone_id, (void *)pa, pa_len);
	if (rc != EOK) {
		async_wait_for(req, NULL);
		return rc;
	}

	free(pa);

	async_wait_for(req, &retval);
	return (int)retval;
}


/** Set command-line arguments for the program.
 *
 * Sets the vector of command-line arguments to be passed to the loaded
 * program. By convention, the very first argument is typically the same as
 * the command used to execute the program.
 *
 * @param ldr		Loader connection structure.
 * @param argv		NULL-terminated array of pointers to arguments.
 * @return		Zero on success or negative error code.
 */
int loader_set_args(loader_t *ldr, char *const argv[])
{
	aid_t req;
	ipc_call_t answer;
	ipcarg_t rc;

	char *const *ap;
	char *dp;
	char *arg_buf;
	size_t buffer_size;

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

	req = async_send_0(ldr->phone_id, LOADER_SET_ARGS, &answer);
	rc = ipc_data_write_start(ldr->phone_id, (void *)arg_buf, buffer_size);
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

/** Instruct loader to load the program.
 *
 * If this function succeeds, the program has been successfully loaded
 * and is ready to be executed.
 *
 * @param ldr		Loader connection structure.
 * @return		Zero on success or negative error code.
 */
int loader_load_program(loader_t *ldr)
{
	int rc;

	rc = async_req_0_0(ldr->phone_id, LOADER_LOAD);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Instruct loader to execute the program.
 *
 * Note that this function blocks until the loader actually replies
 * so you cannot expect this function to return if you are debugging
 * the task and its thread is stopped.
 *
 * After using this function, no further operations must be performed
 * on the loader structure. It should be de-allocated using free().
 *
 * @param ldr		Loader connection structure.
 * @return		Zero on success or negative error code.
 */
int loader_run(loader_t *ldr)
{
	int rc;

	rc = async_req_0_0(ldr->phone_id, LOADER_RUN);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Cancel the loader session.
 *
 * Tells the loader not to load any program and terminate.
 * After using this function, no further operations must be performed
 * on the loader structure. It should be de-allocated using free().
 *
 * @param ldr		Loader connection structure.
 * @return		Zero on success or negative error code.
 */
void loader_abort(loader_t *ldr)
{
	ipc_hangup(ldr->phone_id);
	ldr->phone_id = 0;
}

/** @}
 */
