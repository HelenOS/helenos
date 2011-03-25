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
#include <str.h>

#include "errors.h"
#include "config.h"
#include "util.h"
#include "entry.h"
#include "ls.h"
#include "cmds.h"

static const char *cmdname = "ls";

/** Sort array of files/directories
 *
 * Sort an array containing files and directories,
 * in alphabetical order, with files first.
 *
 * @param d			Current directory.
 * @param tab		Array of file/directories to sort.
 * @param nbdirs	Number of elements.
 *
 */
static void ls_sort_dir(const char *d, char ** tab, int nbdirs)
{
	int i = 0;
	int j = 0;
	int min = 0;
	int rc;
	char * buff1 = NULL;
	char * buff2 = NULL;
	char * tmp = NULL;
	struct stat s1;
	struct stat s2;
	
	buff1 = (char *)malloc(PATH_MAX);
	if (NULL == buff1) {
		cli_error(CL_ENOMEM, "ls: failed to scan %s", d);
		return;
	}
	
	buff2 = (char *)malloc(PATH_MAX);
	if (NULL == buff2) {
		cli_error(CL_ENOMEM, "ls: failed to scan %s", d);
		return;
	}
	
	for(i=0;i<nbdirs;i++) {
		min = i;
		
		for(j=i;j<nbdirs;j++) {
			memset(buff1, 0, sizeof(buff1));
			memset(buff2, 0, sizeof(buff2));
			snprintf(buff1, PATH_MAX - 1, "%s/%s", d, tab[min]);
			snprintf(buff2, PATH_MAX - 1, "%s/%s", d, tab[j]);
			
			rc = stat(buff1, &s1);
			if (rc != 0) {
				printf("ls: skipping bogus node %s\n", buff1);
				printf("rc=%d\n", rc);
				return;
			}
			
			rc = stat(buff2, &s2);
			if (rc != 0) {
				printf("ls: skipping bogus node %s\n", buff2);
				printf("rc=%d\n", rc);
				return;
			}
			
			if ((s2.is_file && s1.is_directory)
				|| (((s2.is_file && s1.is_file)
				|| (s2.is_directory && s1.is_directory))
				&& str_cmp(tab[min], tab[j]) > 0))
			    min = j;
		}
		
		tmp = tab[i];
		tab[i] = tab[min];
		tab[min] = tmp;
	}
	
	free(buff1);
	free(buff2);
	
	return;
}

/** Scan a directory.
 *
 * Scan the content of a directory and print it.
 *
 * @param d		Name of the directory.
 * @param dirp	Directory stream.
 *
 */
static void ls_scan_dir(const char *d, DIR *dirp)
{
	int alloc_blocks = 20;
	int i = 0;
	int nbdirs = 0;
	char * buff = NULL;
	char ** tmp = NULL;
	char ** tosort = NULL;
	struct dirent * dp = NULL;

	if (! dirp)
		return;

	buff = (char *)malloc(PATH_MAX);
	if (NULL == buff) {
		cli_error(CL_ENOMEM, "ls: failed to scan %s", d);
		return;
	}
	
	tosort = (char **)malloc(alloc_blocks*sizeof(char *));
	if (NULL == tosort) {
		cli_error(CL_ENOMEM, "ls: failed to scan %s", d);
		return;
	}
	memset(tosort, 0, sizeof(tosort));
	
	while ((dp = readdir(dirp))) {
		nbdirs++;
		
		if (nbdirs > alloc_blocks) {
			alloc_blocks += alloc_blocks;
			
			tmp = (char **)realloc(tosort, (alloc_blocks)*sizeof(char *));
			if (NULL == tmp) {
				cli_error(CL_ENOMEM, "ls: failed to scan %s", d);
				return;
			}
			
			tosort = tmp;
		}
		
		tosort[nbdirs-1] = (char *)malloc(str_length(dp->d_name)+1);
		if (NULL == tosort[nbdirs-1]) {
				cli_error(CL_ENOMEM, "ls: failed to scan %s", d);
				return;
		}
		memset(tosort[nbdirs-1], 0, str_length(dp->d_name)+1);
		
		str_cpy(tosort[nbdirs-1], str_length(dp->d_name)+1, dp->d_name);
	}
	
	ls_sort_dir(d, tosort, nbdirs);
	
	for(i=0;i<nbdirs;i++) {
		memset(buff, 0, sizeof(buff));
		/* Don't worry if inserting a double slash, this will be fixed by
		 * absolutize() later with subsequent calls to open() or readdir() */
		snprintf(buff, PATH_MAX - 1, "%s/%s", d, tosort[i]);
		ls_print(tosort[i], buff);
		free(tosort[i]);
	}
	
	free(tosort);
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

