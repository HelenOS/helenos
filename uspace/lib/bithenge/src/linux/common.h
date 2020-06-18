/*
 * Copyright (c) 2012 Sean Bartell
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

#ifndef BITHENGE_LINUX_COMMON_H_
#define BITHENGE_LINUX_COMMON_H_

#include <endian.h>
#include <errno.h>
#include <inttypes.h>
#include <memory.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>

#define max(aleph, bet) ((aleph) > (bet) ? (aleph) : (bet))
#define min(aleph, bet) ((aleph) < (bet) ? (aleph) : (bet))

#define EOK 0
#define ELIMIT EINVAL

typedef intmax_t bithenge_int_t;
#define BITHENGE_PRId PRIdMAX
typedef uint64_t aoff64_t;
typedef const char *string_iterator_t;

static inline string_iterator_t string_iterator(const char *string)
{
	return string;
}

static inline errno_t string_iterator_next(string_iterator_t *i, char32_t *out)
{
	wint_t rc = btowc(*(*i)++); // TODO
	*out = (char32_t) rc;
	return rc == WEOF ? EILSEQ : EOK;
}

static inline bool string_iterator_done(const string_iterator_t *i)
{
	return !**i;
}

static inline size_t str_length(const char *string)
{
	return strlen(string);
}

static inline const char *str_chr(const char *string, char32_t ch)
{
	return strchr(string, wctob(ch)); // TODO
}

static inline int str_cmp(const char *s1, const char *s2)
{
	return strcmp(s1, s2);
}

static inline errno_t str_lcmp(const char *s1, const char *s2, size_t max_len)
{
	return strncmp(s1, s2, max_len);
}

static inline char *str_dup(const char *s)
{
	return strdup(s);
}

static inline char *str_ndup(const char *s, size_t max_len)
{
	return strndup(s, max_len);
}

static inline const char *str_error(int e)
{
	return strerror(e);
}

static inline uint16_t uint16_t_le2host(uint16_t val)
{
	return le16toh(val);
}

static inline uint16_t uint16_t_be2host(uint16_t val)
{
	return be16toh(val);
}

static inline uint32_t uint32_t_le2host(uint32_t val)
{
	return le32toh(val);
}

static inline uint32_t uint32_t_be2host(uint32_t val)
{
	return be32toh(val);
}

static inline uint64_t uint64_t_le2host(uint64_t val)
{
	return le64toh(val);
}

static inline uint64_t uint64_t_be2host(uint64_t val)
{
	return be64toh(val);
}

static inline errno_t bithenge_parse_int(const char *start, bithenge_int_t *result)
{
	errno = 0;
	*result = strtoll(start, NULL, 10);
	return errno;
}

#endif
