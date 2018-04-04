/*
 * Copyright (c) 2009 Martin Decky
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

/** @addtogroup getterm GetTerm
 * @brief Console initialization task.
 * @{
 */
/**
 * @file
 */

#include <stdint.h>
#include <stdio.h>
#include <task.h>
#include <str_error.h>
#include <errno.h>
#include <loc.h>
#include <vfs/vfs.h>
#include <str.h>
#include "version.h"
#include "welcome.h"

#define APP_NAME  "getterm"

static void usage(void)
{
	printf("Usage: %s <terminal> <locfs> [--msg] [--wait] -- "
	    "<command> [<arguments...>]\n", APP_NAME);
	printf(" <terminal>    Terminal device\n");
	printf(" <locfs>       Mount point of locfs\n");
	printf(" --msg         Print welcome message\n");
	printf(" --wait        Wait for the terminal to be ready\n");
}

static void reopen(FILE **stream, int fd, const char *path, int mode,
    const char *fmode)
{
	if (fclose(*stream))
		return;

	*stream = NULL;

	int oldfd;
	errno_t rc = vfs_lookup_open(path, WALK_REGULAR, mode, &oldfd);
	if (rc != EOK)
		return;

	if (oldfd != fd) {
		int newfd;
		if (vfs_clone(oldfd, fd, false, &newfd) != EOK)
			return;

		assert(newfd == fd);

		if (vfs_put(oldfd))
			return;
	}

	*stream = fdopen(fd, fmode);
}

int main(int argc, char *argv[])
{
	argv++;
	argc--;
	if (argc < 4) {
		usage();
		return 1;
	}

	char *term = *argv;
	argv++;
	argc--;

	char *locfs = *argv;
	argv++;
	argc--;

	bool print_msg = false;
	bool wait = false;

	while ((argc > 0) && (str_cmp(*argv, "--") != 0)) {
		if (str_cmp(*argv, "--msg") == 0) {
			print_msg = true;
		} else if (str_cmp(*argv, "--wait") == 0) {
			wait = true;
		} else {
			usage();
			return 2;
		}

		argv++;
		argc--;
	}

	if (argc < 1) {
		usage();
		return 3;
	}

	/* Skip "--" */
	argv++;
	argc--;

	char *cmd = *argv;
	char **args = argv;

	if (wait) {
		/* Wait for the terminal service to be ready */
		service_id_t service_id;
		errno_t rc = loc_service_get_id(term, &service_id, IPC_FLAG_BLOCKING);
		if (rc != EOK) {
			printf("%s: Error waiting on %s (%s)\n", APP_NAME, term,
			    str_error(rc));
			return rc;
		}
	}

	char term_node[LOC_NAME_MAXLEN];
	snprintf(term_node, LOC_NAME_MAXLEN, "%s/%s", locfs, term);

	reopen(&stdin, 0, term_node, MODE_READ, "r");
	reopen(&stdout, 1, term_node, MODE_WRITE, "w");
	reopen(&stderr, 2, term_node, MODE_WRITE, "w");

	if (stdin == NULL)
		return 4;

	if (stdout == NULL)
		return 5;

	if (stderr == NULL)
		return 6;

	/*
	 * FIXME: fdopen() should actually detect that we are opening a console
	 * and it should set line-buffering mode automatically.
	 */
	setvbuf(stdout, NULL, _IOLBF, BUFSIZ);

	version_print(term);
	if (print_msg)
		welcome_msg_print();

	task_id_t id;
	task_wait_t twait;

	errno_t rc = task_spawnv(&id, &twait, cmd, (const char *const *) args);
	if (rc != EOK) {
		printf("%s: Error spawning %s (%s)\n", APP_NAME, cmd,
		    str_error(rc));
		return rc;
	}

	task_exit_t texit;
	int retval;
	rc = task_wait(&twait, &texit, &retval);
	if (rc != EOK) {
		printf("%s: Error waiting for %s (%s)\n", APP_NAME, cmd,
		    str_error(rc));
		return rc;
	}

	return 0;
}

/** @}
 */
