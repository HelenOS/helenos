/*
 * Copyright (c) 2009 Jiri Svoboda
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
#include <stdint.h>
#include <str_error.h>
#include <dirent.h>
#include <macros.h>
#include <getopt.h>
#include <stdarg.h>
#include <str.h>
#include <ctype.h>
#include <vfs/vfs.h>

#include "config.h"
#include "errors.h"
#include "util.h"
#include "entry.h"
#include "mkfile.h"
#include "cmds.h"

/** Number of bytes to write at a time */
#define BUFFER_SIZE 16384

static const char *cmdname = "mkfile";

static struct option const long_options[] = {
	{ "size", required_argument, 0, 's' },
	{ "sparse", no_argument, 0, 'p' },
	{ "help", no_argument, 0, 'h' },
	{ 0, 0, 0, 0 }
};

void help_cmd_mkfile(unsigned int level)
{
	if (level == HELP_SHORT) {
		printf("`%s' creates a new zero-filled file\n", cmdname);
	} else {
		help_cmd_mkfile(HELP_SHORT);
		printf(
		    "Usage:  %s [options] <path>\n"
		    "Options:\n"
		    "  -h, --help       A short option summary\n"
		    "  -s, --size sz    Size of the file\n"
		    "  -p, --sparse     Create a sparse file\n"
		    "\n"
		    "Size is a number followed by 'k', 'm' or 'g' for kB, MB, GB.\n"
		    "E.g. 100k, 2m, 1g.\n",
		    cmdname);
	}

	return;
}

/** Parse size specification.
 *
 * Size specification is in the form <decimal_number><unit> where
 * <unit> is 'k', 'm' or 'g' for kB, MB, GB.
 *
 * @param str	String containing the size specification.
 * @param rsize	Place to store size in bytes
 * @return	EOK on success or an error code
 */
static errno_t read_size(const char *str, size_t *rsize)
{
	size_t number, unit;
	char *ep;

	number = strtol(str, &ep, 10);
	if (ep[0] == '\0') {
		*rsize = number;
		return EOK;
	}

	if (ep[1] != '\0')
		return EINVAL;

	switch (tolower(ep[0])) {
	case 'k':
		unit = 1024;
		break;
	case 'm':
		unit = 1024 * 1024;
		break;
	case 'g':
		unit = 1024 * 1024 * 1024;
		break;
	default:
		return EINVAL;
	}

	*rsize = number * unit;
	return EOK;
}

int cmd_mkfile(char **argv)
{
	unsigned int argc;
	int c, opt_ind;
	int fd;
	size_t file_size;
	size_t total_written;
	size_t to_write;
	size_t nwritten;
	errno_t rc;
	char *file_name;
	void *buffer;
	bool create_sparse = false;
	aoff64_t pos = 0;

	file_size = 0;

	argc = cli_count_args(argv);

	c = 0;
	optreset = 1;
	optind = 0;
	opt_ind = 0;

	while (c != -1) {
		c = getopt_long(argc, argv, "ps:h", long_options, &opt_ind);
		switch (c) {
		case 'h':
			help_cmd_mkfile(HELP_LONG);
			return CMD_SUCCESS;
		case 'p':
			create_sparse = true;
			break;
		case 's':
			rc = read_size(optarg, &file_size);
			if (rc != EOK) {
				printf("%s: Invalid file size specification.\n",
				    cmdname);
				return CMD_FAILURE;
			}
			break;
		}
	}

	argc -= optind;

	if (argc != 1) {
		printf("%s: incorrect number of arguments. Try `%s --help'\n",
		    cmdname, cmdname);
		return CMD_FAILURE;
	}

	file_name = argv[optind];

	rc = vfs_lookup_open(file_name, WALK_REGULAR | WALK_MUST_CREATE, MODE_WRITE, &fd);
	if (rc != EOK) {
		printf("%s: failed to create file %s.\n", cmdname, file_name);
		return CMD_FAILURE;
	}

	if (create_sparse && file_size > 0) {
		const char byte = 0x00;

		pos = file_size - 1;
		rc = vfs_write(fd, &pos, &byte, sizeof(char), &nwritten);
		if (rc != EOK) {
			vfs_put(fd);
			goto error;
		}
		return CMD_SUCCESS;
	}

	buffer = calloc(BUFFER_SIZE, 1);
	if (buffer == NULL) {
		printf("%s: Error, out of memory.\n", cmdname);
		return CMD_FAILURE;
	}

	total_written = 0;
	while (total_written < file_size) {
		to_write = min(file_size - total_written, BUFFER_SIZE);
		rc = vfs_write(fd, &pos, buffer, to_write, &nwritten);
		if (rc != EOK) {
			printf("%s: Error writing file (%s).\n", cmdname, str_error(rc));
			vfs_put(fd);
			free(buffer);
			return CMD_FAILURE;
		}
		total_written += nwritten;
	}

	free(buffer);

	rc = vfs_put(fd);
	if (rc != EOK)
		goto error;

	return CMD_SUCCESS;
error:
	printf("%s: Error writing file (%s).\n", cmdname, str_error(rc));
	return CMD_FAILURE;
}
