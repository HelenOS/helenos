/*
 * Copyright (c) 2012-2013 Dominik Taborsky
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

/** @addtogroup hdisk
 * @{
 */
/** @file
 */

#include <str.h>
#include <errno.h>
#include <stdlib.h>
#include "input.h"

typedef int (*conv_f)(const char *, char **, unsigned int, bool, void *);

static int convert(tinput_t *, conv_f, void *);

int get_input_line(tinput_t *in, char **str)
{
	int rc = tinput_read(in, str);
	if (rc == ENOENT)
		return EINTR;
	
	if (rc != EOK)
		return rc;
	
	/* Check for empty input. */
	if (str_cmp(*str, "") == 0) {
		free(*str);
		*str = NULL;
		return EINVAL;
	}
	
	return EOK;
}

uint8_t get_input_uint8(tinput_t *in)
{
	uint32_t val;
	int rc = convert(in, (conv_f) str_uint8_t, &val);
	if (rc != EOK) {
		errno = rc;
		return 0;
	}
	
	errno = EOK;
	return val;
}

uint32_t get_input_uint32(tinput_t *in)
{
	uint32_t val;
	int rc = convert(in, (conv_f) str_uint32_t, &val);
	if (rc != EOK) {
		errno = rc;
		return 0;
	}
	
	errno = EOK;
	return val;
}

uint64_t get_input_uint64(tinput_t *in)
{
	uint64_t val;
	int rc = convert(in, (conv_f) str_uint64_t, &val);
	if (rc != EOK) {
		errno = rc;
		return 0;
	}
	
	errno = EOK;
	return val;
}

size_t get_input_size_t(tinput_t *in)
{
	size_t val;
	int rc = convert(in, (conv_f) str_size_t, &val);
	if (rc != EOK) {
		errno = rc;
		return 0;
	}
	
	errno = EOK;
	return val;
}

static int convert(tinput_t *in, conv_f str_f, void *val)
{
	char *str;
	int rc = get_input_line(in, &str);
	if (rc != EOK) {
		printf("Error reading input.\n");
		return rc;
	}
	
	rc = str_f(str, NULL, 10, true, val);
	if (rc != EOK)
		printf("Invalid value.\n");
	
	free(str);
	return rc;
}
