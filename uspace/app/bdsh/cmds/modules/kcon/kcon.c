/*
 * SPDX-FileCopyrightText: 2008 Tim Post
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <io/console.h>
#include "config.h"
#include "util.h"
#include "errors.h"
#include "entry.h"
#include "kcon.h"
#include "cmds.h"

static const char *cmdname = "kcon";

/* Display help for kcon in various levels */
void help_cmd_kcon(unsigned int level)
{
	printf("`kcon' switches to the kernel debug console.\n");

	if (level != HELP_SHORT)
		printf("Usage: %s\n", cmdname);

	return;
}

/* Main entry point for kcon, accepts an array of arguments */
int cmd_kcon(char **argv)
{
	unsigned int argc = cli_count_args(argv);

	if (argc != 1) {
		printf("%s - incorrect number of arguments. Try `%s --help'\n",
		    cmdname, cmdname);
		return CMD_FAILURE;
	}

	if (console_kcon())
		return CMD_SUCCESS;
	else
		return CMD_FAILURE;
}
