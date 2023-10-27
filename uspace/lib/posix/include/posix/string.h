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
/*
 * TODO: not implemented due to missing locale support
 *
 * int      strcoll_l(const char *, const char *, locale_t);
 * char    *strerror_l(int, locale_t);
 * size_t   strxfrm_l(char *__restrict__, const char *__restrict__, size_t, locale_t);
 */

#include <stddef.h>

#include <common/mem.h>

#define _REALLY_WANT_STRING_H
#include <libc/string.h>

__C_DECLS_BEGIN;

/* Copying and Concatenation */
extern char *stpcpy(char *__restrict__ dest, const char *__restrict__ src);
extern char *stpncpy(char *__restrict__ dest, const char *__restrict__ src, size_t n);
extern void *memccpy(void *__restrict__ dest, const void *__restrict__ src, int c, size_t n);

/* Search Functions */
extern char *gnu_strchrnul(const char *s, int c);

/* Tokenization functions. */
extern char *strtok_r(char *, const char *, char **);

/* Error Messages */
extern int strerror_r(int errnum, char *buf, size_t bufsz);

/* Signal Messages */
extern char *strsignal(int signum);

/* Legacy Declarations */
extern int ffs(int i);
extern int strcasecmp(const char *s1, const char *s2);
extern int strncasecmp(const char *s1, const char *s2, size_t n);

__C_DECLS_END;

#endif  // POSIX_STRING_H_

/** @}
 */
