/*
 * SPDX-FileCopyrightText: 2008 Tim Post
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include "entry.h"
#include "exit.h"
#include "cmds.h"

static const char *cmdname = "exit";

extern volatile unsigned int cli_quit;
extern const char *progname;

void help_cmd_exit(unsigned int level)
{
	printf("Type `%s' to exit %s\n", cmdname, progname);
	return;
}

/** Quits the program and returns the status of whatever command
 * came before invoking 'quit'
 */
int cmd_exit(char *argv[], cliuser_t *usr)
{
	/* Inform that we're outta here */
	cli_quit = 1;
	return CMD_SUCCESS;
}
