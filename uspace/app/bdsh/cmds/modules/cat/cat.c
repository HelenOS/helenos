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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>

#include "config.h"
#include "util.h"
#include "errors.h"
#include "entry.h"
#include "cat.h"
#include "cmds.h"

static char *cmdname = "cat";
#define CAT_VERSION "0.0.1"
#define CAT_DEFAULT_BUFLEN 1024

static char *cat_oops = "That option is not yet supported\n";

static struct option const long_options[] = {
	{ "help", no_argument, 0, 'h' },
	{ "version", no_argument, 0, 'v' },
	{ "head", required_argument, 0, 'H' },
	{ "tail", required_argument, 0, 't' },
	{ "buffer", required_argument, 0, 'b' },
	{ "more", no_argument, 0, 'm' },
	{ 0, 0, 0, 0 }
};

/* Dispays help for cat in various levels */
void help_cmd_cat(unsigned int level)
{
	if (level == HELP_SHORT) {
		printf("`%s' shows the contents of files\n", cmdname);
	} else {
		help_cmd_cat(HELP_SHORT);
		printf(
		"Usage:  %s [options] <file1> [file2] [...]\n"
		"Options:\n"
		"  -h, --help       A short option summary\n"
		"  -v, --version    Print version information and exit\n"
		"  -H, --head ##    Print only the first ## bytes\n"
		"  -t, --tail ##    Print only the last ## bytes\n"
		"  -b, --buffer ##  Set the read buffer size to ##\n"
		"  -m, --more       Pause after each screen full\n"
		"Currently, %s is under development, some options don't work.\n",
		cmdname, cmdname);
	}

	return;
}

static unsigned int cat_file(const char *fname, size_t blen)
{
	int fd, bytes = 0, count = 0, reads = 0;
	off_t total = 0;
	char *buff = NULL;

	if (-1 == (fd = open(fname, O_RDONLY))) {
		printf("Unable to open %s\n", fname);
		return 1;
	}

	total = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	if (NULL == (buff = (char *) malloc(blen + 1))) {
		close(fd);
		printf("Unable to allocate enough memory to read %s\n",
			fname);
		return 1;
	}

	do {
		bytes = read(fd, buff, blen);
		if (bytes > 0) {
			count += bytes;
			buff[bytes] = '\0';
			printf("%s", buff);
			reads++;
		}
	} while (bytes > 0);

	close(fd);
	if (bytes == -1) {
		printf("Error reading %s\n", fname);
		free(buff);
		return 1;
	}

	free(buff);

	return 0;
}

/* Main entry point for cat, accepts an array of arguments */
int cmd_cat(char **argv)
{
	unsigned int argc, i, ret = 0, buffer = 0;
	int c, opt_ind;

	argc = cli_count_args(argv);

	for (c = 0, optind = 0, opt_ind = 0; c != -1;) {
		c = getopt_long(argc, argv, "hvmH:t:b:", long_options, &opt_ind);
		switch (c) {
		case 'h':
			help_cmd_cat(HELP_LONG);
			return CMD_SUCCESS;
		case 'v':
			printf("%s\n", CAT_VERSION);
			return CMD_SUCCESS;
		case 'H':
			printf(cat_oops);
			return CMD_FAILURE;
		case 't':
			printf(cat_oops);
			return CMD_FAILURE;
		case 'b':
			printf(cat_oops);
			break;
		case 'm':
			printf(cat_oops);
			return CMD_FAILURE;
		}
	}

	argc -= optind;

	if (argc < 1) {
		printf("%s - incorrect number of arguments. Try `%s --help'\n",
			cmdname, cmdname);
		return CMD_FAILURE;
	}

	if (buffer <= 0)
		buffer = CAT_DEFAULT_BUFLEN;

	for (i = optind; argv[i] != NULL; i++)
		ret += cat_file(argv[i], buffer);

	if (ret)
		return CMD_FAILURE;
	else
		return CMD_SUCCESS;
}

