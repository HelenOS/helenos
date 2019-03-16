/*
 * Copyright (c) 2008 Tim Post
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

/*
 * The VERY basics of execute in place support. These are buggy, leaky
 * and not nearly done. Only here for beta testing!! You were warned!!
 * TODO:
 * Hash command lookups to save time
 * Create a running pointer to **path and advance/rewind it as we go
 */

#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <str_error.h>
#include <errno.h>
#include <vfs/vfs.h>

#include "config.h"
#include "util.h"
#include "exec.h"
#include "errors.h"

static errno_t find_command(char *, char **);
static int try_access(const char *);

const char *search_dir[] = { "/app", NULL };

/* work-around for access() */
static int try_access(const char *f)
{
	int fd;

	errno_t rc = vfs_lookup_open(f, WALK_REGULAR, MODE_READ, &fd);
	if (rc == EOK) {
		vfs_put(fd);
		return 0;
	} else
		return -1;
}

/** Returns EOK if no internal failure or else ENOMEM
 *
 * When the parameter `cmd` can be found then the absolute path will be set in `found`.
 * Or else `found` will be NULL.
 * `found` will be newly allocated and must be freed by the caller
 */
static errno_t find_command(char *cmd, char **found)
{
	/* The user has specified a full or relative path, just give it back. */
	if (is_path(cmd)) {
		if (-1 != try_access(cmd)) {
			*found = str_dup(cmd);
			return EOK;
		}

		*found = NULL;
		return EOK;
	}

	*found = (char *)malloc(PATH_MAX);
	if (*found == NULL) {
		return ENOMEM;
	}

	/* We now have n places to look for the command */
	size_t i;
	size_t cmd_length = str_length(cmd);
	for (i = 0; search_dir[i] != NULL; i++) {
		if (str_length(search_dir[i]) + cmd_length + 2 > PATH_MAX) {
			free(*found);
			return ENOMEM;
		}

		memset(*found, 0, PATH_MAX);
		snprintf(*found, PATH_MAX, "%s/%s", search_dir[i], cmd);
		if (-1 != try_access(*found)) {
			return EOK;
		}
	}
	free(*found);
	*found = NULL;

	/* We didn't find it, return NULL */
	return EOK;
}

unsigned int try_exec(char *cmd, char **argv, iostate_t *io)
{
	task_id_t tid;
	task_wait_t twait;
	task_exit_t texit;
	char *tmp;
	errno_t rc;
	int retval, i;
	int file_handles[3] = { -1, -1, -1 };
	FILE *files[3];

	rc = find_command(cmd, &tmp);
	if (rc != EOK) {
		cli_error(CL_ENOMEM, "%s: failure executing find_command()", progname);
		return 1;
	}

	if (tmp == NULL) {
		cli_error(CL_EEXEC, "%s: Command not found '%s'", progname, cmd);
		return 1;
	}

	files[0] = io->stdin;
	files[1] = io->stdout;
	files[2] = io->stderr;

	for (i = 0; i < 3 && files[i] != NULL; i++) {
		vfs_fhandle(files[i], &file_handles[i]);
	}

	rc = task_spawnvf(&tid, &twait, tmp, (const char **) argv,
	    file_handles[0], file_handles[1], file_handles[2]);
	free(tmp);

	if (rc != EOK) {
		cli_error(CL_EEXEC, "%s: Cannot spawn `%s' (%s)", progname, cmd,
		    str_error(rc));
		return 1;
	}

	rc = task_wait(&twait, &texit, &retval);
	if (rc != EOK) {
		printf("%s: Failed waiting for command (%s)\n", progname,
		    str_error(rc));
		return 1;
	} else if (texit != TASK_EXIT_NORMAL) {
		printf("%s: Command failed (unexpectedly terminated)\n", progname);
		return 1;
	} else if (retval != 0) {
		printf("%s: Command failed (exit code %d)\n",
		    progname, retval);
		return 1;
	}

	return 0;
}
