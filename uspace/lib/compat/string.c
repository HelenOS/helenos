/*
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
 

#include "string.h"
#include "assert.h"
#include <str_error.h>

/**
 * Defined for convenience. Returns pointer to the terminating nul character.
 */
static char *strzero(const char *s) {
	for (; *s != '\0'; ++ s) ;;
	return (char*) s;
}

char *strcpy(char *dest, const char *src) {
	assert (dest != NULL);
	assert (src != NULL);

	return (char *) memcpy(dest, src, strlen(src) + 1);
}

char *strncpy(char *dest, const char *src, size_t n) {
	assert (dest != NULL);
	assert (src != NULL);

	size_t len = strlen(src) + 1;
	memcpy(dest, src, (n < len ? n : len));
	if (n > len) {
		/* the standard requires that nul characters
		   are appended to the length of n */
		memset(dest + len, '\0', n - len);
	}
	return dest;
}

char *strcat(char *dest, const char *src) {
	assert (dest != NULL);
	assert (src != NULL);

	strcpy(strzero(dest), src);
	return dest;
}

char *strncat(char *dest, const char *src, size_t n) {
	assert (dest != NULL);
	assert (src != NULL);

	char *zeroptr = strncpy(strzero(dest), src, n);
	/* strncpy doesn't append the nul terminator */
	zeroptr[n] = '\0';
	return dest;
}

int memcmp(const void *mem1, const void *mem2, size_t n) {
	assert (mem1 != NULL);
	assert (mem2 != NULL);

	const unsigned char *s1 = mem1;
	const unsigned char *s2 = mem2;
	
	while (n != 0 && *s1 == *s2) {
		n --;
		s1 ++;
		s2 ++;
	}
	
	return (n == 0) ? 0 : (*s2 - *s1);
}

int strcmp(const char *s1, const char *s2) {
	assert (s1 != NULL);
	assert (s2 != NULL);

	return strncmp(s1, s2, STR_NO_LIMIT);
}

int strncmp(const char *s1, const char *s2, size_t n) {
	assert (s1 != NULL);
	assert (s2 != NULL);

	while (n != 0 && *s1 == *s2 && *s1 != '\0') {
		n --;
		s1 ++;
		s2 ++;
	}
	
	return (n == 0) ? 0 : (*s2 - *s1);
}

/* currently ignores locale and just calls strcmp */
int strcoll(const char *s1, const char *s2) {
	assert (s1 != NULL);
	assert (s2 != NULL);

	return strcmp(s1, s2);
}

/* strcoll is stub, so this just makes a copy */
size_t strxfrm(char *s1, const char *s2, size_t n) {
	assert (s1 != NULL || n == 0);
	assert (s2 != NULL);

	size_t len = strlen(s2);

	if (n > len)
		strcpy(s1, s2);

	return len;
}

void *memchr(const void *mem, int c, size_t n) {
	assert (mem != NULL);
	
	const unsigned char *s = mem;
	
	for (; n != 0; -- n) {
		if (*s == (unsigned char) c) {
			return (void *) s;
		}
		s ++;
	}
	return NULL;
}


char *strchr(const char *s, int c) {
	assert (s != NULL);
	
	if (c == '\0')
		return strzero(s);
	
	while (*s != (char) c) {
		if (*s == '\0')
			return NULL;
		
		s ++;
	}
	return (char *) s;
}

char *strrchr(const char *s, int c) {
	assert (s != NULL);
	
	const char *ptr;
	
	for (ptr = strzero(s); ptr >= s; -- ptr) {
		if (*ptr == (char) c)
			return (char *) ptr;
	}
	return NULL;
}

/* the same as strpbrk, except it returns pointer to the nul terminator
   if no occurence is found */
static char *strpbrk_null(const char *s1, const char *s2) {
	while (!strchr(s2, *s1))
		++ s1;
	return (char *) s1;
}

size_t strcspn(const char *s1, const char *s2) {
	assert (s1 != NULL);
	assert (s2 != NULL);

	char *ptr = strpbrk_null(s1, s2);
	return (size_t) (ptr - s1);
}

char *strpbrk(const char *s1, const char *s2) {
	assert (s1 != NULL);
	assert (s2 != NULL);

	char *ptr = strpbrk_null(s1, s2);
	return (*ptr == '\0') ? NULL : ptr;
}

size_t strspn(const char *s1, const char *s2) {
	assert (s1 != NULL);
	assert (s2 != NULL);

	const char *ptr;
	for (ptr = s1; *ptr != '\0'; ++ ptr) {
		if (!strchr(s2, *ptr))
			break;
	}
	return ptr - s1;
}

/* returns true if s2 is a prefix of s1 */
static bool begins_with(const char *s1, const char *s2) {
	for (; *s1 == *s2 && *s2 != '\0'; ++ s1, ++ s2) ;;
	return *s2 == '\0';
}

char *strstr(const char *s1, const char *s2) {
	assert (s1 != NULL);
	assert (s2 != NULL);

	if (*s2 == '\0')
		return (char *) s1;

	while (*s1 != '\0') {
		s1 = strchr(s1, *s2);
		
		if (s1 == NULL)
			return NULL;
		
		if (begins_with(s1, s2))
			return (char *) s1;
		
		s1 ++;
	}
	
	return NULL;
}

char *strerror(int errnum) {
	return (char *) str_error (-errnum);
}

size_t strlen(const char *s) {
	assert (s != NULL);
	return (size_t) (strzero(s) - s);
}

