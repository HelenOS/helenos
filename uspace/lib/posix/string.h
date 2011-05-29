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

#ifndef POSIX_STRING_H_
#define POSIX_STRING_H_

#include <mem.h>
#include <str.h>

#ifndef NULL
	#define NULL  ((void *) 0)
#endif

/* Copying and Concatenation */
extern char *posix_strcpy(char *restrict dest, const char *restrict src);
extern char *posix_strncpy(char *restrict dest, const char *restrict src, size_t n);
extern char *posix_strcat(char *restrict dest, const char *restrict src);
extern char *posix_strncat(char *restrict dest, const char *restrict src, size_t n);
extern void *posix_mempcpy(void *restrict dest, const void *restrict src, size_t n);
extern char *posix_strdup(const char *s);

/* String/Array Comparison */
extern int posix_memcmp(const void *mem1, const void *mem2, size_t n);
extern int posix_strcmp(const char *s1, const char *s2);
extern int posix_strncmp(const char *s1, const char *s2, size_t n);
extern int posix_strcasecmp(const char *s1, const char *s2);
extern int posix_strncasecmp(const char *s1, const char *s2, size_t n);

/* Search Functions */
extern void *posix_memchr(const void *mem, int c, size_t n);
extern void *posix_rawmemchr(const void *mem, int c);
extern char *posix_strchr(const char *s, int c);
extern char *posix_strrchr(const char *s, int c);
extern char *posix_strpbrk(const char *s1, const char *s2);
extern size_t posix_strcspn(const char *s1, const char *s2);
extern size_t posix_strspn(const char *s1, const char *s2);
extern char *posix_strstr(const char *s1, const char *s2);

/* Collation Functions */
extern int posix_strcoll(const char *s1, const char *s2);
extern size_t posix_strxfrm(char *restrict s1, const char *restrict s2, size_t n);

/* Error Messages */
extern char *posix_strerror(int errnum);

/* String Length */
extern size_t posix_strlen(const char *s);

#define strcpy posix_strcpy
#define strncpy posix_strncpy
#define strcat posix_strcat
#define strncat posix_strncat
#define mempcpy posix_mempcpy
#define strdup posix_strdup

#define memcmp posix_memcmp
#define strcmp posix_strcmp
#define strncmp posix_strncmp
#define strcasecmp posix_strcasecmp
#define strncasecmp posix_strncasecmp

#define memchr posix_memchr
#define rawmemchr posix_rawmemchr
#define strchr posix_strchr
#define strrchr posix_strrchr
#define strpbrk posix_strpbrk
#define strcspn posix_strcspn
#define strspn posix_strspn
#define strstr posix_strstr

#define strcoll posix_strcoll
#define strxfrm posix_strxfrm

#define strerror posix_strerror

#define strlen posix_strlen

#endif  // POSIX_STRING_H_

/** @}
 */
