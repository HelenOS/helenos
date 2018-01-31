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
#include <str_error.h>
#include <stdio.h>
#include <stdlib.h>
#include <io/console.h>
#include <io/keycode.h>
#include <getopt.h>
#include <str.h>
#include <vfs/vfs.h>
#include <dirent.h>
#include "config.h"
#include "util.h"
#include "errors.h"
#include "entry.h"
#include "cp.h"
#include "cmds.h"

#define CP_VERSION "0.0.1"
#define CP_DEFAULT_BUFLEN  1024

static const char *cmdname = "cp";
static console_ctrl_t *con;

static struct option const long_options[] = {
	{ "buffer", required_argument, 0, 'b' },
	{ "force", no_argument, 0, 'f' },
	{ "interactive", no_argument, 0, 'i'},
	{ "recursive", no_argument, 0, 'r' },
	{ "help", no_argument, 0, 'h' },
	{ "version", no_argument, 0, 'v' },
	{ "verbose", no_argument, 0, 'V' },
	{ 0, 0, 0, 0 }
};

typedef enum {
	TYPE_NONE,
	TYPE_FILE,
	TYPE_DIR
} dentry_type_t;

static int copy_file(const char *src, const char *dest,
    size_t blen, int vb);

/** Get the type of a directory entry.
 *
 * @param path	Path of the directory entry.
 *
 * @return TYPE_DIR if the dentry is a directory.
 * @return TYPE_FILE if the dentry is a file.
 * @return TYPE_NONE if the dentry does not exists.
 */
static dentry_type_t get_type(const char *path)
{
	vfs_stat_t s;

	errno_t r = vfs_stat_path(path, &s);

	if (r != EOK)
		return TYPE_NONE;
	else if (s.is_directory)
		return TYPE_DIR;
	else if (s.is_file)
		return TYPE_FILE;

	return TYPE_NONE;
}

static int strtoint(const char *s1)
{
	long t1;

	if (-1 == (t1 = strtol(s1, (char **) NULL, 10)))
		return -1;

	if (t1 <= 0)
		return -1;

	return (int) t1;
}

/** Get the last component of a path.
 *
 * e.g. /data/a  ---> a
 *
 * @param path	Pointer to the path.
 *
 * @return	Pointer to the last component or to the path itself.
 */
static char *get_last_path_component(char *path)
{
	char *ptr;

	ptr = str_rchr(path, '/');
	if (!ptr)
		return path;
	else
		return ptr + 1;
}

/** Merge two paths together.
 *
 * e.g. (path1 = /data/dir, path2 = a/b) --> /data/dir/a/b
 *
 * @param path1		Path to which path2 will be appended.
 * @param path1_size	Size of the path1 buffer.
 * @param path2		Path that will be appended to path1.
 */
static void merge_paths(char *path1, size_t path1_size, char *path2)
{
	const char *delim = "/";

	str_rtrim(path1, '/');
	str_append(path1, path1_size, delim);
	str_append(path1, path1_size, path2);
}

static bool get_user_decision(bool bdefault, const char *message, ...)
{
	va_list args;

	va_start(args, message);
	vprintf(message, args);
	va_end(args);

	while (true) {
		cons_event_t ev;
		console_flush(con);
		console_get_event(con, &ev);
		if (ev.type != CEV_KEY || ev.ev.key.type != KEY_PRESS ||
		    (ev.ev.key.mods & (KM_CTRL | KM_ALT)) != 0) {
			continue;
		}

		switch(ev.ev.key.key) {
		case KC_Y:
			printf("y\n");
			return true;
		case KC_N:
			printf("n\n");
			return false;
		case KC_ENTER:
			printf("%c\n", bdefault ? 'Y' : 'N');
			return bdefault;
		default:
			break;
		}
	}
}

static errno_t do_copy(const char *src, const char *dest,
    size_t blen, int vb, int recursive, int force, int interactive)
{
	errno_t rc = EOK;
	char dest_path[PATH_MAX];
	char src_path[PATH_MAX];
	DIR *dir = NULL;
	struct dirent *dp;

	dentry_type_t src_type = get_type(src);
	dentry_type_t dest_type = get_type(dest);

	const size_t src_len = str_size(src);

	if (src_type == TYPE_FILE) {
		char *src_fname;

		/* Initialize the src_path with the src argument */
		str_cpy(src_path, src_len + 1, src);
		str_rtrim(src_path, '/');
		
		/* Get the last component name from the src path */
		src_fname = get_last_path_component(src_path);
		
		/* Initialize dest_path with the dest argument */
		str_cpy(dest_path, PATH_MAX, dest);

		if (dest_type == TYPE_DIR) {
			/* e.g. cp file_name /data */
			/* e.g. cp file_name /data/ */
			
			/* dest is a directory,
			 * append the src filename to it.
			 */
			merge_paths(dest_path, PATH_MAX, src_fname);
			dest_type = get_type(dest_path);
		} else if (dest_type == TYPE_NONE) {
			if (dest_path[str_size(dest_path) - 1] == '/') {
				/* e.g. cp /textdemo /data/dirnotexists/ */

				printf("The dest directory %s does not exists",
				    dest_path);
				rc = ENOENT;
				goto exit;
			}
		}

		if (dest_type == TYPE_DIR) {
			printf("Cannot overwrite existing directory %s\n",
			    dest_path);
			rc = EEXIST;
			goto exit;
		} else if (dest_type == TYPE_FILE) {
			/* e.g. cp file_name existing_file */

			/* dest already exists, 
			 * if force is set we will try to remove it.
			 * if interactive is set user input is required.
			 */
			if (force && !interactive) {
				rc = vfs_unlink_path(dest_path);
				if (rc != EOK) {
					printf("Unable to remove %s\n",
					    dest_path);
					goto exit;
				}
			} else if (!force && interactive) {
				bool overwrite = get_user_decision(false,
				    "File already exists: %s. Overwrite? [y/N]: ",
				    dest_path);
				if (overwrite) {
					printf("Overwriting file: %s\n", dest_path);
					rc = vfs_unlink_path(dest_path);
					if (rc != EOK) {
						printf("Unable to remove %s\n", dest_path);
						goto exit;
					}
				} else {
					printf("Not overwriting file: %s\n", dest_path);
					rc = EOK;
					goto exit;
				}
			} else {
				printf("File already exists: %s\n", dest_path);
				rc = EEXIST;
				goto exit;
			}
		}

		/* call copy_file and exit */
		if (copy_file(src, dest_path, blen, vb) < 0) {
			rc = EIO;
		}

	} else if (src_type == TYPE_DIR) {
		/* e.g. cp -r /x/srcdir /y/destdir/ */

		if (!recursive) {
			printf("Cannot copy the %s directory without the "
			    "-r option\n", src);
			rc = EINVAL;
			goto exit;
		} else if (dest_type == TYPE_FILE) {
			printf("Cannot overwrite a file with a directory\n");
			rc = EEXIST;
			goto exit;
		}

		char *src_dirname;

		/* Initialize src_path with the content of src */
		str_cpy(src_path, src_len + 1, src);
		str_rtrim(src_path, '/');

		src_dirname = get_last_path_component(src_path);

		str_cpy(dest_path, PATH_MAX, dest);

		switch (dest_type) {
		case TYPE_DIR:
			if (str_cmp(src_dirname, "..") &&
			    str_cmp(src_dirname, ".")) {
				/* The last component of src_path is
				 * not '.' or '..'
				 */
				merge_paths(dest_path, PATH_MAX, src_dirname);

				rc = vfs_link_path(dest_path, KIND_DIRECTORY,
				    NULL);
				if (rc != EOK) {
					printf("Unable to create "
					    "dest directory %s\n", dest_path);
					goto exit;
				}
			}
			break;
		default:
		case TYPE_NONE:
			/* dest does not exists, this means the user wants
			 * to specify the name of the destination directory
			 *
			 * e.g. cp -r /src /data/new_dir_src
			 */
			rc = vfs_link_path(dest_path, KIND_DIRECTORY, NULL);
			if (rc != EOK) {
				printf("Unable to create "
				    "dest directory %s\n", dest_path);
				goto exit;
			}
			break;
		}

		dir = opendir(src);
		if (!dir) {
			/* Something strange is happening... */
			printf("Unable to open src %s directory\n", src);
			rc = ENOENT;
			goto exit;
		}

		/* Copy every single directory entry of src into the
		 * destination directory.
		 */
		while ((dp = readdir(dir))) {
			vfs_stat_t src_s;
			vfs_stat_t dest_s;

			char src_dent[PATH_MAX];
			char dest_dent[PATH_MAX];

			str_cpy(src_dent, PATH_MAX, src);
			merge_paths(src_dent, PATH_MAX, dp->d_name);

			str_cpy(dest_dent, PATH_MAX, dest_path);
			merge_paths(dest_dent, PATH_MAX, dp->d_name);

			/* Check if we are copying a directory into itself */
			vfs_stat_path(src_dent, &src_s);
			vfs_stat_path(dest_path, &dest_s);

			if (dest_s.index == src_s.index &&
			    dest_s.fs_handle == src_s.fs_handle) {
				printf("Cannot copy a directory "
				    "into itself\n");
				rc = EEXIST;
				goto exit;
			}

			if (vb)
				printf("copy %s %s\n", src_dent, dest_dent);

			/* Recursively call do_copy() */
			rc = do_copy(src_dent, dest_dent, blen, vb, recursive,
			    force, interactive);
			if (rc != EOK)
				goto exit;

		}
	} else
		printf("Unable to open source file %s\n", src);

exit:
	if (dir)
		closedir(dir);
	return rc;
}

static int copy_file(const char *src, const char *dest,
	size_t blen, int vb)
{
	int fd1, fd2;
	size_t rbytes, wbytes;
	errno_t rc;
	off64_t total;
	char *buff = NULL;
	aoff64_t posr = 0, posw = 0;
	vfs_stat_t st;

	if (vb)
		printf("Copying %s to %s\n", src, dest);

	rc = vfs_lookup_open(src, WALK_REGULAR, MODE_READ, &fd1);
	if (rc != EOK) {
		printf("Unable to open source file %s\n", src);
		return -1;
	}

	rc = vfs_lookup_open(dest, WALK_REGULAR | WALK_MAY_CREATE, MODE_WRITE, &fd2);
	if (rc != EOK) {
		printf("Unable to open destination file %s\n", dest);
		vfs_put(fd1);
		return -1;
	}

	if (vfs_stat(fd1, &st) != EOK) {
		printf("Unable to fstat %d\n", fd1);
		vfs_put(fd1);
		vfs_put(fd2);
		return -1;	
	}

	total = st.size;
	if (vb)
		printf("%" PRIu64 " bytes to copy\n", total);

	if (NULL == (buff = (char *) malloc(blen))) {
		printf("Unable to allocate enough memory to read %s\n",
		    src);
		rc = ENOMEM;
		goto out;
	}

	while ((rc = vfs_read(fd1, &posr, buff, blen, &rbytes)) == EOK &&
	    rbytes > 0) {
		if ((rc = vfs_write(fd2, &posw, buff, rbytes, &wbytes)) != EOK)
			break;
	}

	if (rc != EOK) {
		printf("\nError copying %s: %s\n", src, str_error(rc));
		return -1;
	}

out:
	vfs_put(fd1);
	vfs_put(fd2);
	if (buff)
		free(buff);
	if (rc != EOK) {
		return -1;
	} else {
		return 0;
	}
}

void help_cmd_cp(unsigned int level)
{
	static char helpfmt[] =
	    "Usage:  %s [options] <source> <dest>\n"
	    "Options:\n"
	    "  -h, --help       A short option summary\n"
	    "  -v, --version    Print version information and exit\n"
	    "  -V, --verbose    Be annoyingly noisy about what's being done\n"
	    "  -f, --force      Do not complain when <dest> exists (overrides a previous -i)\n"
	    "  -i, --interactive Ask what to do when <dest> exists (overrides a previous -f)\n"
	    "  -r, --recursive  Copy entire directories\n"
	    "  -b, --buffer ## Set the read buffer size to ##\n";
	if (level == HELP_SHORT) {
		printf("`%s' copies files and directories\n", cmdname);
	} else {
		help_cmd_cp(HELP_SHORT);
		printf(helpfmt, cmdname);
	}

	return;
}

int cmd_cp(char **argv)
{
	unsigned int argc, verbose = 0;
	int buffer = 0, recursive = 0;
	int force = 0, interactive = 0;
	int c, opt_ind;
	errno_t ret;

	con = console_init(stdin, stdout);
	argc = cli_count_args(argv);

	for (c = 0, optreset = 1, optind = 0, opt_ind = 0; c != -1;) {
		c = getopt_long(argc, argv, "hvVfirb:", long_options, &opt_ind);
		switch (c) { 
		case 'h':
			help_cmd_cp(1);
			return CMD_SUCCESS;
		case 'v':
			printf("%s\n", CP_VERSION);
			return CMD_SUCCESS;
		case 'V':
			verbose = 1;
			break;
		case 'f':
			interactive = 0;
			force = 1;
			break;
		case 'i':
			force = 0;
			interactive = 1;
			break;
		case 'r':
			recursive = 1;
			break;
		case 'b':
			if (-1 == (buffer = strtoint(optarg))) {
				printf("%s: Invalid buffer specification, "
				    "(should be a number greater than zero)\n",
				    cmdname);
				console_done(con);
				return CMD_FAILURE;
			}
			if (verbose)
				printf("Buffer = %d\n", buffer);
			break;
		}
	}

	if (buffer == 0)
		buffer = CP_DEFAULT_BUFLEN;

	argc -= optind;

	if (argc != 2) {
		printf("%s: invalid number of arguments. Try %s --help\n",
		    cmdname, cmdname);
		console_done(con);
		return CMD_FAILURE;
	}

	ret = do_copy(argv[optind], argv[optind + 1], buffer, verbose,
	    recursive, force, interactive);

	console_done(con);

	if (ret == 0)
		return CMD_SUCCESS;
	else
		return CMD_FAILURE;
}

