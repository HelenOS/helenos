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
