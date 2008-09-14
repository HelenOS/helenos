/* Copyright (c) 2008, Tim Post <tinkertim@gmail.com>
 * Copyright (C) 1998 by Wes Peters <wes@softweyr.com>
 * Copyright (c) 1988, 1993 The Regents of the University of California.
 * All rights reserved by all copyright holders.
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

/* NOTES:
 * 1 - Various functions were adapted from FreeBSD (copyright holders noted above)
 *     these functions are identified with comments.
 *
 * 2 - Some of these have since appeared in libc. They remain here for various
 *     reasons, such as the eventual integration of garbage collection for things
 *     that allocate memory and don't automatically free it.
 *
 * 3 - Things that expect a pointer to an allocated string do _no_ sanity checking
 *     if developing on a simulator with no debugger, take care :)
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdarg.h>

#include "config.h"
#include "errors.h"
#include "util.h"

extern volatile int cli_errno;

/* some platforms do not have strdup, implement it here.
 * Returns a pointer to an allocated string or NULL on failure */
char * cli_strdup(const char *s1)
{
	size_t len = strlen(s1) + 1;
	void *ret = malloc(len);

	if (ret == NULL) {
		cli_errno = CL_ENOMEM;
		return (char *) NULL;
	}

	cli_errno = CL_EOK;
	return (char *) memcpy(ret, s1, len);
}

/*
 * Take a previously allocated string (s1), re-size it to accept s2 and copy
 * the contents of s2 into s1.
 * Return -1 on failure, or the length of the copied string on success.
 */
int cli_redup(char **s1, const char *s2)
{
	size_t len = strlen(s2) + 1;

	if (! len)
		return -1;

	*s1 = realloc(*s1, len);

	if (*s1 == NULL) {
		cli_errno = CL_ENOMEM;
		return -1;
	}

	memset(*s1, 0, sizeof(*s1));
	memcpy(*s1, s2, len);
	cli_errno = CL_EOK;
	return (int) len;
}

/* An asprintf() for formatting paths, similar to asprintf() but ensures
 * the returned allocated string is <= PATH_MAX. On failure, an attempt
 * is made to return the original string (if not null) unmodified.
 *
 * Returns: Length of the new string on success, 0 if the string was handed
 * back unmodified, -1 on failure. On failure, cli_errno is set.
 *
 * We do not use POSIX_PATH_MAX, as it is typically much smaller than the
 * PATH_MAX defined by the kernel.
 *
 * Use this like:
 * if (1 > cli_psprintf(&char, "%s/%s", foo, bar)) {
 *   cli_error(cli_errno, "Failed to format path");
 *   stop_what_your_doing_as_your_out_of_memory();
 * }
 */

int cli_psprintf(char **s1, const char *fmt, ...)
{
	va_list ap;
	size_t needed, base = PATH_MAX + 1;
	int skipped = 0;
	char *orig = NULL;
	char *tmp = (char *) malloc(base);

	/* Don't even touch s1, not enough memory */
	if (NULL == tmp) {
		cli_errno = CL_ENOMEM;
		return -1;
	}

	/* If re-allocating s1, save a copy in case we fail */
	if (NULL != *s1)
		orig = cli_strdup(*s1);

	/* Print the string to tmp so we can determine the size that
	 * we actually need */
	memset(tmp, 0, sizeof(tmp));
	va_start(ap, fmt);
	/* vsnprintf will return the # of bytes not written */
	skipped = vsnprintf(tmp, base, fmt, ap);
	va_end(ap);

	/* realloc/alloc s1 to be just the size that we need */
	needed = strlen(tmp) + 1;
	*s1 = realloc(*s1, needed);

	if (NULL == *s1) {
		/* No string lived here previously, or we failed to
		 * make a copy of it, either way there's nothing we
		 * can do. */
		if (NULL == *orig) {
			cli_errno = CL_ENOMEM;
			return -1;
		}
		/* We can't even allocate enough size to restore the
		 * saved copy, just give up */
		*s1 = realloc(*s1, strlen(orig) + 1);
		if (NULL == *s1) {
			free(tmp);
			free(orig);
			cli_errno = CL_ENOMEM;
			return -1;
		}
		/* Give the string back as we found it */
		memset(*s1, 0, sizeof(*s1));
		memcpy(*s1, orig, strlen(orig) + 1);
		free(tmp);
		free(orig);
		cli_errno = CL_ENOMEM;
		return 0;
	}

	/* Ok, great, we have enough room */
	memset(*s1, 0, sizeof(*s1));
	memcpy(*s1, tmp, needed);
	free(tmp);

	/* Free tmp only if s1 was reallocated instead of allocated */
	if (NULL != orig)
		free(orig);

	if (skipped) {
		/* s1 was bigger than PATH_MAX when expanded, however part
		 * of the string was printed. Tell the caller not to use it */
		cli_errno = CL_ETOOBIG;
		return -1;
	}

	/* Success! */
	cli_errno = CL_EOK;
	return (int) needed;
}
	
/* Ported from FBSD strtok.c 8.1 (Berkeley) 6/4/93 */
char * cli_strtok_r(char *s, const char *delim, char **last)
{
	char *spanp, *tok;
	int c, sc;

	if (s == NULL && (s = *last) == NULL) {
		cli_errno = CL_EFAIL;
		return (NULL);
	}

cont:
	c = *s++;
	for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
		if (c == sc)
			goto cont;
	}

	if (c == 0) {		/* no non-delimiter characters */
		*last = NULL;
		return (NULL);
	}

	tok = s - 1;

	for (;;) {
		c = *s++;
		spanp = (char *)delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = '\0';
				*last = s;
				return (tok);
			}
		} while (sc != 0);
	}
}

/* Ported from FBSD strtok.c 8.1 (Berkeley) 6/4/93 */
char * cli_strtok(char *s, const char *delim)
{
	static char *last;

	return (cli_strtok_r(s, delim, &last));
}

/* Count and return the # of elements in an array */
unsigned int cli_count_args(char **args)
{
	unsigned int i;

	for (i=0; args[i] != NULL; i++);
	return i;
}

/* (re)allocates memory to store the current working directory, gets
 * and updates the current working directory, formats the prompt
 * string */
unsigned int cli_set_prompt(cliuser_t *usr)
{
	usr->prompt = (char *) realloc(usr->prompt, PATH_MAX);
	if (NULL == usr->prompt) {
		cli_error(CL_ENOMEM, "Can not allocate prompt");
		cli_errno = CL_ENOMEM;
		return 1;
	}
	memset(usr->prompt, 0, sizeof(usr->prompt));

	usr->cwd = (char *) realloc(usr->cwd, PATH_MAX);
	if (NULL == usr->cwd) {
		cli_error(CL_ENOMEM, "Can not allocate cwd");
		cli_errno = CL_ENOMEM;
		return 1;
	}
	memset(usr->cwd, 0, sizeof(usr->cwd));

	usr->cwd = getcwd(usr->cwd, PATH_MAX - 1);

	if (NULL == usr->cwd)
		snprintf(usr->cwd, PATH_MAX, "(unknown)");

	if (1 < cli_psprintf(&usr->prompt, "%s ", usr->cwd)) {
		cli_error(cli_errno, "Failed to set prompt");
		return 1;
	}

	return 0;
}


