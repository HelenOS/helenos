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

static char *cmdname = "ls";

static inline off_t flen(const char *f)
{
	int fd;
	off_t size;

	fd = open(f, O_RDONLY);
	if (fd == -1)
		return 0;

	size = lseek(fd, 0, SEEK_END);
	close(fd);

	if (size < 0)
		size = 0;

	return size;
}

static unsigned int ls_scope(const char *path)
{
	int fd;
	DIR *dirp;

	dirp = opendir(path);
	if (dirp) {
		closedir(dirp);
		return LS_DIR;
	}

	fd = open(path, O_RDONLY);
	if (fd > 0) {
		close(fd);
		return LS_FILE;
	}

	return LS_BOGUS;
}

static void ls_scan_dir(const char *d, DIR *dirp)
{
	struct dirent *dp;
	unsigned int scope;
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
		scope = ls_scope(buff);
		switch (scope) {
		case LS_DIR:
			ls_print_dir(dp->d_name);
			break;
		case LS_FILE:
			ls_print_file(dp->d_name, buff);
			break;
		case LS_BOGUS:
			/* Odd chance it was deleted from the time readdir() found
			 * it and the time that it was scoped */
			printf("ls: skipping bogus node %s\n", dp->d_name);
			break;
		}
	}

	free(buff);

	return;
}

/* ls_print_* currently does nothing more than print the entry.
 * in the future, we will likely pass the absolute path, and
 * some sort of ls_options structure that controls how each
 * entry is printed and what is printed about it.
 *
 * Now we just print basic DOS style lists */

static void ls_print_dir(const char *d)
{
	printf("%-40s\t<dir>\n", d);

	return;
}

static void ls_print_file(const char *name, const char *pathname)
{
	printf("%-40s\t%llu\n", name, (long long) flen(pathname));

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
	unsigned int scope;
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

	scope = ls_scope(buff);

	switch (scope) {
	case LS_BOGUS:
		cli_error(CL_ENOENT, buff);
		free(buff);
		return CMD_FAILURE;
	case LS_FILE:
		ls_print_file(buff, buff);
		break;
	case LS_DIR:
		dirp = opendir(buff);
		if (! dirp) {
			/* May have been deleted between scoping it and opening it */
			cli_error(CL_EFAIL, "Could not stat %s", buff);
			free(buff);
			return CMD_FAILURE;
		}
		ls_scan_dir(buff, dirp);
		closedir(dirp);
		break;
	}

	free(buff);

	return CMD_SUCCESS;
}

