/*
 * SPDX-FileCopyrightText: 2012 Sean Bartell
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
