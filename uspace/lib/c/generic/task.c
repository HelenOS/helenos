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
#include <loader/loader.h>
#include <stdarg.h>
#include <str.h>
#include <ipc/ns.h>
#include <macros.h>
#include <assert.h>
#include <async.h>
#include <errno.h>
#include <malloc.h>
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
 * @return Zero on success or negative error code.
 */
int task_set_name(const char *name)
{
	assert(name);
	
	return __SYSCALL2(SYS_TASK_SET_NAME, (sysarg_t) name, str_size(name));
}

/** Kill a task.
 *
 * @param task_id ID of task to kill.
 *
 * @return Zero on success or negative error code.
 */

int task_kill(task_id_t task_id)
{
	return (int) __SYSCALL1(SYS_TASK_KILL, (sysarg_t) &task_id);
}

/** Create a new task by running an executable from the filesystem.
 *
 * This is really just a convenience wrapper over the more complicated
 * loader API. Arguments are passed as a null-terminated array of strings.
 *
 * @param id   If not NULL, the ID of the task is stored here on success.
 * @param path Pathname of the binary to execute.
 * @param argv Command-line arguments.
 *
 * @return Zero on success or negative error code.
 *
 */
int task_spawnv(task_id_t *id, const char *path, const char *const args[])
{
	/* Send default files */
	int *files[4];
	int fd_stdin;
	int fd_stdout;
	int fd_stderr;
	
	if ((stdin != NULL) && (fhandle(stdin, &fd_stdin) == EOK))
		files[0] = &fd_stdin;
	else
		files[0] = NULL;
	
	if ((stdout != NULL) && (fhandle(stdout, &fd_stdout) == EOK))
		files[1] = &fd_stdout;
	else
		files[1] = NULL;
	
	if ((stderr != NULL) && (fhandle(stderr, &fd_stderr) == EOK))
		files[2] = &fd_stderr;
	else
		files[2] = NULL;
	
	files[3] = NULL;
	
	return task_spawnvf(id, path, args, files);
}

/** Create a new task by running an executable from the filesystem.
 *
 * This is really just a convenience wrapper over the more complicated
 * loader API. Arguments are passed as a null-terminated array of strings.
 * Files are passed as null-terminated array of pointers to fdi_node_t.
 *
 * @param id    If not NULL, the ID of the task is stored here on success.
 * @param path  Pathname of the binary to execute.
 * @param argv  Command-line arguments.
 * @param files Standard files to use.
 *
 * @return Zero on success or negative error code.
 *
 */
int task_spawnvf(task_id_t *id, const char *path, const char *const args[],
    int *const files[])
{
	/* Connect to a program loader. */
	loader_t *ldr = loader_connect();
	if (ldr == NULL)
		return EREFUSED;
	
	/* Get task ID. */
	task_id_t task_id;
	int rc = loader_get_task_id(ldr, &task_id);
	if (rc != EOK)
		goto error;
	
	/* Send spawner's current working directory. */
	rc = loader_set_cwd(ldr);
	if (rc != EOK)
		goto error;
	
	/* Send program pathname. */
	rc = loader_set_pathname(ldr, path);
	if (rc != EOK)
		goto error;
	
	/* Send arguments. */
	rc = loader_set_args(ldr, args);
	if (rc != EOK)
		goto error;
	
	/* Send files */
	rc = loader_set_files(ldr, files);
	if (rc != EOK)
		goto error;
	
	/* Load the program. */
	rc = loader_load_program(ldr);
	if (rc != EOK)
		goto error;
	
	/* Run it. */
	rc = loader_run(ldr);
	if (rc != EOK)
		goto error;
	
	/* Success */
	if (id != NULL)
		*id = task_id;
	
	return EOK;
	
error:
	/* Error exit */
	loader_abort(ldr);
	return rc;
}

/** Create a new task by running an executable from the filesystem.
 *
 * This is really just a convenience wrapper over the more complicated
 * loader API. Arguments are passed as a null-terminated list of arguments.
 *
 * @param id   If not NULL, the ID of the task is stored here on success.
 * @param path Pathname of the binary to execute.
 * @param ...  Command-line arguments.
 *
 * @return Zero on success or negative error code.
 *
 */
int task_spawnl(task_id_t *task_id, const char *path, ...)
{
	/* Count the number of arguments. */
	
	va_list ap;
	const char *arg;
	const char **arglist;
	int cnt = 0;
	
	va_start(ap, path);
	do {
		arg = va_arg(ap, const char *);
		cnt++;
	} while (arg != NULL);
	va_end(ap);
	
	/* Allocate argument list. */
	arglist = malloc(cnt * sizeof(const char *));
	if (arglist == NULL)
		return ENOMEM;
	
	/* Fill in arguments. */
	cnt = 0;
	va_start(ap, path);
	do {
		arg = va_arg(ap, const char *);
		arglist[cnt++] = arg;
	} while (arg != NULL);
	va_end(ap);
	
	/* Spawn task. */
	int rc = task_spawnv(task_id, path, arglist);
	
	/* Free argument list. */
	free(arglist);
	return rc;
}

int task_wait(task_id_t id, task_exit_t *texit, int *retval)
{
	assert(texit);
	assert(retval);
	
	async_exch_t *exch = async_exchange_begin(session_ns);
	sysarg_t te, rv;
	int rc = (int) async_req_2_2(exch, NS_TASK_WAIT, LOWER32(id),
	    UPPER32(id), &te, &rv);
	async_exchange_end(exch);
	
	*texit = te;
	*retval = rv;
	
	return rc;
}

int task_retval(int val)
{
	async_exch_t *exch = async_exchange_begin(session_ns);
	int rc = (int) async_req_1_0(exch, NS_RETVAL, val);
	async_exchange_end(exch);
	
	return rc;
}

/** @}
 */
