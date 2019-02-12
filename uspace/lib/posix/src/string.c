/*
 * Copyright (c) 2011 Petr Koupy
 * Copyright (c) 2011 Jiri Zarevucky
 * Copyright (c) 2018 Jiri Svoboda
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

/** @addtogroup libposix
 * @{
 */
/** @file String manipulation.
 */

#include "internal/common.h"
#include <string.h>

#include <assert.h>

#include <errno.h>

#include <limits.h>
#include <stdlib.h>
#include <signal.h>
#include <str.h>
#include <str_error.h>

/**
 * Copy a string.
 *
 * @param dest Destination pre-allocated buffer.
 * @param src Source string to be copied.
 * @return Pointer to the nul character in the destination string.
 */
char *stpcpy(char *restrict dest, const char *restrict src)
{
	assert(dest != NULL);
	assert(src != NULL);

	for (size_t i = 0; ; ++i) {
		dest[i] = src[i];

		if (src[i] == '\0') {
			/* pointer to the terminating nul character */
			return &dest[i];
		}
	}

	/* unreachable */
	return NULL;
}

/**
 * Copy fixed length string.
 *
 * @param dest Destination pre-allocated buffer.
 * @param src Source string to be copied.
 * @param n Number of bytes to be stored into destination buffer.
 * @return Pointer to the first written nul character or &dest[n].
 */
char *stpncpy(char *restrict dest, const char *restrict src, size_t n)
{
	assert(dest != NULL);
	assert(src != NULL);

	for (size_t i = 0; i < n; ++i) {
		dest[i] = src[i];

		/*
		 * the standard requires that nul characters
		 * are appended to the length of n, in case src is shorter
		 */
		if (src[i] == '\0') {
			char *result = &dest[i];
			for (++i; i < n; ++i) {
				dest[i] = '\0';
			}
			return result;
		}
	}

	return &dest[n];
}

/**
 * Copy limited number of bytes in memory.
 *
 * @param dest Destination buffer.
 * @param src Source buffer.
 * @param c Character after which the copying shall stop.
 * @param n Number of bytes that shall be copied if not stopped earlier by c.
 * @return Pointer to the first byte after c in dest if found, NULL otherwise.
 */
void *memccpy(void *restrict dest, const void *restrict src, int c, size_t n)
{
	assert(dest != NULL);
	assert(src != NULL);

	unsigned char *bdest = dest;
	const unsigned char *bsrc = src;

	for (size_t i = 0; i < n; ++i) {
		bdest[i] = bsrc[i];

		if (bsrc[i] == (unsigned char) c) {
			/* pointer to the next byte */
			return &bdest[i + 1];
		}
	}

	return NULL;
}

/**
 * Scan string for a first occurence of a character.
 *
 * @param s String in which to look for the character.
 * @param c Character to look for.
 * @return Pointer to the specified character on success, pointer to the
 *     string terminator otherwise.
 */
char *gnu_strchrnul(const char *s, int c)
{
	assert(s != NULL);

	while (*s != c && *s != '\0') {
		s++;
	}

	return (char *) s;
}

/** Split string by delimiters.
 *
 * @param s             String to be tokenized. May not be NULL.
 * @param delim		String with the delimiters.
 * @param next		Variable which will receive the pointer to the
 *                      continuation of the string following the first
 *                      occurrence of any of the delimiter characters.
 *                      May be NULL.
 * @return              Pointer to the prefix of @a s before the first
 *                      delimiter character. NULL if no such prefix
 *                      exists.
 */
char *strtok_r(char *s, const char *delim, char **next)
{
	return __strtok_r(s, delim, next);
}

/**
 * Get error message string.
 *
 * @param errnum Error code for which to obtain human readable string.
 * @param buf Buffer to store a human readable string to.
 * @param bufsz Size of buffer pointed to by buf.
 * @return Zero on success, errno otherwise.
 */
int strerror_r(int errnum, char *buf, size_t bufsz)
{
	assert(buf != NULL);

	char *errstr = strerror(errnum);

	if (strlen(errstr) + 1 > bufsz) {
		return ERANGE;
	} else {
		strcpy(buf, errstr);
	}

	return EOK;
}

/**
 * Get description of a signal.
 *
 * @param signum Signal number.
 * @return Human readable signal description.
 */
char *strsignal(int signum)
{
	static const char *const sigstrings[] = {
		[SIGABRT] = "SIGABRT (Process abort signal)",
		[SIGALRM] = "SIGALRM (Alarm clock)",
		[SIGBUS] = "SIGBUS (Access to an undefined portion of a memory object)",
		[SIGCHLD] = "SIGCHLD (Child process terminated, stopped, or continued)",
		[SIGCONT] = "SIGCONT (Continue executing, if stopped)",
		[SIGFPE] = "SIGFPE (Erroneous arithmetic operation)",
		[SIGHUP] = "SIGHUP (Hangup)",
		[SIGILL] = "SIGILL (Illegal instruction)",
		[SIGINT] = "SIGINT (Terminal interrupt signal)",
		[SIGKILL] = "SIGKILL (Kill process)",
		[SIGPIPE] = "SIGPIPE (Write on a pipe with no one to read it)",
		[SIGQUIT] = "SIGQUIT (Terminal quit signal)",
		[SIGSEGV] = "SIGSEGV (Invalid memory reference)",
		[SIGSTOP] = "SIGSTOP (Stop executing)",
		[SIGTERM] = "SIGTERM (Termination signal)",
		[SIGTSTP] = "SIGTSTP (Terminal stop signal)",
		[SIGTTIN] = "SIGTTIN (Background process attempting read)",
		[SIGTTOU] = "SIGTTOU (Background process attempting write)",
		[SIGUSR1] = "SIGUSR1 (User-defined signal 1)",
		[SIGUSR2] = "SIGUSR2 (User-defined signal 2)",
		[SIGPOLL] = "SIGPOLL (Pollable event)",
		[SIGPROF] = "SIGPROF (Profiling timer expired)",
		[SIGSYS] = "SIGSYS (Bad system call)",
		[SIGTRAP] = "SIGTRAP (Trace/breakpoint trap)",
		[SIGURG] = "SIGURG (High bandwidth data is available at a socket)",
		[SIGVTALRM] = "SIGVTALRM (Virtual timer expired)",
		[SIGXCPU] = "SIGXCPU (CPU time limit exceeded)",
		[SIGXFSZ] = "SIGXFSZ (File size limit exceeded)"
	};

	if (signum <= _TOP_SIGNAL) {
		return (char *) sigstrings[signum];
	}

	return (char *) "ERROR, Invalid signal number";
}

/** @}
 */
