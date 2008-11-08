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
#include <libc.h>
#include <stdlib.h>
#include <errno.h>
#include <loader/loader.h>

task_id_t task_get_id(void)
{
	task_id_t task_id;

	(void) __SYSCALL1(SYS_TASK_GET_ID, (sysarg_t) &task_id);

	return task_id;
}

/** Create a new task by running an executable from the filesystem.
 *
 * This is really just a convenience wrapper over the more complicated
 * loader API.
 *
 * @param path	pathname of the binary to execute
 * @param argv	command-line arguments
 * @return	ID of the newly created task or zero on error.
 */
task_id_t task_spawn(const char *path, char *const argv[])
{
	loader_t *ldr;
	task_id_t task_id;
	int rc;

	/* Spawn a program loader. */	
	ldr = loader_spawn(path);
	if (ldr == NULL)
		return 0;

	/* Get task ID. */
	rc = loader_get_task_id(ldr, &task_id);
	if (rc != EOK)
		goto error;

	/* Send program pathname. */
	rc = loader_set_pathname(ldr, path);
	if (rc != EOK)
		goto error;

	/* Send arguments. */
	rc = loader_set_args(ldr, argv);
	if (rc != EOK)
		goto error;

	/* Load the program. */
	rc = loader_load_program(ldr);
	if (rc != EOK)
		goto error;

	/* Run it. */
	/* Load the program. */
	rc = loader_run(ldr);
	if (rc != EOK)
		goto error;

	/* Success */

	free(ldr);
	return task_id;

	/* Error exit */
error:
	loader_abort(ldr);
	free(ldr);

	return 0;
}

/** @}
 */
