/*
 * SPDX-FileCopyrightText: 2008 Tim Post
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <mem.h>
#include <vfs/vfs.h>
#include <abi/errno.h>

#include "config.h"
#include "errors.h"
#include "entry.h"
#include "cmds.h"
#include "pwd.h"

static const char *cmdname = "pwd";

void help_cmd_pwd(unsigned int level)
{
	printf("`%s' prints your current working directory.\n", cmdname);
	return;
}

int cmd_pwd(char *argv[])
{
	char *buff;

	buff = (char *) malloc(PATH_MAX);
	if (NULL == buff) {
		cli_error(CL_ENOMEM, "%s:", cmdname);
		return CMD_FAILURE;
	}

	memset(buff, 0, PATH_MAX);

	if (vfs_cwd_get(buff, PATH_MAX) != EOK) {
		cli_error(CL_EFAIL,
		    "Unable to determine the current working directory");
		free(buff);
		return CMD_FAILURE;
	} else {
		printf("%s\n", buff);
		free(buff);
		return CMD_SUCCESS;
	}
}
