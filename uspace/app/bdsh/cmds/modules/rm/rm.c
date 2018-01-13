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
#include <dirent.h>
#include <getopt.h>
#include <mem.h>
#include <str.h>
#include <vfs/vfs.h>

#include "config.h"
#include "errors.h"
#include "util.h"
#include "entry.h"
#include "rm.h"
#include "cmds.h"

static const char *cmdname = "rm";
#define RM_VERSION "0.0.1"

static struct option const long_options[] = {
	{ "help", no_argument, 0, 'h' },
	{ "version", no_argument, 0, 'v' },
	{ "recursive", no_argument, 0, 'r' },
	{ "force", no_argument, 0, 'f' },
	{ "safe", no_argument, 0, 's' },
	{ 0, 0, 0, 0 }
};

/* Return values for rm_scope() */
#define RM_BOGUS 0
#define RM_FILE  1
#define RM_DIR   2

/* Flags for rm_update() */
#define _RM_ENTRY   0
#define _RM_ADVANCE 1
#define _RM_REWIND  2
#define _RM_EXIT    3

/* A simple job structure */
typedef struct {
	/* Options set at run time */
	unsigned int force;      /* -f option */
	unsigned int recursive;  /* -r option */
	unsigned int safe;       /* -s option */

	/* Keeps track of the job in progress */
	int advance; /* How far deep we've gone since entering */
	DIR *entry;  /* Entry point to the tree being removed */
	char *owd;   /* Where we were when we invoked rm */
	char *cwd;   /* Current directory being transversed */
	char *nwd;   /* Next directory to be transversed */

	/* Counters */
	int f_removed; /* Number of files unlinked */
	int d_removed; /* Number of directories unlinked */
} rm_job_t;

static rm_job_t rm;

static unsigned int rm_recursive(const char *);

static unsigned int rm_start(rm_job_t *rm)
{
	rm->recursive = 0;
	rm->force = 0;
	rm->safe = 0;

	/* Make sure we can allocate enough memory to store
	 * what is needed in the job structure */
	if (NULL == (rm->nwd = (char *) malloc(PATH_MAX)))
		return 0;
	memset(rm->nwd, 0, PATH_MAX);

	if (NULL == (rm->owd = (char *) malloc(PATH_MAX)))
		return 0;
	memset(rm->owd, 0, PATH_MAX);

	if (NULL == (rm->cwd = (char *) malloc(PATH_MAX)))
		return 0;
	memset(rm->cwd, 0, PATH_MAX);

	vfs_cwd_set(".");

	if (EOK != vfs_cwd_get(rm->owd, PATH_MAX))
		return 0;

	return 1;
}

static void rm_end(rm_job_t *rm)
{
	if (NULL != rm->nwd)
		free(rm->nwd);

	if (NULL != rm->owd)
		free(rm->owd);

	if (NULL != rm->cwd)
		free(rm->cwd);
}

static unsigned int rm_single(const char *path)
{
	if (vfs_unlink_path(path) != EOK) {
		cli_error(CL_EFAIL, "rm: could not remove file %s", path);
		return 1;
	}
	return 0;
}

static unsigned int rm_scope(const char *path)
{
	int fd;
	DIR *dirp;

	dirp = opendir(path);
	if (dirp) {
		closedir(dirp);
		return RM_DIR;
	}

	if (vfs_lookup(path, WALK_REGULAR, &fd) == EOK) {
		vfs_put(fd);
		return RM_FILE;
	}

	return RM_BOGUS;
}

static unsigned int rm_recursive_not_empty_dirs(const char *path)
{
	DIR *dirp;
	struct dirent *dp;
	char buff[PATH_MAX];
	unsigned int scope;
	unsigned int ret = 0;

	dirp = opendir(path);
	if (!dirp) {
		/* May have been deleted between scoping it and opening it */
		cli_error(CL_EFAIL, "Could not open %s", path);
		return ret;
	}

	memset(buff, 0, sizeof(buff));
	while ((dp = readdir(dirp))) {
		snprintf(buff, PATH_MAX - 1, "%s/%s", path, dp->d_name);
		scope = rm_scope(buff);
		switch (scope) {
		case RM_BOGUS:
			break;
		case RM_FILE:
			ret += rm_single(buff);
			break;
		case RM_DIR:
			ret += rm_recursive(buff);
			break;
		}
	}

	closedir(dirp);
	
	return ret;
}

static unsigned int rm_recursive(const char *path)
{
	errno_t rc;
	unsigned int ret = 0;

	/* First see if it will just go away */
	rc = vfs_unlink_path(path);
	if (rc == EOK)
		return 0;

	/* Its not empty, recursively scan it */
	ret = rm_recursive_not_empty_dirs(path);

	/* Delete directory */
	rc = vfs_unlink_path(path);
	if (rc == EOK)
		return 0;

	cli_error(CL_ENOTSUP, "Can not remove %s", path);

	return ret + 1;
}

/* Dispays help for rm in various levels */
void help_cmd_rm(unsigned int level)
{
	if (level == HELP_SHORT) {
		printf("`%s' removes files and directories.\n", cmdname);
	} else {
		help_cmd_rm(HELP_SHORT);
		printf(
		"Usage:  %s [options] <path>\n"
		"Options:\n"
		"  -h, --help       A short option summary\n"
		"  -v, --version    Print version information and exit\n"
		"  -r, --recursive  Recursively remove sub directories\n"
		"  -f, --force      Do not prompt prior to removing files\n"
		"  -s, --safe       Stop if directories change during removal\n\n"
		"Currently, %s is under development, some options don't work.\n",
		cmdname, cmdname);
	}
	return;
}

/* Main entry point for rm, accepts an array of arguments */
int cmd_rm(char **argv)
{
	unsigned int argc;
	unsigned int i, scope, ret = 0;
	int c, opt_ind;
	size_t len;
	char *buff = NULL;

	argc = cli_count_args(argv);

	if (argc < 2) {
		cli_error(CL_EFAIL,
			"%s: insufficient arguments. Try %s --help", cmdname, cmdname);
		return CMD_FAILURE;
	}

	if (!rm_start(&rm)) {
		cli_error(CL_ENOMEM, "%s: could not initialize", cmdname);
		rm_end(&rm);
		return CMD_FAILURE;
	}

	for (c = 0, optreset = 1, optind = 0, opt_ind = 0; c != -1;) {
		c = getopt_long(argc, argv, "hvrfs", long_options, &opt_ind);
		switch (c) {
		case 'h':
			help_cmd_rm(HELP_LONG);
			return CMD_SUCCESS;
		case 'v':
			printf("%s\n", RM_VERSION);
			return CMD_SUCCESS;
		case 'r':
			rm.recursive = 1;
			break;
		case 'f':
			rm.force = 1;
			break;
		case 's':
			rm.safe = 1;
			break;
		}
	}

	if ((unsigned) optind == argc) {
		cli_error(CL_EFAIL,
			"%s: insufficient arguments. Try %s --help", cmdname, cmdname);
		rm_end(&rm);
		return CMD_FAILURE;
	}

	i = optind;
	while (NULL != argv[i]) {
		len = str_size(argv[i]) + 2;
		buff = (char *) realloc(buff, len);
		if (buff == NULL) {
			printf("rm: out of memory\n");
			ret = 1;
			break;
		}
		memset(buff, 0, len);
		snprintf(buff, len, "%s", argv[i]);

		scope = rm_scope(buff);
		switch (scope) {
		case RM_BOGUS: /* FIXME */
		case RM_FILE:
			ret += rm_single(buff);
			break;
		case RM_DIR:
			if (! rm.recursive) {
				printf("%s is a directory, use -r to remove it.\n", buff);
				ret ++;
			} else {
				ret += rm_recursive(buff);
			}
			break;
		}
		i++;
	}

	if (NULL != buff)
		free(buff);

	rm_end(&rm);

	if (ret)
		return CMD_FAILURE;
	else
		return CMD_SUCCESS;
}

