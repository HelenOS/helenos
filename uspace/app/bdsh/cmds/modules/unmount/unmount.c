/*
 * SPDX-FileCopyrightText: 2010 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <vfs/vfs.h>
#include <errno.h>
#include <str_error.h>
#include "config.h"
#include "util.h"
#include "errors.h"
#include "entry.h"
#include "unmount.h"
#include "cmds.h"

static const char *cmdname = "unmount";

/* Dispays help for unmount in various levels */
void help_cmd_unmount(unsigned int level)
{
	if (level == HELP_SHORT) {
		printf("'%s' unmount a file system.\n", cmdname);
	} else {
		help_cmd_unmount(HELP_SHORT);
		printf("Usage: %s <mp>\n", cmdname);
	}
	return;
}

/* Main entry point for unmount, accepts an array of arguments */
int cmd_unmount(char **argv)
{
	unsigned int argc;
	errno_t rc;

	argc = cli_count_args(argv);

	if (argc != 2) {
		printf("%s: invalid number of arguments.\n",
		    cmdname);
		return CMD_FAILURE;
	}

	rc = vfs_unmount_path(argv[1]);
	if (rc != EOK) {
		printf("Unable to unmount %s: %s\n", argv[1], str_error(rc));
		return CMD_FAILURE;
	}

	return CMD_SUCCESS;
}
