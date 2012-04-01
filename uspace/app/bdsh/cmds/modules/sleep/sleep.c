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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "config.h"
#include "util.h"
#include "errors.h"
#include "entry.h"
#include "sleep.h"
#include "cmds.h"

static const char *cmdname = "sleep";

/* Dispays help for sleep in various levels */
void help_cmd_sleep(unsigned int level)
{
	if (level == HELP_SHORT) {
		printf("`%s' pauses for a given time interval\n", cmdname);
	} else {
		help_cmd_sleep(HELP_SHORT);
		printf(
		"Usage:  %s <duration>\n"
		"The duration is an integer number of seconds.\n",
		cmdname);
	}

	return;
}

/* Main entry point for sleep, accepts an array of arguments */
int cmd_sleep(char **argv)
{
	int ret;
	unsigned int argc;
	uint32_t duration;

	/* Count the arguments */
	argc = cli_count_args(argv);

	if (argc != 2) {
		printf("%s - incorrect number of arguments. Try `help %s'\n",
			cmdname, cmdname);
		return CMD_FAILURE;
	}

	ret = str_uint32_t(argv[1], NULL, 10, true, &duration);
	if (ret != EOK) {
		printf("%s - invalid duration.\n", cmdname);
		return CMD_FAILURE;
	}

	(void) usleep((useconds_t)duration * 1000000);

	return CMD_SUCCESS;
}

