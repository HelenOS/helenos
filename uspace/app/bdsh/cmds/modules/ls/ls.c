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
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <str.h>
#include <sort.h>

#include "errors.h"
#include "config.h"
#include "util.h"
#include "entry.h"
#include "ls.h"
#include "cmds.h"

static const char *cmdname = "ls";

static struct option const long_options[] = {
	{ "help", no_argument, 0, 'h' },
	{ "unsort", no_argument, 0, 'u' },	
	{ 0, 0, 0, 0 }
};

/** Compare 2 directory elements.
 *
 * It compares 2 elements of a directory : a file is considered
 * as lower than a directory, and if they have the same type,
 * they are compared alphabetically.
 *
 * @param a		Pointer to the structure of the first element.
 * @param b		Pointer to the structure of the second element.
 * @param arg	Pointer for an other and optionnal argument.
 *
 * @return		-1 if a < b,
 *				1 otherwise.
 */
static int ls_cmp(void *a, void *b, void *arg)
{
	int a_isdir = (*((struct dir_elem_t *)a)).isdir;
	char * a_name = (*((struct dir_elem_t *)a)).name;
	
	int b_isdir = (*((struct dir_elem_t *)b)).isdir;
	char * b_name = (*((struct dir_elem_t *)b)).name;
	
	if ((!a_isdir && b_isdir)
		|| (((!b_isdir && !a_isdir) || (b_isdir && a_isdir))
		&& str_cmp(a_name, b_name) < 0))
		return -1;
	else
		return 1;
}

/** Scan a directory.
 *
 * Scan the content of a directory and print it.
 *
 * @param d		Name of the directory.
 * @param dirp	Directory stream.
 * @param sort	1 if the output must be sorted,
 *				0 otherwise.
 */
static void ls_scan_dir(const char *d, DIR *dirp, int sort)
{
	int alloc_blocks = 20;
	int i = 0;
	int nbdirs = 0;
	int rc = 0;
	char * buff = NULL;
	struct dir_elem_t * tmp = NULL;
	struct dir_elem_t * tosort = NULL;
	struct dirent * dp = NULL;
	struct stat s;
	
	if (! dirp)
		return;

	buff = (char *)malloc(PATH_MAX);
	if (NULL == buff) {
		cli_error(CL_ENOMEM, "ls: failed to scan %s", d);
		return;
	}
	memset(buff, 0, sizeof(buff));
	
	if (!sort) {
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
	
	tosort = (struct dir_elem_t *)malloc(alloc_blocks*sizeof(struct dir_elem_t));
	if (NULL == tosort) {
		cli_error(CL_ENOMEM, "ls: failed to scan %s", d);
		free(buff);
		return;
	}
	memset(tosort, 0, sizeof(tosort));
	
	while ((dp = readdir(dirp))) {
		nbdirs++;
		
		if (nbdirs > alloc_blocks) {
			alloc_blocks += alloc_blocks;
			
			tmp = (struct dir_elem_t *)realloc(tosort,
					alloc_blocks*sizeof(struct dir_elem_t));
			if (NULL == tmp) {
				cli_error(CL_ENOMEM, "ls: failed to scan %s", d);
				for(i=0;i<(nbdirs-1);i++) {
					free(tosort[i].name);
				}
				free(tosort);
				free(buff);
				return;
			}
			
			tosort = tmp;
		}
		
		// fill the name field
		tosort[nbdirs-1].name = (char *)malloc(str_length(dp->d_name)+1);
		if (NULL == tosort[nbdirs-1].name) {
			cli_error(CL_ENOMEM, "ls: failed to scan %s", d);
			for(i=0;i<(nbdirs-1);i++) {
				free(tosort[i].name);
			}
			free(tosort);
			free(buff);
			return;
		}
		
		memset(tosort[nbdirs-1].name, 0, str_length(dp->d_name)+1);
		str_cpy(tosort[nbdirs-1].name, str_length(dp->d_name)+1, dp->d_name);
		
		// fill the isdir field
		memset(buff, 0, sizeof(buff));
		snprintf(buff, PATH_MAX - 1, "%s/%s", d, tosort[nbdirs-1].name);
		
		rc = stat(buff, &s);
		if (rc != 0) {
			printf("ls: skipping bogus node %s\n", buff);
			printf("rc=%d\n", rc);
			for(i=0;i<nbdirs;i++) {
				free(tosort[i].name);
			}
			free(tosort);
			free(buff);
			return;
		}
		
		tosort[nbdirs-1].isdir = s.is_directory ? 1 : 0;
	}
	
	if (!qsort(&tosort[0], nbdirs, sizeof(struct dir_elem_t), ls_cmp, NULL)) {
		printf("Sorting error.\n");
	}
	
	for(i=0;i<nbdirs;i++) {
		memset(buff, 0, sizeof(buff));
		/* Don't worry if inserting a double slash, this will be fixed by
		 * absolutize() later with subsequent calls to open() or readdir() */
		snprintf(buff, PATH_MAX - 1, "%s/%s", d, tosort[i].name);
		ls_print(tosort[i].name, buff);
		free(tosort[i].name);
	}
	
	free(tosort);
	free(buff);

	return;
}

/** Print an entry.
 *
 * ls_print currently does nothing more than print the entry.
 * In the future, we will likely pass the absolute path, and
 * some sort of ls_options structure that controls how each
 * entry is printed and what is printed about it.
 *
 * Now we just print basic DOS style lists.
 *
 * @param name		Name of the entry.
 * @param pathname	Path of the entry.
 */
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
		printf(
		"Usage:  %s [options] [path]\n"
		"If not path is given, the current working directory is used.\n"
		"Options:\n"
		"  -h, --help       A short option summary\n"
		"  -u, --unsort     Do not sort directory entries\n",
		cmdname);
	}

	return;
}

int cmd_ls(char **argv)
{
	unsigned int argc;
	struct stat s;
	char *buff;
	DIR *dirp;
	int c, opt_ind;
	int sort = 1;

	argc = cli_count_args(argv);
	
	for (c = 0, optind = 0, opt_ind = 0; c != -1;) {
		c = getopt_long(argc, argv, "hu", long_options, &opt_ind);
		switch (c) {
		case 'h':
			help_cmd_ls(HELP_LONG);
			return CMD_SUCCESS;
		case 'u':
			sort = 0;
			break;
		}
	}
	
	int dir = (int)argc > optind ? (int)argc-1 : optind-1;
	argc -= (optind-1);
	
	buff = (char *) malloc(PATH_MAX);
	if (NULL == buff) {
		cli_error(CL_ENOMEM, "%s: ", cmdname);
		return CMD_FAILURE;
	}
	memset(buff, 0, sizeof(buff));
	
	if (argc == 1)
		getcwd(buff, PATH_MAX);
	else
		str_cpy(buff, PATH_MAX, argv[dir]);
	
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
		ls_scan_dir(buff, dirp, sort);
		closedir(dirp);
	}

	free(buff);

	return CMD_SUCCESS;
}

