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

#ifndef POSIX_STRING_H_
#define POSIX_STRING_H_

#include <mem.h>
#include <str.h>

/* available in str.h
 *
 * char *strtok(char *restrict, const char *restrict);
 * char *strtok_r(char *restrict, const char *restrict, char **restrict);
 *
 * available in mem.h
 *
 * void *memset(void *, int, size_t);
 * void *memcpy(void *, const void *, size_t);
 * void *memmove(void *, const void *, size_t);
 *
 * TODO: not implemented due to missing locale support
 *
 * int      strcoll_l(const char *, const char *, locale_t);
 * char    *strerror_l(int, locale_t);
 * size_t   strxfrm_l(char *restrict, const char *restrict, size_t, locale_t);
 */

#ifndef NULL
	#define NULL  ((void *) 0)
#endif

/* Copying and Concatenation */
extern char *posix_strcpy(char *restrict dest, const char *restrict src);
extern char *posix_strncpy(char *restrict dest, const char *restrict src, size_t n);
extern char *posix_stpcpy(char *restrict dest, const char *restrict src);
extern char *posix_stpncpy(char *restrict dest, const char *restrict src, size_t n);
extern char *posix_strcat(char *restrict dest, const char *restrict src);
extern char *posix_strncat(char *restrict dest, const char *restrict src, size_t n);
extern void *posix_memccpy(void *restrict dest, const void *restrict src, int c, size_t n);
extern char *posix_strdup(const char *s);
extern char *posix_strndup(const char *s, size_t n);

/* String/Array Comparison */
extern int posix_memcmp(const void *mem1, const void *mem2, size_t n);
extern int posix_strcmp(const char *s1, const char *s2);
extern int posix_strncmp(const char *s1, const char *s2, size_t n);

/* Search Functions */
extern void *posix_memchr(const void *mem, int c, size_t n);
extern char *posix_strchr(const char *s, int c);
extern char *posix_strrchr(const char *s, int c);
extern char *gnu_strchrnul(const char *s, int c);
extern char *posix_strpbrk(const char *s1, const char *s2);
extern size_t posix_strcspn(const char *s1, const char *s2);
extern size_t posix_strspn(const char *s1, const char *s2);
extern char *posix_strstr(const char *haystack, const char *needle);

/* Collation Functions */
extern int posix_strcoll(const char *s1, const char *s2);
extern size_t posix_strxfrm(char *restrict s1, const char *restrict s2, size_t n);

/* Error Messages */
extern char *posix_strerror(int errnum);
extern int posix_strerror_r(int errnum, char *buf, size_t bufsz);

/* String Length */
extern size_t posix_strlen(const char *s);
extern size_t posix_strnlen(const char *s, size_t n);

/* Signal Messages */
extern char *posix_strsignal(int signum);

/* Legacy Declarations */
#ifndef POSIX_STRINGS_H_
extern int posix_ffs(int i);
extern int posix_strcasecmp(const char *s1, const char *s2);
extern int posix_strncasecmp(const char *s1, const char *s2, size_t n);
#endif

#ifndef LIBPOSIX_INTERNAL
	#define strcpy posix_strcpy
	#define strncpy posix_strncpy
	#define stpcpy posix_stpcpy
	#define stpncpy posix_stpncpy
	#define strcat posix_strcat
	#define strncat posix_strncat
	#define memccpy posix_memccpy
	#define strdup posix_strdup
	#define strndup posix_strndup

	#define memcmp posix_memcmp
	#define strcmp posix_strcmp
	#define strncmp posix_strncmp

	#define memchr posix_memchr
	#define strchr posix_strchr
	#define strrchr posix_strrchr
	#define strchrnul gnu_strchrnul
	#define strpbrk posix_strpbrk
	#define strcspn posix_strcspn
	#define strspn posix_strspn
	#define strstr posix_strstr

	#define strcoll posix_strcoll
	#define strxfrm posix_strxfrm

	#define strerror posix_strerror
	#define strerror_r posix_strerror_r

	#define strlen posix_strlen
	#define strnlen posix_strnlen

	#define strsignal posix_strsignal

	#define ffs posix_ffs
	#define strcasecmp posix_strcasecmp
	#define strncasecmp posix_strncasecmp
#endif

#endif  // POSIX_STRING_H_

/** @}
 */
