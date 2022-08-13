/*
 * SPDX-FileCopyrightText: 2008 Tim Post
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
