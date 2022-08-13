/*
 * SPDX-FileCopyrightText: 2011 Petr Koupy
 * SPDX-FileCopyrightText: 2011 Jiri Zarevucky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

#include <libc/mem.h>

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
