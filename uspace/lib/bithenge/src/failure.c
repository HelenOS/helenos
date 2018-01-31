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

/** @cond internal */
/** @addtogroup bithenge
 * @{
 */
/**
 * @file
 * Fake system call errors for testing.
 */

#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/wait.h>
#define BITHENGE_FAILURE_DECLS_ONLY 1
#include "failure.h"
#include "common.h"

/* This file raises fake errors from system calls, to test that Bithenge
 * handles the errors correctly. It has two primary modes of operation,
 * depending on an environment variable:
 *
 * BITHENGE_FAILURE_INDEX not set: when a system call is made, a child process
 * returns a fake error from that call. If the child process handles the error
 * correctly (exit code is 1), the main process continues without errors. If
 * the child process has a problem, the main process raises the fake error
 * again and shows all stdout and stderr output. For speed, errors are only
 * raised when part of the backtrace has not been seen before.
 *
 * BITHENGE_FAILURE_INDEX set: the program runs normally until system call
 * number BITHENGE_FAILURE_INDEX is made; a fake error is returned from this
 * call. */

static int g_initialized = 0;
static int g_failure_index = -1;
static int g_failure_index_selected = -1;

typedef struct backtrace_item {
	struct backtrace_item *next;
	void *backtrace_item;
} backtrace_item_t;

static backtrace_item_t *g_backtrace_items = NULL;

static void atexit_handler(void)
{
	while (g_backtrace_items) {
		backtrace_item_t *first = g_backtrace_items;
		g_backtrace_items = first->next;
		free(first);
	}
}

static inline void initialize(void)
{
	g_initialized = 1;
	errno_t rc = atexit(atexit_handler);
	if (rc)
		exit(127);

	char *sel_str = getenv("BITHENGE_FAILURE_INDEX");
	if (sel_str)
		g_failure_index_selected = strtol(sel_str, NULL, 10);
}

/* Record a hit for a backtrace address and return whether this is the first
 * hit. */
static inline errno_t backtrace_item_hit(void *addr)
{
	backtrace_item_t **bip;
	for (bip = &g_backtrace_items; *bip; bip = &(*bip)->next) {
		backtrace_item_t *bi = *bip;
		if (bi->backtrace_item == addr) {
			/* Keep frequently accessed items near the front. */
			*bip = bi->next;
			bi->next = g_backtrace_items;
			g_backtrace_items = bi;
			return 0;
		}
	}

	/* No item found; create one. */
	backtrace_item_t *i = malloc(sizeof(*i));
	if (!i)
		exit(127);
	i->next = g_backtrace_items;
	i->backtrace_item = addr;
	g_backtrace_items = i;
	return 1;
}

errno_t bithenge_should_fail(void)
{
	g_failure_index++;

	if (!g_initialized)
		initialize();

	if (g_failure_index_selected != -1) {
		if (g_failure_index == g_failure_index_selected)
			return 1; /* breakpoint here */
		return 0;
	}

	/* If all backtrace items have been seen already, there's no need to
	 * try raising an error. */
	void *trace[256];
	int size = backtrace(trace, 256);
	int raise_error = 0;
	for (int i = 0; i < size; i++) {
		if (backtrace_item_hit(trace[i]))
			raise_error = 1;
	}
	if (!raise_error)
		return 0;

	if (!fork()) {
		/* Child silently fails. */
		int null = open("/dev/null", O_WRONLY);
		if (null == -1)
			exit(127);
		vfs_clone(null, STDOUT_FILENO, false);
		vfs_clone(null, STDERR_FILENO, false);
		vfs_put(null);
		return 1;
	}

	/* Parent checks whether child failed correctly. */
	int status;
	wait(&status);
	if (WIFEXITED(status) && WEXITSTATUS(status) == 1)
		return 0;

	/* The child had an error! We couldn't easily debug it because it was
	 * in a separate process with redirected stdout and stderr. Do it again
	 * without redirecting or forking. */
	fprintf(stderr, "** Fake error raised here (BITHENGE_FAILURE_INDEX=%d)\n",
	    g_failure_index);
	return 1;
}

void *bithenge_failure_malloc(size_t size)
{
	if (bithenge_should_fail())
		return NULL;
	return malloc(size);
}

void *bithenge_failure_realloc(void *ptr, size_t size)
{
	if (bithenge_should_fail())
		return NULL;
	return realloc(ptr, size);
}

ssize_t bithenge_failure_read(int fd, void *buf, size_t count)
{
	if (bithenge_should_fail()) {
		errno = EIO;
		return -1;
	}
	return read(fd, buf, count);
}

off_t bithenge_failure_lseek(int fd, off_t offset, int whither)
{
	if (bithenge_should_fail()) {
		errno = EINVAL;
		return (off_t) -1;
	}
	return lseek(fd, offset, whither);
}

errno_t bithenge_failure_ferror(FILE *stream)
{
	if (bithenge_should_fail())
		return 1;
	return ferror(stream);
}

char *bithenge_failure_str_ndup(const char *s, size_t max_len)
{
	if (bithenge_should_fail())
		return NULL;
	return str_ndup(s, max_len);
}

errno_t bithenge_failure_open(const char *pathname, int flags)
{
	if (bithenge_should_fail()) {
		errno = EACCES;
		return -1;
	}
	return open(pathname, flags);
}

errno_t bithenge_failure_fstat(int fd, vfs_stat_t *buf)
{
	if (bithenge_should_fail()) {
		errno = EIO;
		return -1;
	}
	return fstat(fd, buf);
}

/** @}
 */
/** @endcond */
