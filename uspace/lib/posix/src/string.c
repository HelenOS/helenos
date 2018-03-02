/*
 * Copyright (c) 2011 Petr Koupy
 * Copyright (c) 2011 Jiri Zarevucky
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
#include "posix/string.h"

#include <assert.h>

#include <errno.h>

#include "posix/limits.h"
#include "posix/stdlib.h"
#include "posix/signal.h"

#include "libc/str.h"
#include "libc/str_error.h"

/**
 * The same as strpbrk, except it returns pointer to the nul terminator
 * if no occurence is found.
 *
 * @param s1 String in which to look for the bytes.
 * @param s2 String of bytes to look for.
 * @return Pointer to the found byte on success, pointer to the
 *     string terminator otherwise.
 */
static char *strpbrk_null(const char *s1, const char *s2)
{
	while (!strchr(s2, *s1)) {
		++s1;
	}

	return (char *) s1;
}

/**
 * Copy a string.
 *
 * @param dest Destination pre-allocated buffer.
 * @param src Source string to be copied.
 * @return Pointer to the destination buffer.
 */
char *strcpy(char *restrict dest, const char *restrict src)
{
	stpcpy(dest, src);
	return dest;
}

/**
 * Copy fixed length string.
 *
 * @param dest Destination pre-allocated buffer.
 * @param src Source string to be copied.
 * @param n Number of bytes to be stored into destination buffer.
 * @return Pointer to the destination buffer.
 */
char *strncpy(char *restrict dest, const char *restrict src, size_t n)
{
	stpncpy(dest, src, n);
	return dest;
}

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

		/* the standard requires that nul characters
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
 * Concatenate two strings.
 *
 * @param dest String to which src shall be appended.
 * @param src String to be appended after dest.
 * @return Pointer to destination buffer.
 */
char *strcat(char *restrict dest, const char *restrict src)
{
	assert(dest != NULL);
	assert(src != NULL);

	strcpy(strchr(dest, '\0'), src);
	return dest;
}

/**
 * Concatenate a string with part of another.
 *
 * @param dest String to which part of src shall be appended.
 * @param src String whose part shall be appended after dest.
 * @param n Number of bytes to append after dest.
 * @return Pointer to destination buffer.
 */
char *strncat(char *restrict dest, const char *restrict src, size_t n)
{
	assert(dest != NULL);
	assert(src != NULL);

	char *zeroptr = strncpy(strchr(dest, '\0'), src, n);
	/* strncpy doesn't append the nul terminator, so we do it here */
	zeroptr[n] = '\0';
	return dest;
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

	unsigned char* bdest = dest;
	const unsigned char* bsrc = src;

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
 * Duplicate a string.
 *
 * @param s String to be duplicated.
 * @return Newly allocated copy of the string.
 */
char *strdup(const char *s)
{
	return strndup(s, SIZE_MAX);
}

/**
 * Duplicate a specific number of bytes from a string.
 *
 * @param s String to be duplicated.
 * @param n Maximum length of the resulting string..
 * @return Newly allocated string copy of length at most n.
 */
char *strndup(const char *s, size_t n)
{
	assert(s != NULL);

	size_t len = strnlen(s, n);
	char *dup = malloc(len + 1);
	if (dup == NULL) {
		return NULL;
	}

	memcpy(dup, s, len);
	dup[len] = '\0';

	return dup;
}

/**
 * Compare two strings.
 *
 * @param s1 First string to be compared.
 * @param s2 Second string to be compared.
 * @return Difference of the first pair of inequal characters,
 *     or 0 if strings have the same content.
 */
int strcmp(const char *s1, const char *s2)
{
	assert(s1 != NULL);
	assert(s2 != NULL);

	return strncmp(s1, s2, STR_NO_LIMIT);
}

/**
 * Compare part of two strings.
 *
 * @param s1 First string to be compared.
 * @param s2 Second string to be compared.
 * @param n Maximum number of characters to be compared.
 * @return Difference of the first pair of inequal characters,
 *     or 0 if strings have the same content.
 */
int strncmp(const char *s1, const char *s2, size_t n)
{
	assert(s1 != NULL);
	assert(s2 != NULL);

	for (size_t i = 0; i < n; ++i) {
		if (s1[i] != s2[i]) {
			return s1[i] - s2[i];
		}
		if (s1[i] == '\0') {
			break;
		}
	}

	return 0;
}

/**
 * Find byte in memory.
 *
 * @param mem Memory area in which to look for the byte.
 * @param c Byte to look for.
 * @param n Maximum number of bytes to be inspected.
 * @return Pointer to the specified byte on success,
 *     NULL pointer otherwise.
 */
void *memchr(const void *mem, int c, size_t n)
{
	assert(mem != NULL);

	const unsigned char *s = mem;

	for (size_t i = 0; i < n; ++i) {
		if (s[i] == (unsigned char) c) {
			return (void *) &s[i];
		}
	}
	return NULL;
}

/**
 * Scan string for a first occurence of a character.
 *
 * @param s String in which to look for the character.
 * @param c Character to look for.
 * @return Pointer to the specified character on success,
 *     NULL pointer otherwise.
 */
char *strchr(const char *s, int c)
{
	assert(s != NULL);

	char *res = gnu_strchrnul(s, c);
	return (*res == c) ? res : NULL;
}

/**
 * Scan string for a last occurence of a character.
 *
 * @param s String in which to look for the character.
 * @param c Character to look for.
 * @return Pointer to the specified character on success,
 *     NULL pointer otherwise.
 */
char *strrchr(const char *s, int c)
{
	assert(s != NULL);

	const char *ptr = strchr(s, '\0');

	/* the same as in strchr, except it loops in reverse direction */
	while (*ptr != (char) c) {
		if (ptr == s) {
			return NULL;
		}

		ptr--;
	}

	return (char *) ptr;
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

/**
 * Scan a string for a first occurence of one of provided bytes.
 *
 * @param s1 String in which to look for the bytes.
 * @param s2 String of bytes to look for.
 * @return Pointer to the found byte on success,
 *     NULL pointer otherwise.
 */
char *strpbrk(const char *s1, const char *s2)
{
	assert(s1 != NULL);
	assert(s2 != NULL);

	char *ptr = strpbrk_null(s1, s2);
	return (*ptr == '\0') ? NULL : ptr;
}

/**
 * Get the length of a complementary substring.
 *
 * @param s1 String that shall be searched for complementary prefix.
 * @param s2 String of bytes that shall not occur in the prefix.
 * @return Length of the prefix.
 */
size_t strcspn(const char *s1, const char *s2)
{
	assert(s1 != NULL);
	assert(s2 != NULL);

	char *ptr = strpbrk_null(s1, s2);
	return (size_t) (ptr - s1);
}

/**
 * Get length of a substring.
 *
 * @param s1 String that shall be searched for prefix.
 * @param s2 String of bytes that the prefix must consist of.
 * @return Length of the prefix.
 */
size_t strspn(const char *s1, const char *s2)
{
	assert(s1 != NULL);
	assert(s2 != NULL);

	const char *ptr;
	for (ptr = s1; *ptr != '\0'; ++ptr) {
		if (!strchr(s2, *ptr)) {
			break;
		}
	}
	return ptr - s1;
}

/**
 * Find a substring. Uses Knuth-Morris-Pratt algorithm.
 *
 * @param s1 String in which to look for a substring.
 * @param s2 Substring to look for.
 * @return Pointer to the first character of the substring in s1, or NULL if
 *     not found.
 */
char *strstr(const char *haystack, const char *needle)
{
	assert(haystack != NULL);
	assert(needle != NULL);

	/* Special case - needle is an empty string. */
	if (needle[0] == '\0') {
		return (char *) haystack;
	}

	/* Preprocess needle. */
	size_t nlen = strlen(needle);
	size_t prefix_table[nlen + 1];

	{
		size_t i = 0;
		ssize_t j = -1;

		prefix_table[i] = j;

		while (i < nlen) {
			while (j >= 0 && needle[i] != needle[j]) {
				j = prefix_table[j];
			}
			i++; j++;
			prefix_table[i] = j;
		}
	}

	/* Search needle using the precomputed table. */
	size_t npos = 0;

	for (size_t hpos = 0; haystack[hpos] != '\0'; ++hpos) {
		while (npos != 0 && haystack[hpos] != needle[npos]) {
			npos = prefix_table[npos];
		}

		if (haystack[hpos] == needle[npos]) {
			npos++;

			if (npos == nlen) {
				return (char *) (haystack + hpos - nlen + 1);
			}
		}
	}

	return NULL;
}

/** Split string by delimiters.
 *
 * @param s             String to be tokenized. May not be NULL.
 * @param delim		String with the delimiters.
 * @return              Pointer to the prefix of @a s before the first
 *                      delimiter character. NULL if no such prefix
 *                      exists.
 */
char *strtok(char *s, const char *delim)
{
	static char *next;

	return strtok_r(s, delim, &next);
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
	char *start, *end;

	if (s == NULL)
		s = *next;

	/* Skip over leading delimiters. */
	while (*s && (strchr(delim, *s) != NULL)) ++s;
	start = s;

	/* Skip over token characters. */
	while (*s && (strchr(delim, *s) == NULL)) ++s;
	end = s;
	*next = (*s ? s + 1 : s);

	if (start == end) {
		return NULL;	/* No more tokens. */
	}

	/* Overwrite delimiter with NULL terminator. */
	*end = '\0';
	return start;
}

/**
 * String comparison using collating information.
 *
 * Currently ignores locale and just calls strcmp.
 *
 * @param s1 First string to be compared.
 * @param s2 Second string to be compared.
 * @return Difference of the first pair of inequal characters,
 *     or 0 if strings have the same content.
 */
int strcoll(const char *s1, const char *s2)
{
	assert(s1 != NULL);
	assert(s2 != NULL);

	return strcmp(s1, s2);
}

/**
 * Transform a string in such a way that the resulting string yields the same
 * results when passed to the strcmp as if the original string is passed to
 * the strcoll.
 *
 * Since strcoll is equal to strcmp here, this just makes a copy.
 *
 * @param s1 Transformed string.
 * @param s2 Original string.
 * @param n Maximum length of the transformed string.
 * @return Length of the transformed string.
 */
size_t strxfrm(char *restrict s1, const char *restrict s2, size_t n)
{
	assert(s1 != NULL || n == 0);
	assert(s2 != NULL);

	size_t len = strlen(s2);

	if (n > len) {
		strcpy(s1, s2);
	}

	return len;
}

/**
 * Get error message string.
 *
 * @param errnum Error code for which to obtain human readable string.
 * @return Error message.
 */
char *strerror(int errnum)
{
	// FIXME: move strerror() and strerror_r() to libc.
	return (char *) str_error(errnum);
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
 * Get length of the string.
 *
 * @param s String which length shall be determined.
 * @return Length of the string.
 */
size_t strlen(const char *s)
{
	assert(s != NULL);

	return (size_t) (strchr(s, '\0') - s);
}

/**
 * Get limited length of the string.
 *
 * @param s String which length shall be determined.
 * @param n Maximum number of bytes that can be examined to determine length.
 * @return The lower of either string length or n limit.
 */
size_t strnlen(const char *s, size_t n)
{
	assert(s != NULL);

	for (size_t sz = 0; sz < n; ++sz) {

		if (s[sz] == '\0') {
			return sz;
		}
	}

	return n;
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
