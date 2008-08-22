/* Copyright (c) 2008, Tim Post <tinkertim@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of the original program's authors nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* TODO:
 * Implement -p option when some type of getopt is ported */

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "config.h"
#include "errors.h"
#include "entry.h"
#include "mkdir.h"
#include "cmds.h"

static char *cmdname = "mkdir";

/* Dispays help for mkdir in various levels */
void * help_cmd_mkdir(unsigned int level)
{
	if (level == HELP_SHORT) {
		printf("`%s' creates a new directory\n", cmdname);
	} else {
		help_cmd_mkdir(HELP_SHORT);
		printf("  `%s' <directory>\n", cmdname);
	}

	return CMD_VOID;
}

/* Main entry point for mkdir, accepts an array of arguments */
int * cmd_mkdir(char **argv)
{
	unsigned int argc;
	DIR *dirp;

	/* Count the arguments */
	for (argc = 0; argv[argc] != NULL; argc ++);

	if (argc != 2) {
		printf("%s - incorrect number of arguments. Try `help %s extended'\n",
			cmdname, cmdname);
		return CMD_FAILURE;
	}

	dirp = opendir(argv[1]);
	if (dirp) {
		closedir(dirp);
		cli_error(CL_EEXISTS, "Can not create directory %s", argv[1]);
		return CMD_FAILURE;
	}

	if (mkdir(argv[1], 0) != 0) {
		cli_error(CL_EFAIL, "Could not create %s", argv[1]);
		return CMD_FAILURE;
	}

	return CMD_SUCCESS;
}

