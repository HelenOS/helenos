/*
 * SPDX-FileCopyrightText: 2012 Alexander Prutkov
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "config.h"
#include "util.h"
#include "errors.h"
#include "entry.h"
#include "echo.h"
#include "cmds.h"
#include "errno.h"

static const char *cmdname = "echo";

void help_cmd_echo(unsigned int level)
{
	if (level == HELP_SHORT) {
		printf("`%s' prints arguments as they are, followed by a new line.\n", cmdname);
	} else {
		help_cmd_echo(HELP_SHORT);
		printf("Usage:  %s [arg ...]\n", cmdname);
	}

	return;
}

/* Main entry point for echo, accepts an array of arguments */
int cmd_echo(char **argv)
{
	unsigned int argc;

	for (argc = 1; argv[argc] != NULL; argc++) {
		printf("%s ", argv[argc]);
	}
	printf("\n");
	return CMD_SUCCESS;

}
