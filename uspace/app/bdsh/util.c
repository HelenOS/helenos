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

/* some platforms do not have strdup, implement it here.
 * Returns a pointer to an allocated string or NULL on failure */
char * cli_strdup(const char *s1)
{
	size_t len = strlen(s1) + 1;
	void *ret = malloc(len);

	if (ret == NULL)
		return (char *) NULL;

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

	if (*s1 == NULL)
		return -1;

	memset(*s1, 0, sizeof(*s1));
	memcpy(*s1, s2, len);
	return (int) len;
}

/* An asprintf() for concantenating paths. Allocates the system PATH_MAX value,
 * expands the formatted string and re-sizes the block s1 points to accordingly.
 *
 * Returns the length of the new s1 on success, -1 on failure. On failure, an
 * attempt is made to return s1 unmodified for sanity, in this case 0 is returned.
 * to indicate that s1 was not modified.
 *
 * FIXME: ugly hack to get around asprintf(), if you use this, CHECK ITS VALUE! */
int cli_psprintf(char **s1, const char *fmt, ...)
{
	va_list ap;
	size_t needed, base = PATH_MAX + 1;
	char *tmp = (char *) malloc(base);

	if (NULL == tmp)
		return -1;

	char *orig = *s1;

	memset(tmp, 0, sizeof(tmp));
	va_start(ap, fmt);
	vsnprintf(tmp, base, fmt, ap);
	va_end(ap);
	needed = strlen(tmp) + 1;
	*s1 = realloc(*s1, needed);
	if (NULL == *s1) {
		*s1 = realloc(*s1, strlen(orig) + 1);
		if (NULL == *s1) {
			free(tmp);
			return -1;
		}
		memset(*s1, 0, sizeof(*s1));
		memcpy(*s1, orig, strlen(orig) + 1);
		free(tmp);
		return 0;
	}
	memset(*s1, 0, sizeof(*s1));
	memcpy(*s1, tmp, needed);
	free(tmp);

	return (int) needed;
}
	
/* Ported from FBSD strtok.c 8.1 (Berkeley) 6/4/93 */
char * cli_strtok_r(char *s, const char *delim, char **last)
{
	char *spanp, *tok;
	int c, sc;

	if (s == NULL && (s = *last) == NULL)
		return (NULL);

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
