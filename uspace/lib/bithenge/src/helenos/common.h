/*
 * SPDX-FileCopyrightText: 2012 Sean Bartell
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BITHENGE_HELENOS_COMMON_H_
#define BITHENGE_HELENOS_COMMON_H_

#include <bithenge/os.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <byteorder.h>
#include <errno.h>
#include <inttypes.h>
#include <macros.h>
#include <mem.h>
#include <str.h>
#include <str_error.h>

#define BITHENGE_PRId PRId64

typedef struct {
	const char *string;
	size_t offset;
	char32_t ch;
} string_iterator_t;

static inline string_iterator_t string_iterator(const char *string)
{
	string_iterator_t i;
	i.string = string;
	i.offset = 0;
	i.ch = str_decode(i.string, &i.offset, STR_NO_LIMIT);
	return i;
}

static inline bool string_iterator_done(const string_iterator_t *i)
{
	return i->ch == L'\0';
}

static inline errno_t string_iterator_next(string_iterator_t *i, char32_t *out)
{
	*out = i->ch;
	if (*out == U_SPECIAL)
		return EINVAL;
	i->ch = str_decode(i->string, &i->offset, STR_NO_LIMIT);
	return EOK;
}

static inline errno_t bithenge_parse_int(const char *start, bithenge_int_t *result)
{
	const char *real_start = *start == '-' ? start + 1 : start;
	uint64_t val;
	errno_t rc = str_uint64_t(real_start, NULL, 10, false, &val);
	*result = val;
	if (*start == '-')
		*result = -*result;
	return rc;
}

#endif
