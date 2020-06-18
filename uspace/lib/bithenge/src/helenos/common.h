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
