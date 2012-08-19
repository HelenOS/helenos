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

/** @addtogroup bithenge
 * @{
 */
/**
 * @file
 * Fake system call errors for testing.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#define BITHENGE_FAILURE_DECLS_ONLY 1
#include "failure.h"

/* This file raises fake errors from system calls, to test that Bithenge
 * handles the errors correctly. It has two primary modes of operation,
 * depending on an environment variable:
 *
 * BITHENGE_FAILURE_INDEX not set: when a system call is made, a child process
 * returns a fake error from that call. If the child process handles the error
 * correctly (exit code is 1), the main process continues without errors. If
 * the child process has a problem, the main process raises the fake error
 * again and shows all stdout and stderr output. For speed, failures only occur
 * for some system calls after the first 128.
 *
 * BITHENGE_FAILURE_INDEX set: the program runs normally until system call
 * number BITHENGE_FAILURE_INDEX is made; a fake error is returned from this
 * call. */

static int g_failure_index = -1;
static int g_failure_index_selected = -2;

static int should_fail(void)
{
	g_failure_index++;

	if (g_failure_index_selected == -2) {
		char *sel_str = getenv("BITHENGE_FAILURE_INDEX");
		if (sel_str) {
			g_failure_index_selected = strtol(sel_str, NULL, 10);
		} else {
			g_failure_index_selected = -1;
		}
	} else if (g_failure_index_selected != -1) {
		if (g_failure_index == g_failure_index_selected)
			return 1; /* breakpoint here */
		return 0;
	}

	/* Only fail half the time after 128 failures, 1/4 the time after 256
	 * failures, 1/8 the time after 512 failures... */
	int index = g_failure_index;
	while (index >= 128) {
		int test = (index & (64 | 1));
		if (test == (64 | 1) || test == 0)
			return 0;
		index >>= 1;
	}

	if (!fork()) {
		/* Child */
		int null = open("/dev/null", O_WRONLY);
		if (null == -1)
			exit(127);
		dup2(null, STDOUT_FILENO);
		dup2(null, STDERR_FILENO);
		close(null);
		return 1;
	}

	/* Parent */
	int status;
	wait(&status);
	if (WIFEXITED(status) && WEXITSTATUS(status) == 1)
		return 0;

	/* The child had an error! We couldn't see it because stdout and stderr
	 * were redirected, and we couldn't debug it easily because it was a
	 * separate process. Do it again without redirecting or forking. */
	fprintf(stderr, "** Fake error raised here (BITHENGE_FAILURE_INDEX=%d)\n",
	    g_failure_index);
	return 1;
}

void *bithenge_failure_malloc(size_t size)
{
	if (should_fail())
		return NULL;
	return malloc(size);
}

void *bithenge_failure_realloc(void *ptr, size_t size)
{
	if (should_fail())
		return NULL;
	return realloc(ptr, size);
}

ssize_t bithenge_failure_read(int fd, void *buf, size_t count)
{
	if (should_fail()) {
		errno = EIO;
		return -1;
	}
	return read(fd, buf, count);
}

off_t bithenge_failure_lseek(int fd, off_t offset, int whither)
{
	if (should_fail()) {
		errno = EINVAL;
		return (off_t) -1;
	}
	return lseek(fd, offset, whither);
}

/** @}
 */
