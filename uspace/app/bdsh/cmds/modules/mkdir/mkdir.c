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
#include <stdint.h>
#include <dirent.h>
#include <getopt.h>
#include <stdarg.h>
#include <str.h>
#include <errno.h>
#include <str_error.h>
#include <vfs/vfs.h>

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
create_directory(const char *user_path, bool create_parents)
{
	/* Ensure we would always work with absolute and canonified path. */
	char *path = vfs_absolutize(user_path, NULL);
	if (path == NULL) {
		cli_error(CL_ENOMEM, "%s: path too big?", cmdname);
		return 1;
	}

	int ret = 0;
	errno_t rc;

	if (!create_parents) {
		rc = vfs_link_path(path, KIND_DIRECTORY, NULL);
		if (rc != EOK) {
			cli_error(CL_EFAIL, "%s: could not create %s (%s)",
			    cmdname, path, str_error(rc));
			ret = 1;
		}
	} else {
		/* Create the parent directories as well. */
		size_t off = 0;
		while (1) {
			size_t prev_off = off;
			wchar_t cur_char = str_decode(path, &off, STR_NO_LIMIT);
			if ((cur_char == 0) || (cur_char == U_SPECIAL)) {
				break;
			}
			if (cur_char != '/') {
				continue;
			}
			if (prev_off == 0) {
				continue;
			}
			/*
			 * If we are here, it means that:
			 * - we found /
			 * - it is not the first / (no need to create root
			 *   directory)
			 *
			 * We would now overwrite the / with 0 to terminate the
			 * string (that shall be okay because we are
			 * overwriting at the beginning of UTF sequence).
			 * That would allow us to create the directories
			 * in correct nesting order.
			 *
			 * Notice that we ignore EEXIST errors as some of
			 * the parent directories may already exist.
			 */
			char slash_char = path[prev_off];
			path[prev_off] = 0;

			rc = vfs_link_path(path, KIND_DIRECTORY, NULL);
			if (rc != EOK && rc != EEXIST) {
				cli_error(CL_EFAIL, "%s: could not create %s (%s)",
				    cmdname, path, str_error(rc));
				ret = 1;
				goto leave;
			}

			path[prev_off] = slash_char;
		}
		/* Create the final directory. */
		rc = vfs_link_path(path, KIND_DIRECTORY, NULL);
		if (rc != EOK) {
			cli_error(CL_EFAIL, "%s: could not create %s (%s)",
			    cmdname, path, str_error(rc));
			ret = 1;
		}
	}

leave:
	free(path);
	return ret;
}

int cmd_mkdir(char **argv)
{
	unsigned int argc, i, ret = 0;
	bool create_parents = false, follow = false, verbose = false;
	int c, opt_ind;

	argc = cli_count_args(argv);

	for (c = 0, optreset = 1, optind = 0, opt_ind = 0; c != -1;) {
		c = getopt_long(argc, argv, "pvhVfm:", long_options, &opt_ind);
		switch (c) {
		case 'p':
			create_parents = true;
			break;
		case 'v':
			verbose = true;
			break;
		case 'h':
			help_cmd_mkdir(HELP_LONG);
			return CMD_SUCCESS;
		case 'V':
			printf("%s\n", MKDIR_VERSION);
			return CMD_SUCCESS;
		case 'f':
			follow = true;
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

	for (i = optind; argv[i] != NULL; i++) {
		if (verbose)
			printf("%s: creating %s%s\n",
				cmdname, argv[i],
				create_parents ? " (and all parents)" : "");
		ret += create_directory(argv[i], create_parents);
	}

	if (follow && (argv[optind] != NULL)) {
		if (vfs_cwd_set(argv[optind]) != EOK)
			printf("%s: Error switching to directory.", cmdname);
	}

	if (ret)
		return CMD_FAILURE;
	else
		return CMD_SUCCESS;
}

