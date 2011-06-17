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
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <stdarg.h>
#include <str.h>

#include "config.h"
#include "errors.h"
#include "util.h"
#include "entry.h"
#include "mkdir.h"
#include "cmds.h"

#define MKDIR_VERSION "0.0.1"

static const char *cmdname = "mkdir";

static struct option const long_options[] = {
	{"parents", no_argument, 0, 'p'},
	{"verbose", no_argument, 0, 'v'},
	{"mode", required_argument, 0, 'm'},
	{"help", no_argument, 0, 'h'},
	{"version", no_argument, 0, 'V'},
	{"follow", no_argument, 0, 'f'},
	{0, 0, 0, 0}
};


void help_cmd_mkdir(unsigned int level)
{
	if (level == HELP_SHORT) {
		printf("`%s' creates a new directory\n", cmdname);
	} else {
		help_cmd_mkdir(HELP_SHORT);
		printf(
		"Usage:  %s [options] <path>\n"
		"Options:\n"
		"  -h, --help       A short option summary\n"
		"  -V, --version    Print version information and exit\n"
		"  -p, --parents    Create needed parents for <path>\n"
		"  -m, --mode       Set permissions to [mode] (UNUSED)\n"
		"  -v, --verbose    Be extremely noisy about what is happening\n"
		"  -f, --follow     Go to the new directory once created\n"
		"Currently, %s is under development, some options don't work.\n",
		cmdname, cmdname);
	}

	return;
}

/* This is kind of clunky, but effective for now */
static unsigned int
create_directory(const char *path, unsigned int p)
{
	DIR *dirp;
	char *tmp = NULL, *buff = NULL, *wdp = NULL;
	char *dirs[255];
	unsigned int absolute = 0, i = 0, ret = 0;

	/* Its a good idea to allocate path, plus we (may) need a copy of
	 * path to tokenize if parents are specified */
	if (NULL == (tmp = str_dup(path))) {
		cli_error(CL_ENOMEM, "%s: path too big?", cmdname);
		return 1;
	}

	if (NULL == (wdp = (char *) malloc(PATH_MAX))) {
		cli_error(CL_ENOMEM, "%s: could not alloc cwd", cmdname);
		free(tmp);
		return 1;
	}

	/* The only reason for wdp is to be (optionally) verbose */
	getcwd(wdp, PATH_MAX);

	/* Typical use without specifying the creation of parents */
	if (p == 0) {
		dirp = opendir(tmp);
		if (dirp) {
			cli_error(CL_EEXISTS, "%s: can not create %s, try -p", cmdname, path);
			closedir(dirp);
			goto finit;
		}
		if (-1 == (mkdir(tmp, 0))) {
			cli_error(CL_EFAIL, "%s: could not create %s", cmdname, path);
			goto finit;
		}
	}

	/* Parents need to be created, path has to be broken up */

	/* See if path[0] is a slash, if so we have to remember to append it */
	if (tmp[0] == '/')
		absolute = 1;

	/* TODO: Canonify the path prior to tokenizing it, see below */
	dirs[i] = strtok(tmp, "/");
	while (dirs[i] && i < 255)
		dirs[++i] = strtok(NULL, "/");

	if (NULL == dirs[0])
		return 1;

	if (absolute == 1) {
		asprintf(&buff, "/%s", dirs[0]);
		mkdir(buff, 0);
		chdir(buff);
		free(buff);
		getcwd(wdp, PATH_MAX);
		i = 1;
	} else {
		i = 0;
	}

	while (dirs[i] != NULL) {
		/* Sometimes make or scripts conjoin odd paths. Account for something
		 * like this: ../../foo/bar/../foo/foofoo/./bar */
		if (!str_cmp(dirs[i], "..") || !str_cmp(dirs[i], ".")) {
			if (0 != (chdir(dirs[i]))) {
				cli_error(CL_EFAIL, "%s: impossible path: %s",
					cmdname, path);
				ret ++;
				goto finit;
			}
			getcwd(wdp, PATH_MAX);
		} else {
			if (-1 == (mkdir(dirs[i], 0))) {
				cli_error(CL_EFAIL,
					"%s: failed at %s/%s", wdp, dirs[i]);
				ret ++;
				goto finit;
			}
			if (0 != (chdir(dirs[i]))) {
				cli_error(CL_EFAIL, "%s: failed creating %s\n",
					cmdname, dirs[i]);
				ret ++;
				break;
			}
		}
		i++;
	}
	goto finit;

finit:
	free(wdp);
	free(tmp);
	return ret;
}

int cmd_mkdir(char **argv)
{
	unsigned int argc, create_parents = 0, i, ret = 0, follow = 0;
	unsigned int verbose = 0;
	int c, opt_ind;
	char *cwd;

	argc = cli_count_args(argv);

	for (c = 0, optind = 0, opt_ind = 0; c != -1;) {
		c = getopt_long(argc, argv, "pvhVfm:", long_options, &opt_ind);
		switch (c) {
		case 'p':
			create_parents = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'h':
			help_cmd_mkdir(HELP_LONG);
			return CMD_SUCCESS;
		case 'V':
			printf("%s\n", MKDIR_VERSION);
			return CMD_SUCCESS;
		case 'f':
			follow = 1;
			break;
		case 'm':
			printf("%s: [W] Ignoring mode %s\n", cmdname, optarg);
			break;
		}
	}

	argc -= optind;

	if (argc < 1) {
		printf("%s - incorrect number of arguments. Try `%s --help'\n",
			cmdname, cmdname);
		return CMD_FAILURE;
	}

	if (NULL == (cwd = (char *) malloc(PATH_MAX))) {
		cli_error(CL_ENOMEM, "%s: could not allocate cwd", cmdname);
		return CMD_FAILURE;
	}

	memset(cwd, 0, sizeof(cwd));
	getcwd(cwd, PATH_MAX);

	for (i = optind; argv[i] != NULL; i++) {
		if (verbose == 1)
			printf("%s: creating %s%s\n",
				cmdname, argv[i],
				create_parents ? " (and all parents)" : "");
		ret += create_directory(argv[i], create_parents);
	}

	if (follow == 0)
		chdir(cwd);

	free(cwd);

	if (ret)
		return CMD_FAILURE;
	else
		return CMD_SUCCESS;
}

