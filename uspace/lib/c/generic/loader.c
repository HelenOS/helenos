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

#include <ipc/loader.h>
#include <ipc/services.h>
#include <ns.h>
#include <libc.h>
#include <task.h>
#include <str.h>
#include <stdlib.h>
#include <async.h>
#include <errno.h>
#include <vfs/vfs.h>
#include <loader/loader.h>
#include "private/loader.h"

/** Connect to a new program loader.
 *
 * Spawns a new program loader task and returns the connection structure.
 *
 * @param name Symbolic name to set on the newly created task.
 *
 * @return Error code.
 */
errno_t loader_spawn(const char *name)
{
	return (errno_t) __SYSCALL2(SYS_PROGRAM_SPAWN_LOADER,
	    (sysarg_t) name, str_size(name));
}

loader_t *loader_connect(void)
{
	loader_t *ldr = malloc(sizeof(loader_t));
	if (ldr == NULL)
		return NULL;

	async_sess_t *sess =
	    service_connect_blocking(SERVICE_LOADER, INTERFACE_LOADER, 0);
	if (sess == NULL) {
		free(ldr);
		return NULL;
	}

	ldr->sess = sess;
	return ldr;
}

/** Get ID of the new task.
 *
 * Retrieves the ID of the new task from the loader.
 *
 * @param ldr     Loader connection structure.
 * @param task_id Points to a variable where the ID should be stored.
 *
 * @return Zero on success or an error code.
 *
 */
errno_t loader_get_task_id(loader_t *ldr, task_id_t *task_id)
{
	/* Get task ID. */
	async_exch_t *exch = async_exchange_begin(ldr->sess);

	ipc_call_t answer;
	aid_t req = async_send_0(exch, LOADER_GET_TASKID, &answer);
	errno_t rc = async_data_read_start(exch, task_id, sizeof(task_id_t));

	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return (errno_t) rc;
	}

	async_wait_for(req, &rc);
	return (errno_t) rc;
}

/** Set current working directory for the loaded task.
 *
 * Sets the current working directory for the loaded task.
 *
 * @param ldr  Loader connection structure.
 *
 * @return Zero on success or an error code.
 *
 */
errno_t loader_set_cwd(loader_t *ldr)
{
	char *cwd = (char *) malloc(MAX_PATH_LEN + 1);
	if (!cwd)
		return ENOMEM;

	if (vfs_cwd_get(cwd, MAX_PATH_LEN + 1) != EOK)
		str_cpy(cwd, MAX_PATH_LEN + 1, "/");

	size_t len = str_length(cwd);

	async_exch_t *exch = async_exchange_begin(ldr->sess);

	ipc_call_t answer;
	aid_t req = async_send_0(exch, LOADER_SET_CWD, &answer);
	errno_t rc = async_data_write_start(exch, cwd, len);

	async_exchange_end(exch);
	free(cwd);

	if (rc != EOK) {
		async_forget(req);
		return (errno_t) rc;
	}

	async_wait_for(req, &rc);
	return (errno_t) rc;
}

/** Set the program to load.
 *
 * @param ldr  Loader connection structure.
 * @param name Name to set for the spawned program.
 * @param file Program file.
 *
 * @return Zero on success or an error code.
 *
 */
errno_t loader_set_program(loader_t *ldr, const char *name, int file)
{
	async_exch_t *exch = async_exchange_begin(ldr->sess);

	ipc_call_t answer;
	aid_t req = async_send_0(exch, LOADER_SET_PROGRAM, &answer);

	errno_t rc = async_data_write_start(exch, name, str_size(name) + 1);
	if (rc == EOK) {
		async_exch_t *vfs_exch = vfs_exchange_begin();
		rc = vfs_pass_handle(vfs_exch, file, exch);
		vfs_exchange_end(vfs_exch);
	}

	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return (errno_t) rc;
	}

	async_wait_for(req, &rc);
	return (errno_t) rc;
}

/** Set the program to load by path.
 *
 * @param ldr  Loader connection structure.
 * @param path Program path.
 *
 * @return Zero on success or an error code.
 *
 */
errno_t loader_set_program_path(loader_t *ldr, const char *path)
{
	const char *name = str_rchr(path, '/');
	if (name == NULL) {
		name = path;
	} else {
		name++;
	}

	int fd;
	errno_t rc = vfs_lookup(path, 0, &fd);
	if (rc != EOK) {
		return rc;
	}

	rc = loader_set_program(ldr, name, fd);
	vfs_put(fd);
	return rc;
}


/** Set command-line arguments for the program.
 *
 * Sets the vector of command-line arguments to be passed to the loaded
 * program. By convention, the very first argument is typically the same as
 * the command used to execute the program.
 *
 * @param ldr  Loader connection structure.
 * @param argv NULL-terminated array of pointers to arguments.
 *
 * @return Zero on success or an error code.
 *
 */
errno_t loader_set_args(loader_t *ldr, const char *const argv[])
{
	/*
	 * Serialize the arguments into a single array. First
	 * compute size of the buffer needed.
	 */
	const char *const *ap = argv;
	size_t buffer_size = 0;
	while (*ap != NULL) {
		buffer_size += str_size(*ap) + 1;
		ap++;
	}

	char *arg_buf = malloc(buffer_size);
	if (arg_buf == NULL)
		return ENOMEM;

	/* Now fill the buffer with null-terminated argument strings */
	ap = argv;
	char *dp = arg_buf;

	while (*ap != NULL) {
		str_cpy(dp, buffer_size - (dp - arg_buf), *ap);
		dp += str_size(*ap) + 1;
		ap++;
	}

	/* Send serialized arguments to the loader */
	async_exch_t *exch = async_exchange_begin(ldr->sess);

	ipc_call_t answer;
	aid_t req = async_send_0(exch, LOADER_SET_ARGS, &answer);
	errno_t rc = async_data_write_start(exch, (void *) arg_buf,
	    buffer_size);

	async_exchange_end(exch);
	free(arg_buf);

	if (rc != EOK) {
		async_forget(req);
		return (errno_t) rc;
	}

	async_wait_for(req, &rc);
	return (errno_t) rc;
}

/** Add a file to the task's inbox.
 *
 * @param ldr        Loader connection structure.
 * @param name       Identification of the file.
 * @param file       The file's descriptor.
 *
 * @return Zero on success or an error code.
 *
 */
errno_t loader_add_inbox(loader_t *ldr, const char *name, int file)
{
	async_exch_t *exch = async_exchange_begin(ldr->sess);
	async_exch_t *vfs_exch = vfs_exchange_begin();

	aid_t req = async_send_0(exch, LOADER_ADD_INBOX, NULL);

	errno_t rc = async_data_write_start(exch, name, str_size(name) + 1);
	if (rc == EOK) {
		rc = vfs_pass_handle(vfs_exch, file, exch);
	}

	async_exchange_end(vfs_exch);
	async_exchange_end(exch);

	if (rc == EOK) {
		async_wait_for(req, &rc);
	} else {
		async_forget(req);
	}

	return (errno_t) rc;
}

/** Instruct loader to load the program.
 *
 * If this function succeeds, the program has been successfully loaded
 * and is ready to be executed.
 *
 * @param ldr Loader connection structure.
 *
 * @return Zero on success or an error code.
 *
 */
errno_t loader_load_program(loader_t *ldr)
{
	async_exch_t *exch = async_exchange_begin(ldr->sess);
	errno_t rc = async_req_0_0(exch, LOADER_LOAD);
	async_exchange_end(exch);

	return rc;
}

/** Instruct loader to execute the program.
 *
 * Note that this function blocks until the loader actually replies
 * so you cannot expect this function to return if you are debugging
 * the task and its thread is stopped.
 *
 * After using this function, no further operations can be performed
 * on the loader structure and it is deallocated.
 *
 * @param ldr Loader connection structure.
 *
 * @return Zero on success or an error code.
 *
 */
errno_t loader_run(loader_t *ldr)
{
	async_exch_t *exch = async_exchange_begin(ldr->sess);
	errno_t rc = async_req_0_0(exch, LOADER_RUN);
	async_exchange_end(exch);

	if (rc != EOK)
		return rc;

	async_hangup(ldr->sess);
	free(ldr);

	return EOK;
}

/** Cancel the loader session.
 *
 * Tell the loader not to load any program and terminate.
 * After using this function, no further operations can be performed
 * on the loader structure and it is deallocated.
 *
 * @param ldr Loader connection structure.
 *
 * @return Zero on success or an error code.
 *
 */
void loader_abort(loader_t *ldr)
{
	async_hangup(ldr->sess);
	free(ldr);
}

/** @}
 */
