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
#include "cp.h"
#include "cmds.h"

#define CP_VERSION "0.0.1"
#define CP_DEFAULT_BUFLEN  1024

static const char *cmdname = "cp";

static struct option const long_options[] = {
	{ "buffer", required_argument, 0, 'b' },
	{ "force", no_argument, 0, 'f' },
	{ "recursive", no_argument, 0, 'r' },
	{ "help", no_argument, 0, 'h' },
	{ "version", no_argument, 0, 'v' },
	{ "verbose", no_argument, 0, 'V' },
	{ 0, 0, 0, 0 }
};

static int strtoint(const char *s1)
{
	long t1;

	if (-1 == (t1 = strtol(s1, (char **) NULL, 10)))
		return -1;

	if (t1 <= 0)
		return -1;

	return (int) t1;
}

static int64_t copy_file(const char *src, const char *dest,
	size_t blen, int vb)
{
	int fd1, fd2, bytes = 0;
	off_t total = 0;
	int64_t copied = 0;
	char *buff = NULL;

	if (vb)
		printf("Copying %s to %s\n", src, dest);

	if (-1 == (fd1 = open(src, O_RDONLY))) {
		printf("Unable to open source file %s\n", src);
		return -1;
	}

	if (-1 == (fd2 = open(dest, O_CREAT))) {
		printf("Unable to open destination file %s\n", dest);
		close(fd1);
		return -1;
	}

	total = lseek(fd1, 0, SEEK_END);

	if (vb)
		printf("%d bytes to copy\n", total);

	lseek(fd1, 0, SEEK_SET);

	if (NULL == (buff = (char *) malloc(blen))) {
		printf("Unable to allocate enough memory to read %s\n",
		    src);
		copied = -1;
		goto out;
	}

	for (;;) {
		ssize_t res;

		bytes = read(fd1, buff, blen);
		if (bytes <= 0)
			break;
		copied += bytes;
		res = bytes;
		do {
			/*
			 * Theoretically, it may not be enough to call write()
			 * only once. Also the previous read() may have
			 * returned less data than requested.
			 */
			bytes = write(fd2, buff, res);
			if (bytes < 0)
				goto err;
			res -= bytes;
		} while (res > 0);

		/* TODO: re-insert assert() once this is stand alone,
		 * removed as abort() exits the entire shell
		 */
		if (res != 0) {
			printf("\n%d more bytes than actually exist were copied\n", res);
			goto err;
		}
	}

	if (bytes < 0) {
err:
		printf("\nError copying %s, (%d)\n", src, bytes);
		copied = bytes;
	}

out:
	close(fd1);
	close(fd2);
	if (buff)
		free(buff);
	return copied;
}

void help_cmd_cp(unsigned int level)
{
	static char helpfmt[] =
	    "Usage:  %s [options] <source> <dest>\n"
	    "Options: (* indicates not yet implemented)\n"
	    "  -h, --help       A short option summary\n"
	    "  -v, --version    Print version information and exit\n"
	    "* -V, --verbose    Be annoyingly noisy about what's being done\n"
	    "* -f, --force      Do not complain when <dest> exists\n"
	    "* -r, --recursive  Copy entire directories\n"
	    "  -b, --buffer ## Set the read buffer size to ##\n"
	    "Currently, %s is under development, some options may not work.\n";
	if (level == HELP_SHORT) {
		printf("`%s' copies files and directories\n", cmdname);
	} else {
		help_cmd_cp(HELP_SHORT);
		printf(helpfmt, cmdname, cmdname);
	}

	return;
}

int cmd_cp(char **argv)
{
	unsigned int argc, buffer = 0, verbose = 0;
	int c, opt_ind;
	int64_t ret;

	argc = cli_count_args(argv);

	for (c = 0, optind = 0, opt_ind = 0; c != -1;) {
		c = getopt_long(argc, argv, "hvVfrb:", long_options, &opt_ind);
		switch (c) { 
		case 'h':
			help_cmd_cp(1);
			return CMD_SUCCESS;
		case 'v':
			printf("%d\n", CP_VERSION);
			return CMD_SUCCESS;
		case 'V':
			verbose = 1;
			break;
		case 'f':
			break;
		case 'r':
			break;
		case 'b':
			if (-1 == (buffer = strtoint(optarg))) {
				printf("%s: Invalid buffer specification, "
				    "(should be a number greater than zero)\n",
				    cmdname);
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
		return CMD_FAILURE;
	}

	ret = copy_file(argv[optind], argv[optind + 1], buffer, verbose);

	if (verbose)
		printf("%d bytes copied\n", ret);

	if (ret >= 0)
		return CMD_SUCCESS;
	else
		return CMD_FAILURE;
}

