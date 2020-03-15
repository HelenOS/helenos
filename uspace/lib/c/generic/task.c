/*
 * Copyright (c) 2006 Jakub Jermar
 * Copyright (c) 2008 Jiri Svoboda
 * Copyright (c) 2014 Martin Sucha
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

#include <async.h>
#include <task.h>
#include <loader/loader.h>
#include <stdarg.h>
#include <str.h>
#include <ipc/ns.h>
#include <macros.h>
#include <assert.h>
#include <async.h>
#include <errno.h>
#include <ns.h>
#include <stdlib.h>
#include <udebug.h>
#include <libc.h>
#include "private/ns.h"
#include <vfs/vfs.h>

task_id_t task_get_id(void)
{
#ifdef __32_BITS__
	task_id_t task_id;
	(void) __SYSCALL1(SYS_TASK_GET_ID, (sysarg_t) &task_id);

	return task_id;
#endif  /* __32_BITS__ */

#ifdef __64_BITS__
	return (task_id_t) __SYSCALL0(SYS_TASK_GET_ID);
#endif  /* __64_BITS__ */
}

/** Set the task name.
 *
 * @param name The new name, typically the command used to execute the
 *             program.
 *
 * @return Zero on success or an error code.
 */
errno_t task_set_name(const char *name)
{
	assert(name);

	return (errno_t) __SYSCALL2(SYS_TASK_SET_NAME, (sysarg_t) name, str_size(name));
}

/** Kill a task.
 *
 * @param task_id ID of task to kill.
 *
 * @return Zero on success or an error code.
 */

errno_t task_kill(task_id_t task_id)
{
	return (errno_t) __SYSCALL1(SYS_TASK_KILL, (sysarg_t) &task_id);
}

/** Create a new task by running an executable from the filesystem.
 *
 * This is really just a convenience wrapper over the more complicated
 * loader API. Arguments are passed as a null-terminated array of strings.
 * A debug session is created optionally.
 *
 * @param id   If not NULL, the ID of the task is stored here on success.
 * @param wait If not NULL, setup waiting for task's return value and store
 *             the information necessary for waiting here on success.
 * @param path Pathname of the binary to execute.
 * @param argv Command-line arguments.
 * @param rsess   Place to store pointer to debug session or @c NULL
 *                not to start a debug session
 *
 * @return Zero on success or an error code.
 *
 */
errno_t task_spawnv_debug(task_id_t *id, task_wait_t *wait, const char *path,
    const char *const args[], async_sess_t **rsess)
{
	/* Send default files */

	int fd_stdin = -1;
	int fd_stdout = -1;
	int fd_stderr = -1;

	if (stdin != NULL) {
		(void) vfs_fhandle(stdin, &fd_stdin);
	}

	if (stdout != NULL) {
		(void) vfs_fhandle(stdout, &fd_stdout);
	}

	if (stderr != NULL) {
		(void) vfs_fhandle(stderr, &fd_stderr);
	}

	return task_spawnvf_debug(id, wait, path, args, fd_stdin, fd_stdout,
	    fd_stderr, rsess);
}

/** Create a new task by running an executable from the filesystem.
 *
 * This is really just a convenience wrapper over the more complicated
 * loader API. Arguments are passed as a null-terminated array of strings.
 *
 * @param id   If not NULL, the ID of the task is stored here on success.
 * @param wait If not NULL, setup waiting for task's return value and store
 *             the information necessary for waiting here on success.
 * @param path Pathname of the binary to execute.
 * @param argv Command-line arguments.
 *
 * @return Zero on success or an error code.
 *
 */
errno_t task_spawnv(task_id_t *id, task_wait_t *wait, const char *path,
    const char *const args[])
{
	return task_spawnv_debug(id, wait, path, args, NULL);
}

/** Create a new task by loading an executable from the filesystem.
 *
 * This is really just a convenience wrapper over the more complicated
 * loader API. Arguments are passed as a null-terminated array of strings.
 * Files are passed as null-terminated array of pointers to fdi_node_t.
 * A debug session is created optionally.
 *
 * @param id      If not NULL, the ID of the task is stored here on success.
 * @param wait    If not NULL, setup waiting for task's return value and store.
 * @param path    Pathname of the binary to execute.
 * @param argv    Command-line arguments
 * @param std_in  File to use as stdin
 * @param std_out File to use as stdout
 * @param std_err File to use as stderr
 * @param rsess   Place to store pointer to debug session or @c NULL
 *                not to start a debug session
 *
 * @return Zero on success or an error code
 *
 */
errno_t task_spawnvf_debug(task_id_t *id, task_wait_t *wait,
    const char *path, const char *const args[], int fd_stdin, int fd_stdout,
    int fd_stderr, async_sess_t **rsess)
{
	async_sess_t *ksess = NULL;

	/* Connect to a program loader. */
	errno_t rc;
	loader_t *ldr = loader_connect(&rc);
	if (ldr == NULL)
		return rc;

	bool wait_initialized = false;

	/* Get task ID. */
	task_id_t task_id;
	rc = loader_get_task_id(ldr, &task_id);
	if (rc != EOK)
		goto error;

	/* Send spawner's current working directory. */
	rc = loader_set_cwd(ldr);
	if (rc != EOK)
		goto error;

	/* Send program binary. */
	rc = loader_set_program_path(ldr, path);
	if (rc != EOK)
		goto error;

	/* Send arguments. */
	rc = loader_set_args(ldr, args);
	if (rc != EOK)
		goto error;

	/* Send files */
	int root = vfs_root();
	if (root >= 0) {
		rc = loader_add_inbox(ldr, "root", root);
		vfs_put(root);
		if (rc != EOK)
			goto error;
	}

	if (fd_stdin >= 0) {
		rc = loader_add_inbox(ldr, "stdin", fd_stdin);
		if (rc != EOK)
			goto error;
	}

	if (fd_stdout >= 0) {
		rc = loader_add_inbox(ldr, "stdout", fd_stdout);
		if (rc != EOK)
			goto error;
	}

	if (fd_stderr >= 0) {
		rc = loader_add_inbox(ldr, "stderr", fd_stderr);
		if (rc != EOK)
			goto error;
	}

	/* Load the program. */
	rc = loader_load_program(ldr);
	if (rc != EOK)
		goto error;

	/* Setup waiting for return value if needed */
	if (wait) {
		rc = task_setup_wait(task_id, wait);
		if (rc != EOK)
			goto error;
		wait_initialized = true;
	}

	/* Start a debug session if requested */
	if (rsess != NULL) {
		ksess = async_connect_kbox(task_id, &rc);
		if (ksess == NULL) {
			/* Most likely debugging support is not compiled in */
			goto error;
		}

		rc = udebug_begin(ksess);
		if (rc != EOK)
			goto error;

		/*
		 * Run it, not waiting for response. It would never come
		 * as the loader is stopped.
		 */
		loader_run_nowait(ldr);
	} else {
		/* Run it. */
		rc = loader_run(ldr);
		if (rc != EOK)
			goto error;
	}

	/* Success */
	if (id != NULL)
		*id = task_id;
	if (rsess != NULL)
		*rsess = ksess;
	return EOK;

error:
	if (ksess != NULL)
		async_hangup(ksess);
	if (wait_initialized)
		task_cancel_wait(wait);

	/* Error exit */
	loader_abort(ldr);
	return rc;
}

/** Create a new task by running an executable from the filesystem.
 *
 * Arguments are passed as a null-terminated array of strings.
 * Files are passed as null-terminated array of pointers to fdi_node_t.
 *
 * @param id      If not NULL, the ID of the task is stored here on success.
 * @param wait    If not NULL, setup waiting for task's return value and store.
 * @param path    Pathname of the binary to execute
 * @param argv    Command-line arguments
 * @param std_in  File to use as stdin
 * @param std_out File to use as stdout
 * @param std_err File to use as stderr
 *
 * @return Zero on success or an error code.
 *
 */
errno_t task_spawnvf(task_id_t *id, task_wait_t *wait, const char *path,
    const char *const args[], int fd_stdin, int fd_stdout, int fd_stderr)
{
	return task_spawnvf_debug(id, wait, path, args, fd_stdin, fd_stdout,
	    fd_stderr, NULL);
}

/** Create a new task by running an executable from the filesystem.
 *
 * This is really just a convenience wrapper over the more complicated
 * loader API. Arguments are passed in a va_list.
 *
 * @param id   If not NULL, the ID of the task is stored here on success.
 * @param wait If not NULL, setup waiting for task's return value and store
 *             the information necessary for waiting here on success.
 * @param path Pathname of the binary to execute.
 * @param cnt  Number of arguments.
 * @param ap   Command-line arguments.
 *
 * @return Zero on success or an error code.
 *
 */
errno_t task_spawn(task_id_t *task_id, task_wait_t *wait, const char *path,
    int cnt, va_list ap)
{
	/* Allocate argument list. */
	const char **arglist = malloc(cnt * sizeof(const char *));
	if (arglist == NULL)
		return ENOMEM;

	/* Fill in arguments. */
	const char *arg;
	cnt = 0;
	do {
		arg = va_arg(ap, const char *);
		arglist[cnt++] = arg;
	} while (arg != NULL);

	/* Spawn task. */
	errno_t rc = task_spawnv(task_id, wait, path, arglist);

	/* Free argument list. */
	free(arglist);
	return rc;
}

/** Create a new task by running an executable from the filesystem.
 *
 * This is really just a convenience wrapper over the more complicated
 * loader API. Arguments are passed as a null-terminated list of arguments.
 *
 * @param id   If not NULL, the ID of the task is stored here on success.
 * @param wait If not NULL, setup waiting for task's return value and store
 *             the information necessary for waiting here on success.
 * @param path Pathname of the binary to execute.
 * @param ...  Command-line arguments.
 *
 * @return Zero on success or an error code.
 *
 */
errno_t task_spawnl(task_id_t *task_id, task_wait_t *wait, const char *path, ...)
{
	/* Count the number of arguments. */

	va_list ap;
	const char *arg;
	int cnt = 0;

	va_start(ap, path);
	do {
		arg = va_arg(ap, const char *);
		cnt++;
	} while (arg != NULL);
	va_end(ap);

	va_start(ap, path);
	errno_t rc = task_spawn(task_id, wait, path, cnt, ap);
	va_end(ap);

	return rc;
}

/** Setup waiting for a task.
 *
 * If the task finishes after this call succeeds, it is guaranteed that
 * task_wait(wait, &texit, &retval) will return correct return value for
 * the task.
 *
 * @param id   ID of the task to setup waiting for.
 * @param wait Information necessary for the later task_wait call is stored here.
 *
 * @return EOK on success, else error code.
 */
errno_t task_setup_wait(task_id_t id, task_wait_t *wait)
{
	errno_t rc;
	async_sess_t *sess_ns = ns_session_get(&rc);
	if (sess_ns == NULL)
		return rc;

	async_exch_t *exch = async_exchange_begin(sess_ns);
	wait->aid = async_send_2(exch, NS_TASK_WAIT, LOWER32(id), UPPER32(id),
	    &wait->result);
	async_exchange_end(exch);

	return EOK;
}

/** Cancel waiting for a task.
 *
 * This can be called *instead of* task_wait if the caller is not interested
 * in waiting for the task anymore.
 *
 * This function cannot be called if the task_wait was already called.
 *
 * @param wait task_wait_t previously initialized by task_setup_wait.
 */
void task_cancel_wait(task_wait_t *wait)
{
	async_forget(wait->aid);
}

/** Wait for a task to finish.
 *
 * This function returns correct values even if the task finished in
 * between task_setup_wait and this task_wait call.
 *
 * This function cannot be called more than once with the same task_wait_t
 * (it can be reused, but must be reinitialized with task_setup_wait first)
 *
 * @param wait   task_wait_t previously initialized by task_setup_wait.
 * @param texit  Store type of task exit here.
 * @param retval Store return value of the task here.
 *
 * @return EOK on success, else error code.
 */
errno_t task_wait(task_wait_t *wait, task_exit_t *texit, int *retval)
{
	assert(texit);
	assert(retval);

	errno_t rc;
	async_wait_for(wait->aid, &rc);

	if (rc == EOK) {
		*texit = ipc_get_arg1(&wait->result);
		*retval = ipc_get_arg2(&wait->result);
	}

	return rc;
}

/** Wait for a task to finish by its id.
 *
 * Note that this will fail with ENOENT if the task id is not registered in ns
 * (e.g. if the task finished). If you are spawning a task and need to wait
 * for its completion, use wait parameter of the task_spawn* functions instead
 * to prevent a race where the task exits before you may have a chance to wait
 * wait for it.
 *
 * @param id ID of the task to wait for.
 * @param texit  Store type of task exit here.
 * @param retval Store return value of the task here.
 *
 * @return EOK on success, else error code.
 */
errno_t task_wait_task_id(task_id_t id, task_exit_t *texit, int *retval)
{
	task_wait_t wait;
	errno_t rc = task_setup_wait(id, &wait);
	if (rc != EOK)
		return rc;

	return task_wait(&wait, texit, retval);
}

errno_t task_retval(int val)
{
	errno_t rc;
	async_sess_t *sess_ns = ns_session_get(&rc);
	if (sess_ns == NULL)
		return rc;

	async_exch_t *exch = async_exchange_begin(sess_ns);
	rc = (errno_t) async_req_1_0(exch, NS_RETVAL, val);
	async_exchange_end(exch);

	return rc;
}

/** @}
 */
