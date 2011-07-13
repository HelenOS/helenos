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

/* TODO: Options that people would expect, such as not creating the file if
 * it doesn't exist, specifying the access time, etc */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <str.h>

#include "config.h"
#include "errors.h"
#include "util.h"
#include "entry.h"
#include "touch.h"
#include "cmds.h"

static const char *cmdname = "touch";

/* Dispays help for touch in various levels */
void help_cmd_touch(unsigned int level)
{
	if (level == HELP_SHORT) {
		printf("`%s' updates access times for files\n", cmdname);
	} else {
		help_cmd_touch(HELP_SHORT);
		printf("  `%s' <file>, if the file does not exist it will be "
				"created\n", cmdname);
	}

	return;
}

/* Main entry point for touch, accepts an array of arguments */
int cmd_touch(char **argv)
{
	unsigned int argc, i = 0, ret = 0;
	int fd;
	char *buff = NULL;

	DIR *dirp;

	argc = cli_count_args(argv);

	if (argc == 1) {
		printf("%s - incorrect number of arguments. Try `help %s extended'\n",
			cmdname, cmdname);
		return CMD_FAILURE;
	}

	for (i = 1; i < argc; i ++) {
		buff = str_dup(argv[i]);
		dirp = opendir(buff);
		if (dirp) {
			cli_error(CL_ENOTSUP, "%s is a directory", buff);
			closedir(dirp);
			ret ++;
			continue;
		}

		fd = open(buff, O_RDWR | O_CREAT);
		if (fd < 0) {
			cli_error(CL_EFAIL, "Could not update / create %s ", buff);
			ret ++;
			continue;
		} else
			close(fd);

		free(buff);
	}

	if (ret)
		return CMD_FAILURE;
	else
		return CMD_SUCCESS;
}

