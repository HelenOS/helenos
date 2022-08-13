/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 * SPDX-FileCopyrightText: 2005 Martin Decky
 * SPDX-FileCopyrightText: 2011 Oleg Romanenko
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 */

#ifndef BOOT_STR_H_
#define BOOT_STR_H_

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <uchar.h>

/* Common Unicode characters */
#define U_SPECIAL  '?'

/** No size limit constant */
#define STR_NO_LIMIT  ((size_t) -1)

extern char32_t str_decode(const char *str, size_t *offset, size_t sz);
extern errno_t chr_encode(char32_t ch, char *str, size_t *offset, size_t sz);

extern size_t str_size(const char *str);
extern size_t str_lsize(const char *str, size_t max_len);
extern size_t str_length(const char *str);

extern bool ascii_check(char32_t ch);
extern bool chr_check(char32_t ch);

extern int str_cmp(const char *s1, const char *s2);
extern void str_cpy(char *dest, size_t size, const char *src);

#endif

/** @}
 */
