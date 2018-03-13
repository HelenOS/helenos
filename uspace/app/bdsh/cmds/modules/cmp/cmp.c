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
#include <getopt.h>
#include <mem.h>
#include <stdio.h>
#include <stdlib.h>
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

static struct option const long_options[] = {
	{ "help", no_argument, 0, 'h' },
	{ "version", no_argument, 0, 'v' },
	{ 0, 0, 0, 0 }
};

/* Dispays help for cat in various levels */
void help_cmd_cmp(unsigned int level)
{
	if (level == HELP_SHORT) {
		printf("`%s' compares the contents of two files\n", cmdname);
	} else {
		help_cmd_cmp(HELP_SHORT);
		printf(
		"Usage:  %s [options] <file1> <file2>\n"
		"Options:\n"
		"  -h, --help       A short option summary\n"
		"  -v, --version    Print version information and exit\n"
		"No output is printed; the return code is 1 if the files differ.\n",
		cmdname);
	}

	return;
}

static errno_t cmp_files(const char *fn0, const char *fn1)
{
	errno_t rc = EOK;
	const char *fn[2] = {fn0, fn1};
	int fd[2] = {-1, -1};
	char buffer[2][CMP_BUFLEN];
	size_t offset[2];
	aoff64_t pos[2] = {};

	for (int i = 0; i < 2; i++) {
		rc = vfs_lookup_open(fn[i], WALK_REGULAR, MODE_READ, &(fd[i]));
		if (rc != EOK) {
			printf("Unable to open %s\n", fn[i]);
			goto end;
		}
	}

	do {
		for (int i = 0; i < 2; i++) {
			rc = vfs_read(fd[i], &pos[i], buffer[i], CMP_BUFLEN,
			    &offset[i]);
			if (rc != EOK) {
				printf("Error reading from %s\n",
				    fn[i]);
				goto end;
			}
		}

		if (offset[0] != offset[1] ||
		    memcmp(buffer[0], buffer[1], offset[0]) != 0) {
			printf("Return 1\n");
			rc = EBUSY;
			goto end;
		}
	} while (offset[0] == CMP_BUFLEN);

end:
	if (fd[0] >= 0)
		vfs_put(fd[0]);
	if (fd[1] >= 0)
		vfs_put(fd[1]);
	return rc;
}

/* Main entry point for cmd, accepts an array of arguments */
int cmd_cmp(char **argv)
{
	errno_t rc;
	unsigned int argc;
	int c, opt_ind;

	argc = cli_count_args(argv);

	c = 0;
	optreset = 1;
	optind = 0;
	opt_ind = 0;

	while (c != -1) {
		c = getopt_long(argc, argv, "hv", long_options, &opt_ind);
		switch (c) {
		case 'h':
			help_cmd_cmp(HELP_LONG);
			return CMD_SUCCESS;
		case 'v':
			printf("%s\n", CMP_VERSION);
			return CMD_SUCCESS;
		}
	}

	if (argc - optind != 2) {
		printf("%s - incorrect number of arguments. Try `%s --help'\n",
			cmdname, cmdname);
		return CMD_FAILURE;
	}

	rc = cmp_files(argv[optind], argv[optind + 1]);
	if (rc != EOK)
		return CMD_FAILURE;
	else
		return CMD_SUCCESS;
}
