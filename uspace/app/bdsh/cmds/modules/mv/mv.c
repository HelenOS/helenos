/*
 * SPDX-FileCopyrightText: 2009 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <str_error.h>
#include <vfs/vfs.h>
#include "config.h"
#include "util.h"
#include "errors.h"
#include "entry.h"
#include "mv.h"
#include "cmds.h"

static const char *cmdname = "mv";

/* Dispays help for mv in various levels */
void help_cmd_mv(unsigned int level)
{
	printf("'%s' renames files\n", cmdname);
	return;
}

/* Main entry point for mv, accepts an array of arguments */
int cmd_mv(char **argv)
{
	unsigned int argc;
	errno_t rc;

	argc = cli_count_args(argv);
	if (argc != 3) {
		printf("%s: invalid number of arguments.\n",
		    cmdname);
		return CMD_FAILURE;
	}

	rc = vfs_rename_path(argv[1], argv[2]);
	if (rc != EOK) {
		printf("Unable to rename %s to %s: %s)\n",
		    argv[1], argv[2], str_error(rc));
		return CMD_FAILURE;
	}

	return CMD_SUCCESS;
}
