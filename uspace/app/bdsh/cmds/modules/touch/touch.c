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

/*
 * TODO: Options that people would expect, such as specifying the access time,
 * etc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dirent.h>
#include <str.h>
#include <getopt.h>
#include <errno.h>
#include <vfs/vfs.h>

#include "config.h"
#include "errors.h"
#include "util.h"
#include "entry.h"
#include "touch.h"
#include "cmds.h"

static const char *cmdname = "touch";

static struct option const long_options[] = {
	{ "no-create", no_argument, 0, 'c' },
	{ 0, 0, 0, 0 }
};

/* Dispays help for touch in various levels */
void help_cmd_touch(unsigned int level)
{
	if (level == HELP_SHORT) {
		printf("`%s' updates access times of files\n", cmdname);
	} else {
		help_cmd_touch(HELP_SHORT);
		printf("Usage: `%s' [-c|--no-create] <file>...\n\n"
		    "If the file does not exist it will be created empty,\n"
		    "unless -c (--no-create) is supplied.\n\n"
		    "Options:\n"
		    "   -c, --no-create  Do not create new files\n",
		    cmdname);
	}

	return;
}

/* Main entry point for touch, accepts an array of arguments */
int cmd_touch(char **argv)
{
	unsigned int argc = cli_count_args(argv);
	unsigned int i = 0;
	unsigned int ret = 0;
	int c;
	int longind;
	bool no_create = false;
	vfs_stat_t file_stat;
	int fd = -1;
	char *buff = NULL;

	DIR *dirp;

	c = 0;
	optreset = 1;
	optind = 0;
	longind = 0;

	while (c != -1) {
		c = getopt_long(argc, argv, "c", long_options, &longind);
		switch (c) {
		case 'c':
			no_create = true;
			break;
		}
	}

	if (argc - optind < 1) {
		printf("%s: Incorrect number of arguments. Try `help %s extended'\n",
		    cmdname, cmdname);
		return CMD_FAILURE;
	}

	for (i = optind; argv[i] != NULL; i++) {
		buff = str_dup(argv[i]);
		if (buff == NULL) {
			cli_error(CL_ENOMEM, "Out of memory");
			ret++;
			continue;
		}

		dirp = opendir(buff);
		if (dirp) {
			cli_error(CL_ENOTSUP, "`%s' is a directory", buff);
			closedir(dirp);
			free(buff);
			ret++;
			continue;
		}

		/* Check whether file exists if -c (--no-create) option is given */
		if ((!no_create) ||
		    ((no_create) && (vfs_stat_path(buff, &file_stat) == EOK))) {
			errno_t rc = vfs_lookup(buff, WALK_REGULAR | WALK_MAY_CREATE, &fd);
			if (rc != EOK) {
				fd = -1;
			}
		}

		if (fd < 0) {
			cli_error(CL_EFAIL, "Could not update or create `%s'", buff);
			free(buff);
			ret++;
			continue;
		} else {
			vfs_put(fd);
			fd = -1;
		}

		free(buff);
	}

	if (ret)
		return CMD_FAILURE;
	else
		return CMD_SUCCESS;
}
