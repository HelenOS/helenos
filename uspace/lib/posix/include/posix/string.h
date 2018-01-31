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

#include "sys/types.h"

/*
 * TODO: not implemented due to missing locale support
 *
 * int      strcoll_l(const char *, const char *, locale_t);
 * char    *strerror_l(int, locale_t);
 * size_t   strxfrm_l(char *__restrict__, const char *__restrict__, size_t, locale_t);
 */

#include <_bits/NULL.h>

/*
 * These are the same as in HelenOS libc.
 * It would be possible to directly include <str.h> and <mem.h> but
 * it is better not to pollute POSIX namespace with other functions
 * defined in that header.
 *
 * Because libposix is always linked with libc, providing only these
 * forward declarations ought to be enough.
 */

/* From mem.h */
// #define bzero(ptr, len)  memset((ptr), 0, (len))
extern void *memset(void *, int, size_t)
    __attribute__((nonnull(1)));
extern void *memcpy(void *, const void *, size_t)
    __attribute__((nonnull(1, 2)));
extern void *memmove(void *, const void *, size_t)
    __attribute__((nonnull(1, 2)));


/* Copying and Concatenation */
extern char *strcpy(char *__restrict__ dest, const char *__restrict__ src);
extern char *strncpy(char *__restrict__ dest, const char *__restrict__ src, size_t n);
extern char *stpcpy(char *__restrict__ dest, const char *__restrict__ src);
extern char *stpncpy(char *__restrict__ dest, const char *__restrict__ src, size_t n);
extern char *strcat(char *__restrict__ dest, const char *__restrict__ src);
extern char *strncat(char *__restrict__ dest, const char *__restrict__ src, size_t n);
extern void *memccpy(void *__restrict__ dest, const void *__restrict__ src, int c, size_t n);
extern char *strdup(const char *s);
extern char *strndup(const char *s, size_t n);

/* String/Array Comparison */
extern int memcmp(const void *mem1, const void *mem2, size_t n);
extern int strcmp(const char *s1, const char *s2);
extern int strncmp(const char *s1, const char *s2, size_t n);

/* Search Functions */
extern void *memchr(const void *mem, int c, size_t n);
extern char *strchr(const char *s, int c);
extern char *strrchr(const char *s, int c);
extern char *gnu_strchrnul(const char *s, int c);
extern char *strpbrk(const char *s1, const char *s2);
extern size_t strcspn(const char *s1, const char *s2);
extern size_t strspn(const char *s1, const char *s2);
extern char *strstr(const char *haystack, const char *needle);

/* Tokenization functions. */
extern char *strtok_r(char *, const char *, char **);
extern char *strtok(char *, const char *);


/* Collation Functions */
extern int strcoll(const char *s1, const char *s2);
extern size_t strxfrm(char *__restrict__ s1, const char *__restrict__ s2, size_t n);

/* Error Messages */
extern char *strerror(int errnum);
extern int strerror_r(int errnum, char *buf, size_t bufsz);

/* String Length */
extern size_t strlen(const char *s);
extern size_t strnlen(const char *s, size_t n);

/* Signal Messages */
extern char *strsignal(int signum);

/* Legacy Declarations */
#ifndef POSIX_STRINGS_H_
extern int ffs(int i);
extern int strcasecmp(const char *s1, const char *s2);
extern int strncasecmp(const char *s1, const char *s2, size_t n);
#endif


#endif  // POSIX_STRING_H_

/** @}
 */
