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

#include <stdio.h>
#include <str.h>
#include <stdarg.h>
#include <stdlib.h>
#include <vfs/vfs.h>
#include <abi/errno.h>

#include "config.h"
#include "errors.h"
#include "util.h"

extern volatile int cli_errno;

/* Count and return the # of elements in an array */
unsigned int cli_count_args(char **args)
{
	unsigned int i;

	i = 0;
	while (args[i] != NULL)
		i++;

	return i;
}

/*
 * (re)allocates memory to store the current working directory, gets
 * and updates the current working directory, formats the prompt
 * string
 */
unsigned int cli_set_prompt(cliuser_t *usr)
{
	usr->cwd = (char *) realloc(usr->cwd, PATH_MAX);
	if (NULL == usr->cwd) {
		cli_error(CL_ENOMEM, "Can not allocate cwd");
		cli_errno = CL_ENOMEM;
		return 1;
	}
	if (vfs_cwd_get(usr->cwd, PATH_MAX) != EOK)
		snprintf(usr->cwd, PATH_MAX, "(unknown)");

	if (usr->prompt)
		free(usr->prompt);
	asprintf(&usr->prompt, "%s # ", usr->cwd);

	return 0;
}

/*
 * Returns true if the string is a relative or an absolute path
 */
bool is_path(const char *cmd)
{

	bool ret = str_lcmp(cmd, "/", 1) == 0;
	ret = ret || str_lcmp(cmd, "./", 2) == 0;
	ret = ret || str_lcmp(cmd, "../", 3) == 0;

	return ret;
}
