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

/* NOTE:
 * This is a bit of an ugly hack, working around the absence of fstat / etc.
 * As more stuff is completed and exposed in libc, this will improve */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "errors.h"
#include "config.h"
#include "util.h"
#include "entry.h"
#include "ls.h"
#include "cmds.h"

static const char *cmdname = "ls";

static void ls_scan_dir(const char *d, DIR *dirp)
{
	struct dirent *dp;
	char *buff;

	if (! dirp)
		return;

	buff = (char *)malloc(PATH_MAX);
	if (NULL == buff) {
		cli_error(CL_ENOMEM, "ls: failed to scan %s", d);
		return;
	}

	while ((dp = readdir(dirp))) {
		memset(buff, 0, sizeof(buff));
		/* Don't worry if inserting a double slash, this will be fixed by
		 * absolutize() later with subsequent calls to open() or readdir() */
		snprintf(buff, PATH_MAX - 1, "%s/%s", d, dp->d_name);
		ls_print(dp->d_name, buff);
	}

	free(buff);

	return;
}

/* ls_print currently does nothing more than print the entry.
 * in the future, we will likely pass the absolute path, and
 * some sort of ls_options structure that controls how each
 * entry is printed and what is printed about it.
 *
 * Now we just print basic DOS style lists */

static void ls_print(const char *name, const char *pathname)
{
	struct stat s;
	int rc;

	rc = stat(pathname, &s);
	if (rc != 0) {
		/* Odd chance it was deleted from the time readdir() found it */
		printf("ls: skipping bogus node %s\n", pathname);
		printf("rc=%d\n", rc);
		return;
	}
	
	if (s.is_file)
		printf("%-40s\t%llu\n", name, (long long) s.size);
	else if (s.is_directory)
		printf("%-40s\t<dir>\n", name);
	else
		printf("%-40s\n", name);

	return;
}

void help_cmd_ls(unsigned int level)
{
	if (level == HELP_SHORT) {
		printf("`%s' lists files and directories.\n", cmdname);
	} else {
		help_cmd_ls(HELP_SHORT);
		printf("  `%s' [path], if no path is given the current "
				"working directory is used.\n", cmdname);
	}

	return;
}

int cmd_ls(char **argv)
{
	unsigned int argc;
	struct stat s;
	char *buff;
	DIR *dirp;

	argc = cli_count_args(argv);

	buff = (char *) malloc(PATH_MAX);
	if (NULL == buff) {
		cli_error(CL_ENOMEM, "%s: ", cmdname);
		return CMD_FAILURE;
	}
	memset(buff, 0, sizeof(buff));

	if (argc == 1)
		getcwd(buff, PATH_MAX);
	else
		str_cpy(buff, PATH_MAX, argv[1]);

	if (stat(buff, &s)) {
		cli_error(CL_ENOENT, buff);
		free(buff);
		return CMD_FAILURE;
	}

	if (s.is_file) {
		ls_print(buff, buff);
	} else {
		dirp = opendir(buff);
		if (!dirp) {
			/* May have been deleted between scoping it and opening it */
			cli_error(CL_EFAIL, "Could not stat %s", buff);
			free(buff);
			return CMD_FAILURE;
		}
		ls_scan_dir(buff, dirp);
		closedir(dirp);
	}

	free(buff);

	return CMD_SUCCESS;
}

