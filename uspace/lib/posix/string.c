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
/** @file
 */

#define LIBPOSIX_INTERNAL

#include "internal/common.h"
#include "string.h"

#include "assert.h"
#include "errno.h"
#include "limits.h"
#include "stdlib.h"

#include "libc/str_error.h"

/**
 * Returns true if s2 is a prefix of s1.
 *
 * @param s1
 * @param s2
 * @return
 */
static bool begins_with(const char *s1, const char *s2)
{
	while (*s1 == *s2 && *s2 != '\0') {
		s1++;
		s2++;
	}
	
	/* true if the end was reached */
	return *s2 == '\0';
}

/**
 * The same as strpbrk, except it returns pointer to the nul terminator
 * if no occurence is found.
 *
 * @param s1
 * @param s2
 * @return
 */
static char *strpbrk_null(const char *s1, const char *s2)
{
	while (!posix_strchr(s2, *s1)) {
		++s1;
	}
	
	return (char *) s1;
}

/**
 *
 * @param dest
 * @param src
 * @return
 */
char *posix_strcpy(char *dest, const char *src)
{
	posix_stpcpy(dest, src);
	return dest;
}

/**
 *
 * @param dest
 * @param src
 * @param n
 * @return
 */
char *posix_strncpy(char *dest, const char *src, size_t n)
{
	posix_stpncpy(dest, src, n);
	return dest;
}

/**
 *
 * @param dest
 * @param src
 * @return Pointer to the nul character in the dest string
 */
char *posix_stpcpy(char *restrict dest, const char *restrict src)
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
 *
 * @param dest
 * @param src
 * @param n
 * @return Pointer to the first written nul character or &dest[n]
 */
char *posix_stpncpy(char *restrict dest, const char *restrict src, size_t n)
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
 *
 * @param dest
 * @param src
 * @return
 */
char *posix_strcat(char *dest, const char *src)
{
	assert(dest != NULL);
	assert(src != NULL);

	posix_strcpy(posix_strchr(dest, '\0'), src);
	return dest;
}

/**
 *
 * @param dest
 * @param src
 * @param n
 * @return
 */
char *posix_strncat(char *dest, const char *src, size_t n)
{
	assert(dest != NULL);
	assert(src != NULL);

	char *zeroptr = posix_strncpy(posix_strchr(dest, '\0'), src, n);
	/* strncpy doesn't append the nul terminator, so we do it here */
	zeroptr[n] = '\0';
	return dest;
}

/**
 *
 * @param dest
 * @param src
 * @param c
 * @param n
 * @return Pointer to the first byte after c in dest if found, NULL otherwise.
 */
void *posix_memccpy(void *dest, const void *src, int c, size_t n)
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
 *
 * @param s
 * @return Newly allocated string
 */
char *posix_strdup(const char *s)
{
	return posix_strndup(s, SIZE_MAX);
}

/**
 *
 * @param s
 * @param n
 * @return Newly allocated string of length at most n
 */
char *posix_strndup(const char *s, size_t n)
{
	assert(s != NULL);

	size_t len = posix_strnlen(s, n);
	char *dup = malloc(len + 1);
	if (dup == NULL) {
		return NULL;
	}

	memcpy(dup, s, len);
	dup[len] = '\0';

	return dup;
}

/**
 *
 * @param mem1
 * @param mem2
 * @param n
 * @return Difference of the first pair of inequal bytes,
 *     or 0 if areas have the same content
 */
int posix_memcmp(const void *mem1, const void *mem2, size_t n)
{
	assert(mem1 != NULL);
	assert(mem2 != NULL);

	const unsigned char *s1 = mem1;
	const unsigned char *s2 = mem2;
	
	for (size_t i = 0; i < n; ++i) {
		if (s1[i] != s2[i]) {
			return s2[i] - s1[i];
		}
	}
	
	return 0;
}

/**
 *
 * @param s1
 * @param s2
 * @return
 */
int posix_strcmp(const char *s1, const char *s2)
{
	assert(s1 != NULL);
	assert(s2 != NULL);

	return posix_strncmp(s1, s2, STR_NO_LIMIT);
}

/**
 *
 * @param s1
 * @param s2
 * @param n
 * @return
 */
int posix_strncmp(const char *s1, const char *s2, size_t n)
{
	assert(s1 != NULL);
	assert(s2 != NULL);

	for (size_t i = 0; i < n; ++i) {
		if (s1[i] != s2[i]) {
			return s2[i] - s1[i];
		}
		if (s1[i] == '\0') {
			break;
		}
	}

	return 0;
}

/**
 *
 * @param mem
 * @param c
 * @param n
 * @return
 */
void *posix_memchr(const void *mem, int c, size_t n)
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
 *
 * @param s
 * @param c
 * @return
 */
char *posix_strchr(const char *s, int c)
{
	assert(s != NULL);
	
	char *res = gnu_strchrnul(s, c);
	return (*res == c) ? res : NULL;
}

/**
 *
 * @param s
 * @param c
 * @return
 */
char *posix_strrchr(const char *s, int c)
{
	assert(s != NULL);
	
	const char *ptr = posix_strchr(s, '\0');
	
	/* the same as in strchr, except it loops in reverse direction */
	while (*ptr != (char) c) {
		if (ptr == s) {
			return NULL;
		}

		ptr++;
	}

	return (char *) ptr;
}

char *gnu_strchrnul(const char *s, int c)
{
	assert(s != NULL);
	
	while (*s != c && *s != '\0') {
		s++;
	}
	
	return (char *) s;
}

/**
 *
 * @param s1
 * @param s2
 * @return
 */
char *posix_strpbrk(const char *s1, const char *s2)
{
	assert(s1 != NULL);
	assert(s2 != NULL);

	char *ptr = strpbrk_null(s1, s2);
	return (*ptr == '\0') ? NULL : ptr;
}

/**
 *
 * @param s1
 * @param s2
 * @return
 */
size_t posix_strcspn(const char *s1, const char *s2)
{
	assert(s1 != NULL);
	assert(s2 != NULL);

	char *ptr = strpbrk_null(s1, s2);
	return (size_t) (ptr - s1);
}

/**
 *
 * @param s1
 * @param s2
 * @return
 */
size_t posix_strspn(const char *s1, const char *s2)
{
	assert(s1 != NULL);
	assert(s2 != NULL);

	const char *ptr;
	for (ptr = s1; *ptr != '\0'; ++ptr) {
		if (!posix_strchr(s2, *ptr)) {
			break;
		}
	}
	return ptr - s1;
}

/**
 *
 * @param s1
 * @param s2
 * @return
 */
char *posix_strstr(const char *s1, const char *s2)
{
	assert(s1 != NULL);
	assert(s2 != NULL);

	/* special case - needle is an empty string */
	if (*s2 == '\0') {
		return (char *) s1;
	}

	// TODO: use faster algorithm
	/* check for prefix from every position - quadratic complexity */
	while (*s1 != '\0') {
		if (begins_with(s1, s2)) {
			return (char *) s1;
		}
		
		s1++;
	}
	
	return NULL;
}

/**
 * Currently ignores locale and just calls strcmp.
 *
 * @param s1
 * @param s2
 * @return
 */
int posix_strcoll(const char *s1, const char *s2)
{
	assert(s1 != NULL);
	assert(s2 != NULL);

	return posix_strcmp(s1, s2);
}

/**
 * strcoll is equal to strcmp here, so this just makes a copy.
 *
 * @param s1
 * @param s2
 * @param n
 * @return
 */
size_t posix_strxfrm(char *s1, const char *s2, size_t n)
{
	assert(s1 != NULL || n == 0);
	assert(s2 != NULL);

	size_t len = posix_strlen(s2);

	if (n > len) {
		posix_strcpy(s1, s2);
	}

	return len;
}

/**
 *
 * @param errnum
 * @return
 */
char *posix_strerror(int errnum)
{
	/* uses function from libc, we just have to negate errno
	 * (POSIX uses positive errorcodes, HelenOS has negative)
	 */
	return (char *) str_error(-errnum);
}

/**
 *
 * @param errnum Error code
 * @param buf Buffer to store a human readable string to
 * @param bufsz Size of buffer pointed to by buf
 * @return
 */
int posix_strerror_r(int errnum, char *buf, size_t bufsz)
{
	assert(buf != NULL);
	
	char *errstr = posix_strerror(errnum);
	/* HelenOS str_error can't fail */
	
	if (posix_strlen(errstr) + 1 > bufsz) {
		return -ERANGE;
	} else {
		posix_strcpy(buf, errstr);
	}

	return 0;
}

/**
 *
 * @param s
 * @return
 */
size_t posix_strlen(const char *s)
{
	assert(s != NULL);
	
	return (size_t) (posix_strchr(s, '\0') - s);
}

/**
 *
 * @param s
 * @param n
 * @return
 */
size_t posix_strnlen(const char *s, size_t n)
{
	assert(s != NULL);
	
	for (size_t sz = 0; sz < n; ++sz) {
		
		if (s[sz] == '\0') {
			return sz;
		}
	}
	
	return n;
}

/** @}
 */
