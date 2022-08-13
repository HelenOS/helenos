/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_STRING_H_
#define _LIBC_STRING_H_

#if defined(_HELENOS_SOURCE) && !defined(_REALLY_WANT_STRING_H) && \
    !defined(_LIBC_SOURCE)
#error Please use str.h and mem.h instead
#endif

#include <_bits/decls.h>
#include <_bits/size_t.h>
#include <_bits/NULL.h>
#include <mem.h>

__C_DECLS_BEGIN;

extern char *strcpy(char *, const char *);
extern char *strncpy(char *, const char *, size_t);
extern char *strcat(char *, const char *);
extern char *strncat(char *, const char *, size_t);
extern int strcmp(const char *, const char *);
extern int strcoll(const char *, const char *);
extern int strncmp(const char *, const char *, size_t);
extern size_t strxfrm(char *, const char *, size_t);
extern char *strchr(const char *, int);
extern size_t strcspn(const char *, const char *);
extern char *strpbrk(const char *, const char *);
extern char *strrchr(const char *, int);
extern size_t strspn(const char *, const char *);
extern char *strstr(const char *, const char *);
extern char *strtok(char *, const char *);
extern char *__strtok_r(char *, const char *, char **);
extern char *strerror(int);
extern size_t strlen(const char *);

#if defined(_HELENOS_SOURCE) || defined(_XOPEN_SOURCE) || defined(_GNU_SOURCE) || defined(_LIBC_SOURCE)
extern size_t strnlen(const char *, size_t);
extern char *strdup(const char *);
extern char *strndup(const char *, size_t);
#endif

__C_DECLS_END;

#endif

/** @}
 */
