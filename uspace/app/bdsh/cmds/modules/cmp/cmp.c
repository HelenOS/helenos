/*
 * Copyright (c) 2012 Sean Bartell
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
#include <fcntl.h>
#include <mem.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vfs/vfs.h>

#include "cmds.h"
#include "cmp.h"
#include "config.h"
#include "entry.h"
#include "errors.h"
#include "util.h"

static const char *cmdname = "cmp";
#define CMP_VERSION "0.0.1"
#define CMP_BUFLEN 1024

/* Dispays help for cat in various levels */
void help_cmd_cmp(unsigned int level)
{
	if (level == HELP_SHORT) {
		printf("`%s' compares the contents of two files\n", cmdname);
	} else {
		help_cmd_cmp(HELP_SHORT);
		printf(
		"Usage:  %s [options] <file1> <file2>\n"
		"No output is printed; the return code is 1 if the files differ.\n",
		cmdname);
	}

	return;
}

static int cmp_files(const char *fn0, const char *fn1)
{
	int fd0, fd1;
	char buffer0[CMP_BUFLEN], buffer1[CMP_BUFLEN];
	ssize_t size0, size1;
	ssize_t offset0, offset1;

	fd0 = open(fn0, O_RDONLY);
	if (fd0 < 0) {
		printf("Unable to open %s\n", fn0);
		return errno;
	}
	fd1 = open(fn1, O_RDONLY);
	if (fd1 < 0) {
		printf("Unable to open %s\n", fn1);
		return errno;
	}

	do {
		offset0 = offset1 = 0;

		do {
			size0 = read(fd0, buffer0 + offset0,
			    CMP_BUFLEN - offset0);
			if (size0 < 0) {
				printf("Error reading from %s\n", fn0);
				return errno;
			}
			offset0 += size0;
		} while (size0 && offset0 < CMP_BUFLEN);

		do {
			size1 = read(fd1, buffer1 + offset1,
			    CMP_BUFLEN - offset1);
			if (size1 < 0) {
				printf("Error reading from %s\n", fn1);
				return errno;
			}
			offset1 += size1;
		} while (size1 && offset1 < CMP_BUFLEN);

		if (offset0 != offset1)
			return 1;
		if (bcmp(buffer0, buffer1, offset0))
			return 1;
	} while (offset0 == CMP_BUFLEN);

	return 0;
}

/* Main entry point for cmd, accepts an array of arguments */
int cmd_cmp(char **argv)
{
	unsigned int argc;
	int rc;
	
	argc = cli_count_args(argv);
	if (argc != 3) {
		printf("%s - incorrect number of arguments. Try `%s --help'\n",
			cmdname, cmdname);
		return CMD_FAILURE;
	}

	rc = cmp_files(argv[1], argv[2]);
	if (rc)
		return CMD_FAILURE;
	else
		return CMD_SUCCESS;
}
