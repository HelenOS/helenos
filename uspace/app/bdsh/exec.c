/* Copyright (c) 2008, Tim Post <tinkertim@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of the original program's authors nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* The VERY basics of execute in place support. These are buggy, leaky
 * and not nearly done. Only here for beta testing!! You were warned!!
 * TODO:
 * Hash command lookups to save time
 * Create a running pointer to **path and advance/rewind it as we go */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include "config.h"
#include "util.h"
#include "exec.h"
#include "errors.h"

/* FIXME: Just have find_command() return an allocated string */
static char *found;

static char *find_command(char *);
static int try_access(const char *);

/* work-around for access() */
static int try_access(const char *f)
{
	int fd;

	fd = open(f, O_RDONLY);
	if (fd > -1) {
		close(fd);
		return 0;
	} else
		return -1;
}

/* Returns the full path of "cmd" if cmd is found, else just hand back
 * cmd as it was presented */
static char *find_command(char *cmd)
{
	char *path_tok;
	char *path[PATH_MAX];
	int n = 0, i = 0;
	size_t x = str_size(cmd) + 2;

	found = (char *)malloc(PATH_MAX);

	/* The user has specified a full or relative path, just give it back. */
	if (-1 != try_access(cmd)) {
		return (char *) cmd;
	}

	path_tok = strdup(PATH);

	/* Extract the PATH env to a path[] array */
	path[n] = strtok(path_tok, PATH_DELIM);
	while (NULL != path[n]) {
		if ((str_size(path[n]) + x ) > PATH_MAX) {
			cli_error(CL_ENOTSUP,
				"Segment %d of path is too large, search ends at segment %d",
				n, n-1);
			break;
		}
		path[++n] = strtok(NULL, PATH_DELIM);
	}

	/* We now have n places to look for the command */
	for (i=0; path[i]; i++) {
		memset(found, 0, sizeof(found));
		snprintf(found, PATH_MAX, "%s/%s", path[i], cmd);
		if (-1 != try_access(found)) {
			free(path_tok);
			return (char *) found;
		}
	}

	/* We didn't find it, just give it back as-is. */
	free(path_tok);
	return (char *) cmd;
}

unsigned int try_exec(char *cmd, char **argv)
{
	task_id_t tid;
	char *tmp;

	tmp = strdup(find_command(cmd));
	free(found);

	tid = task_spawn((const char *)tmp, argv);
	free(tmp);

	if (tid == 0) {
		cli_error(CL_EEXEC, "Cannot spawn `%s'.", cmd);
		return 1;
	} else {
		return 0;
	}
}
