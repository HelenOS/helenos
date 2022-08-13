/*
 * SPDX-FileCopyrightText: 2011 Jiri Zarevucky
 * SPDX-FileCopyrightText: 2011 Petr Koupy
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libposix
 * @{
 */
/** @file Additional string manipulation.
 */

#ifndef POSIX_STRINGS_H_
#define POSIX_STRINGS_H_

#include <types/common.h>

__C_DECLS_BEGIN;

/* Search Functions */
extern int ffs(int i);

/* String/Array Comparison */
extern int strcasecmp(const char *s1, const char *s2);
extern int strncasecmp(const char *s1, const char *s2, size_t n);

/*
 * TODO: not implemented due to missing locale support
 *
 * int strcasecmp_l(const char *, const char *, locale_t);
 * int strncasecmp_l(const char *, const char *, size_t, locale_t);
 */

/* Legacy Functions */
extern int bcmp(const void *mem1, const void *mem2, size_t n);
extern void bcopy(const void *src, void *dest, size_t n);
extern void bzero(void *mem, size_t n);
extern char *index(const char *s, int c);
extern char *rindex(const char *s, int c);

__C_DECLS_END;

#endif  // POSIX_STRINGS_H_

/** @}
 */
