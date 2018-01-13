/*
 * Copyright (c) 2005 Martin Decky
 * Copyright (c) 2011 Oleg Romanenko
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef LIBC_STR_H_
#define LIBC_STR_H_

#include <errno.h>
#include <mem.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define U_SPECIAL  '?'

/** No size limit constant */
#define STR_NO_LIMIT  ((size_t) -1)

/** Maximum size of a string containing @c length characters */
#define STR_BOUNDS(length)  ((length) << 2)

/**
 * Maximum size of a buffer needed to a string converted from space-padded
 * ASCII of size @a spa_size using spascii_to_str().
 */
#define SPASCII_STR_BUFSIZE(spa_size) ((spa_size) + 1)

extern wchar_t str_decode(const char *str, size_t *offset, size_t sz);
extern wchar_t str_decode_reverse(const char *str, size_t *offset, size_t sz);
extern errno_t chr_encode(const wchar_t ch, char *str, size_t *offset, size_t sz);

extern size_t str_size(const char *str);
extern size_t wstr_size(const wchar_t *str);

extern size_t str_nsize(const char *str, size_t max_size);
extern size_t wstr_nsize(const wchar_t *str, size_t max_size);

extern size_t str_lsize(const char *str, size_t max_len);
extern size_t wstr_lsize(const wchar_t *str, size_t max_len);

extern size_t str_length(const char *str);
extern size_t wstr_length(const wchar_t *wstr);

extern size_t str_nlength(const char *str, size_t size);
extern size_t wstr_nlength(const wchar_t *str, size_t size);

extern size_t chr_width(wchar_t ch);
extern size_t str_width(const char *str);

extern bool ascii_check(wchar_t ch);
extern bool chr_check(wchar_t ch);

extern int str_cmp(const char *s1, const char *s2);
extern int str_lcmp(const char *s1, const char *s2, size_t max_len);
extern int str_casecmp(const char *s1, const char *s2);
extern int str_lcasecmp(const char *s1, const char *s2, size_t max_len);

extern bool str_test_prefix(const char *s, const char *p);

extern void str_cpy(char *dest, size_t size, const char *src);
extern void str_ncpy(char *dest, size_t size, const char *src, size_t n);
extern void str_append(char *dest, size_t size, const char *src);

extern errno_t spascii_to_str(char *dest, size_t size, const uint8_t *src, size_t n);
extern void wstr_to_str(char *dest, size_t size, const wchar_t *src);
extern char *wstr_to_astr(const wchar_t *src);
extern void str_to_wstr(wchar_t *dest, size_t dlen, const char *src);
extern wchar_t *str_to_awstr(const char *src);
extern errno_t utf16_to_str(char *dest, size_t size, const uint16_t *src);
extern errno_t str_to_utf16(uint16_t *dest, size_t dlen, const char *src);
extern size_t utf16_wsize(const uint16_t *ustr);

extern char *str_chr(const char *str, wchar_t ch);
extern char *str_rchr(const char *str, wchar_t ch);

extern void str_rtrim(char *str, wchar_t ch);
extern void str_ltrim(char *str, wchar_t ch);

extern bool wstr_linsert(wchar_t *str, wchar_t ch, size_t pos, size_t max_pos);
extern bool wstr_remove(wchar_t *str, size_t pos);

extern char *str_dup(const char *);
extern char *str_ndup(const char *, size_t max_size);

extern char *str_tok(char *, const char *, char **);

extern errno_t str_uint8_t(const char *, const char **, unsigned int, bool,
    uint8_t *);
extern errno_t str_uint16_t(const char *, const char **, unsigned int, bool,
    uint16_t *);
extern errno_t str_uint32_t(const char *, const char **, unsigned int, bool,
    uint32_t *);
extern errno_t str_uint64_t(const char *, const char **, unsigned int, bool,
    uint64_t *);
extern errno_t str_size_t(const char *, const char **, unsigned int, bool,
    size_t *);

extern void order_suffix(const uint64_t, uint64_t *, char *);
extern void bin_order_suffix(const uint64_t, uint64_t *, const char **, bool);

/*
 * TODO: Get rid of this.
 */

extern long int strtol(const char *, char **, int);
extern unsigned long strtoul(const char *, char **, int);

#endif

/** @}
 */
