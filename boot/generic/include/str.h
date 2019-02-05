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

/** @file
 */

#ifndef BOOT_STR_H_
#define BOOT_STR_H_

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>

/* Common Unicode characters */
#define U_SPECIAL      '?'

/** No size limit constant */
#define STR_NO_LIMIT  ((size_t) -1)

extern wchar_t str_decode(const char *str, size_t *offset, size_t sz);
extern errno_t chr_encode(wchar_t ch, char *str, size_t *offset, size_t sz);

extern size_t str_size(const char *str);
extern size_t str_lsize(const char *str, size_t max_len);
extern size_t str_length(const char *str);

extern bool ascii_check(wchar_t ch);
extern bool chr_check(wchar_t ch);

extern int str_cmp(const char *s1, const char *s2);
extern void str_cpy(char *dest, size_t size, const char *src);

#endif

/** @}
 */
