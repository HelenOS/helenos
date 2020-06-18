/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_STR_H_
#define KERN_STR_H_

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Common Unicode characters */
#define U_SPECIAL  '?'

#define U_LEFT_ARROW   0x2190
#define U_UP_ARROW     0x2191
#define U_RIGHT_ARROW  0x2192
#define U_DOWN_ARROW   0x2193

#define U_PAGE_UP      0x21de
#define U_PAGE_DOWN    0x21df

#define U_HOME_ARROW   0x21f1
#define U_END_ARROW    0x21f2

#define U_NULL         0x2400
#define U_ESCAPE       0x241b
#define U_DELETE       0x2421

#define U_CURSOR       0x2588

/** No size limit constant */
#define STR_NO_LIMIT  ((size_t) -1)

/** Maximum size of a string containing @c length characters */
#define STR_BOUNDS(length)  ((length) << 2)

extern char32_t str_decode(const char *, size_t *, size_t);
extern errno_t chr_encode(char32_t, char *, size_t *, size_t);

extern size_t str_size(const char *);
extern size_t wstr_size(const char32_t *);

extern size_t str_lsize(const char *, size_t);
extern size_t wstr_lsize(const char32_t *, size_t);

extern size_t str_length(const char *);
extern size_t wstr_length(const char32_t *);

extern size_t str_nlength(const char *, size_t);
extern size_t wstr_nlength(const char32_t *, size_t);

extern bool ascii_check(char32_t);
extern bool chr_check(char32_t);

extern int str_cmp(const char *, const char *);
extern int str_lcmp(const char *, const char *, size_t);

extern void str_cpy(char *, size_t, const char *);
extern void str_ncpy(char *, size_t, const char *, size_t);
extern void wstr_to_str(char *, size_t, const char32_t *);

extern char *str_chr(const char *, char32_t);

extern bool wstr_linsert(char32_t *, char32_t, size_t, size_t);
extern bool wstr_remove(char32_t *, size_t);

extern char *str_dup(const char *);
extern char *str_ndup(const char *, size_t);

extern errno_t str_uint64_t(const char *, char **, unsigned int, bool,
    uint64_t *);

extern void order_suffix(const uint64_t, uint64_t *, char *);
extern void bin_order_suffix(const uint64_t, uint64_t *, const char **, bool);

extern const char *str_error(errno_t);
extern const char *str_error_name(errno_t);

#endif

/** @}
 */
