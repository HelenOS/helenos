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
 * NOTE:
 * This is a bit of an ugly hack, working around the absence of fstat / etc.
 * As more stuff is completed and exposed in libc, this will improve
 */

#include <errno.h>
#include <str_error.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dirent.h>
#include <getopt.h>
#include <vfs/vfs.h>
#include <str.h>
#include <capa.h>

#include "ls.h"
#include "errors.h"
#include "config.h"
#include "util.h"
#include "entry.h"
#include "cmds.h"

static const char *cmdname = "ls";

static ls_job_t ls;

static struct option const long_options[] = {
	{ "help", no_argument, 0, 'h' },
	{ "unsort", no_argument, 0, 'u' },
	{ "recursive", no_argument, 0, 'r' },
	{ "exact-size", no_argument, 0, 'e' },
	{ "single-column", no_argument, 0, '1' },
	{ 0, 0, 0, 0 }
};

/* Prototypes for the ls command, excluding entry points. */
static unsigned int ls_start(ls_job_t *);
static errno_t ls_print(struct dir_elem_t *);
static errno_t ls_print_single_column(struct dir_elem_t *);
static int ls_cmp_type_name(const void *, const void *);
static int ls_cmp_name(const void *, const void *);
static signed int ls_scan_dir(const char *, DIR *, struct dir_elem_t **);
static unsigned int ls_recursive(const char *, DIR *);
static unsigned int ls_scope(const char *, struct dir_elem_t *);

static unsigned int ls_start(ls_job_t *ls)
{
	ls->recursive = 0;
	ls->sort = 1;

	ls->exact_size = false;
	ls->single_column = false;
	ls->printer = ls_print;
	return 1;
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
 * @param de		Directory element.
 */
static errno_t ls_print(struct dir_elem_t *de)
{
	int width = 13;

	if (de->s.is_file) {
		if (ls.exact_size) {
			printf("%-40s\t%*llu\n", de->name, width, (long long) de->s.size);
			return EOK;
		}

		capa_spec_t capa;
		capa_from_blocks(de->s.size, 1, &capa);
		capa_simplify(&capa);

		char *rptr;
		errno_t rc = capa_format(&capa, &rptr);
		if (rc != EOK) {
			return rc;
		}

		char *sep = str_rchr(rptr, ' ');
		if (sep == NULL) {
			free(rptr);
			return ENOENT;
		}

		*sep = '\0';

		printf("%-40s\t%*s %2s\n", de->name, width - 3, rptr, sep + 1);
		free(rptr);
	} else if (de->s.is_directory)
		printf("%-40s\t%*s\n", de->name, width, "<dir>");
	else
		printf("%-40s\n", de->name);

	return EOK;
}

static errno_t ls_print_single_column(struct dir_elem_t *de)
{
	if (de->s.is_file) {
		printf("%s\n", de->name);
	} else {
		printf("%s/\n", de->name);
	}

	return EOK;
}

/** Compare 2 directory elements.
 *
 * It compares 2 elements of a directory : a file is considered
 * as bigger than a directory, and if they have the same type,
 * they are compared alphabetically.
 *
 * @param a		Pointer to the structure of the first element.
 * @param b		Pointer to the structure of the second element.
 *
 * @return		-1 if a < b, 1 otherwise.
 */
static int ls_cmp_type_name(const void *a, const void *b)
{
	struct dir_elem_t const *da = a;
	struct dir_elem_t const *db = b;

	if ((da->s.is_directory && db->s.is_file) ||
	    ((da->s.is_directory == db->s.is_directory) &&
	    str_cmp(da->name, db->name) < 0))
		return -1;
	else
		return 1;
}

/** Compare directories/files per name
 *
 * This comparision ignores the type of
 * the node. Sorted will strictly by name.
 *
 */
static int ls_cmp_name(const void *a, const void *b)
{
	struct dir_elem_t const *da = a;
	struct dir_elem_t const *db = b;

	return str_cmp(da->name, db->name);
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
static signed int ls_scan_dir(const char *d, DIR *dirp,
    struct dir_elem_t **dir_list_ptr)
{
	int alloc_blocks = 20;
	int i;
	int nbdirs = 0;
	errno_t rc;
	int len;
	char *buff;
	struct dir_elem_t *tmp;
	struct dir_elem_t *tosort;
	struct dirent *dp;

	if (!dirp)
		return -1;

	buff = (char *) malloc(PATH_MAX);
	if (!buff) {
		cli_error(CL_ENOMEM, "ls: failed to scan %s", d);
		return -1;
	}

	tosort = (struct dir_elem_t *) malloc(alloc_blocks * sizeof(*tosort));
	if (!tosort) {
		cli_error(CL_ENOMEM, "ls: failed to scan %s", d);
		free(buff);
		return -1;
	}

	while ((dp = readdir(dirp))) {
		if (nbdirs + 1 > alloc_blocks) {
			alloc_blocks += alloc_blocks;

			tmp = (struct dir_elem_t *) realloc(tosort,
			    alloc_blocks * sizeof(struct dir_elem_t));
			if (!tmp) {
				cli_error(CL_ENOMEM, "ls: failed to scan %s", d);
				goto out;
			}
			tosort = tmp;
		}

		/* fill the name field */
		tosort[nbdirs].name = (char *) malloc(str_size(dp->d_name) + 1);
		if (!tosort[nbdirs].name) {
			cli_error(CL_ENOMEM, "ls: failed to scan %s", d);
			goto out;
		}

		str_cpy(tosort[nbdirs].name, str_size(dp->d_name) + 1, dp->d_name);
		len = snprintf(buff, PATH_MAX - 1, "%s/%s", d, tosort[nbdirs].name);
		buff[len] = '\0';

		rc = vfs_stat_path(buff, &tosort[nbdirs++].s);
		if (rc != EOK) {
			printf("ls: skipping bogus node %s\n", buff);
			printf("error=%s\n", str_error_name(rc));
			goto out;
		}
	}

	if (ls.sort) {
		int (*compar)(const void *, const void *);
		compar = ls.single_column ? ls_cmp_name : ls_cmp_type_name;
		qsort(&tosort[0], nbdirs, sizeof(struct dir_elem_t), compar);
	}

	for (i = 0; i < nbdirs; i++) {
		if (ls.printer(&tosort[i]) != EOK) {
			cli_error(CL_ENOMEM, "%s: Out of memory", cmdname);
			goto out;
		}
	}

	/* Populate the directory list. */
	if (ls.recursive && nbdirs > 0) {
		tmp = (struct dir_elem_t *) realloc(*dir_list_ptr,
		    nbdirs * sizeof(struct dir_elem_t));
		if (!tmp) {
			cli_error(CL_ENOMEM, "ls: failed to scan %s", d);
			goto out;
		}
		*dir_list_ptr = tmp;

		for (i = 0; i < nbdirs; i++) {
			(*dir_list_ptr)[i].name = str_dup(tosort[i].name);
			if (!(*dir_list_ptr)[i].name) {
				cli_error(CL_ENOMEM, "ls: failed to scan %s", d);
				goto out;
			}
		}
	}

out:
	for (i = 0; i < nbdirs; i++)
		free(tosort[i].name);
	free(tosort);
	free(buff);

	return nbdirs;
}

/** Visit a directory recursively.
 *
 * ls_recursive visits all the subdirectories recursively and
 * prints the files and directories in them.
 *
 * @param path	Path the current directory being visited.
 * @param dirp	Directory stream.
 */
static unsigned int ls_recursive(const char *path, DIR *dirp)
{
	int i, nbdirs, ret;
	unsigned int scope;
	char *subdir_path;
	DIR *subdirp;
	struct dir_elem_t *dir_list;

	const char *const trailing_slash = "/";

	nbdirs = 0;
	dir_list = (struct dir_elem_t *) malloc(sizeof(struct dir_elem_t));

	printf("\n%s:\n", path);

	subdir_path = (char *) malloc(PATH_MAX);
	if (!subdir_path) {
		ret = CMD_FAILURE;
		goto out;
	}

	nbdirs = ls_scan_dir(path, dirp, &dir_list);
	if (nbdirs == -1) {
		ret = CMD_FAILURE;
		goto out;
	}

	for (i = 0; i < nbdirs; ++i) {
		memset(subdir_path, 0, PATH_MAX);

		if (str_size(subdir_path) + str_size(path) + 1 <= PATH_MAX)
			str_append(subdir_path, PATH_MAX, path);
		if (path[str_size(path) - 1] != '/' &&
		    str_size(subdir_path) + str_size(trailing_slash) + 1 <= PATH_MAX)
			str_append(subdir_path, PATH_MAX, trailing_slash);
		if (str_size(subdir_path) +
		    str_size(dir_list[i].name) + 1 <= PATH_MAX)
			str_append(subdir_path, PATH_MAX, dir_list[i].name);

		scope = ls_scope(subdir_path, &dir_list[i]);
		switch (scope) {
		case LS_FILE:
			break;
		case LS_DIR:
			subdirp = opendir(subdir_path);
			if (!subdirp) {
				/* May have been deleted between scoping it and opening it */
				cli_error(CL_EFAIL, "Could not stat %s", dir_list[i].name);
				ret = CMD_FAILURE;
				goto out;
			}

			ret = ls_recursive(subdir_path, subdirp);
			closedir(subdirp);
			if (ret == CMD_FAILURE)
				goto out;
			break;
		case LS_BOGUS:
			ret = CMD_FAILURE;
			goto out;
		}
	}

	ret = CMD_SUCCESS;

out:
	for (i = 0; i < nbdirs; i++)
		free(dir_list[i].name);
	free(dir_list);
	free(subdir_path);

	return ret;
}

static unsigned int ls_scope(const char *path, struct dir_elem_t *de)
{
	if (vfs_stat_path(path, &de->s) != EOK) {
		cli_error(CL_ENOENT, "%s", path);
		return LS_BOGUS;
	}

	if (de->s.is_file)
		return LS_FILE;
	else if (de->s.is_directory)
		return LS_DIR;

	return LS_BOGUS;
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
		    "  -h, --help            A short option summary\n"
		    "  -u, --unsort          Do not sort directory entries\n"
		    "  -r, --recursive       List subdirectories recursively\n"
		    "  -e, --exact-size      File sizes will be unformatted (raw bytes count)\n"
		    "  -1, --single-column   Only the names will be returned\n",
		    cmdname);
	}

	return;
}

int cmd_ls(char **argv)
{
	unsigned int argc;
	struct dir_elem_t de;
	DIR *dirp;
	int c, opt_ind;
	int ret = 0;
	unsigned int scope;

	if (!ls_start(&ls)) {
		cli_error(CL_EFAIL, "%s: Could not initialize", cmdname);
		return CMD_FAILURE;
	}

	argc = cli_count_args(argv);

	c = 0;
	optreset = 1;
	optind = 0;
	opt_ind = 0;

	while (c != -1) {
		c = getopt_long(argc, argv, "hure1", long_options, &opt_ind);
		switch (c) {
		case 'h':
			help_cmd_ls(HELP_LONG);
			return CMD_SUCCESS;
		case 'u':
			ls.sort = 0;
			break;
		case 'r':
			ls.recursive = 1;
			break;
		case 'e':
			ls.exact_size = true;
			break;
		case '1':
			ls.single_column = true;
			ls.printer = ls_print_single_column;
			break;
		}
	}

	argc -= optind;

	de.name = (char *) malloc(PATH_MAX);
	if (!de.name) {
		cli_error(CL_ENOMEM, "%s: Out of memory", cmdname);
		return CMD_FAILURE;
	}
	memset(de.name, 0, PATH_MAX);

	if (argc == 0) {
		if (vfs_cwd_get(de.name, PATH_MAX) != EOK) {
			cli_error(CL_EFAIL, "%s: Failed determining working "
			    "directory", cmdname);
			return CMD_FAILURE;
		}
	} else {
		str_cpy(de.name, PATH_MAX, argv[optind]);
	}

	scope = ls_scope(de.name, &de);
	switch (scope) {
	case LS_FILE:
		if (ls.printer(&de) != EOK) {
			cli_error(CL_ENOMEM, "%s: Out of memory", cmdname);
			return CMD_FAILURE;
		}
		break;
	case LS_DIR:
		dirp = opendir(de.name);
		if (!dirp) {
			/* May have been deleted between scoping it and opening it */
			cli_error(CL_EFAIL, "Could not stat %s", de.name);
			free(de.name);
			return CMD_FAILURE;
		}
		if (ls.recursive)
			ret = ls_recursive(de.name, dirp);
		else
			ret = ls_scan_dir(de.name, dirp, NULL);

		closedir(dirp);
		break;
	case LS_BOGUS:
		return CMD_FAILURE;
	}

	free(de.name);

	if (ret == -1 || ret == CMD_FAILURE)
		return CMD_FAILURE;
	else
		return CMD_SUCCESS;
}
