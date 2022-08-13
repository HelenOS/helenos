/*
 * SPDX-FileCopyrightText: 2009 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup redir
 * @{
 */
/**
 * @file
 */

#include <stdlib.h>
#include <str.h>
#include <stdio.h>
#include <task.h>
#include <str_error.h>
#include <errno.h>
#include <vfs/vfs.h>

#define NAME  "redir"

static void usage(void)
{
	fprintf(stderr, "Usage: %s [-i <stdin>] [-o <stdout>] [-e <stderr>] -- <cmd> [args ...]\n",
	    NAME);
}

static void reopen(FILE **stream, int fd, const char *path, int flags, int mode,
    const char *fmode)
{
	if (fclose(*stream))
		return;

	*stream = NULL;

	int oldfd;
	errno_t rc = vfs_lookup_open(path, WALK_REGULAR | flags, mode, &oldfd);
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

static task_id_t spawn(task_wait_t *wait, int argc, char *argv[])
{
	const char **args;
	task_id_t id = 0;
	errno_t rc;

	args = (const char **) calloc(argc + 1, sizeof(char *));
	if (!args) {
		fprintf(stderr, "No memory available\n");
		return 0;
	}

	int i;
	for (i = 0; i < argc; i++)
		args[i] = argv[i];

	args[argc] = NULL;

	rc = task_spawnv(&id, wait, argv[0], args);

	free(args);

	if (rc != EOK) {
		fprintf(stderr, "%s: Error spawning %s (%s)\n", NAME, argv[0],
		    str_error(rc));
		return 0;
	}

	return id;
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		usage();
		return -1;
	}

	int i;
	for (i = 1; i < argc; i++) {
		if (str_cmp(argv[i], "-i") == 0) {
			i++;
			if (i >= argc) {
				usage();
				return -2;
			}
			reopen(&stdin, 0, argv[i], 0, MODE_READ, "r");
		} else if (str_cmp(argv[i], "-o") == 0) {
			i++;
			if (i >= argc) {
				usage();
				return -3;
			}
			reopen(&stdout, 1, argv[i], WALK_MAY_CREATE, MODE_WRITE,
			    "w");
		} else if (str_cmp(argv[i], "-e") == 0) {
			i++;
			if (i >= argc) {
				usage();
				return -4;
			}
			reopen(&stderr, 2, argv[i], WALK_MAY_CREATE, MODE_WRITE,
			    "w");
		} else if (str_cmp(argv[i], "--") == 0) {
			i++;
			break;
		}
	}

	if (i >= argc) {
		usage();
		return -5;
	}

	/*
	 * FIXME: fdopen() should actually detect that we are opening a console
	 * and it should set line-buffering mode automatically.
	 */
	setvbuf(stdout, NULL, _IOLBF, BUFSIZ);

	task_wait_t wait;
	task_id_t id = spawn(&wait, argc - i, argv + i);

	if (id != 0) {
		task_exit_t texit;
		int retval;
		task_wait(&wait, &texit, &retval);

		return retval;
	}

	return -6;
}

/** @}
 */
