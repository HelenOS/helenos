/*
 * Copyright (c) 2013 Martin Sucha
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

/** @addtogroup http
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <macros.h>

#include "http.h"

#define HTTP_HEADER_LINE "%s: %s\r\n"

static char *cut_str(const char *start, const char *end)
{
	size_t size = end - start;
	return str_ndup(start, size);
}

http_header_t *http_header_create(const char *name, const char *value)
{
	char *dname = str_dup(name);
	if (dname == NULL)
		return NULL;
	
	char *dvalue = str_dup(value);
	if (dvalue == NULL) {
		free(dname);
		return NULL;
	}

	return http_header_create_no_copy(dname, dvalue);
}

http_header_t *http_header_create_no_copy(char *name, char *value)
{
	http_header_t *header = malloc(sizeof(http_header_t));
	if (header == NULL)
		return NULL;
	
	link_initialize(&header->link);
	header->name = name;
	header->value = value;
	
	return header;
}

void http_header_destroy(http_header_t *header)
{
	free(header->name);
	free(header->value);
	free(header);
}

ssize_t http_encode_header(char *buf, size_t buf_size, http_header_t *header)
{
	/* TODO properly split long header values */
	if (buf == NULL) {
		return printf_size(HTTP_HEADER_LINE, header->name, header->value);
	}
	else {
		return snprintf(buf, buf_size,
		    HTTP_HEADER_LINE, header->name, header->value);
	}
}

int http_parse_header(const char *line, char **out_name, char **out_value)
{
	const char *pos = line;
	while (*pos != 0 && *pos != ':') pos++;
	if (*pos != ':')
		return EINVAL;
	
	char *name = cut_str(line, pos);
	if (name == NULL)
		return ENOMEM;
	
	pos++;
	
	while (*pos == ' ') pos++;
	
	char *value = str_dup(pos);
	if (value == NULL) {
		free(name);
		return ENOMEM;
	}
	
	*out_name = name;
	*out_value = value;
	
	return EOK;
}

/** @}
 */
