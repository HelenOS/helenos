/*
 * SPDX-FileCopyrightText: 2008 Tim Post
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
