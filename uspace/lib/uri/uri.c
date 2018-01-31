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

/** @addtogroup uri
 * @{
 */
/**
 * @file
 */

#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <str.h>
#include <ctype.h>
#include <errno.h>

#include "uri.h"
#include "internal/ctype.h"

static char *cut_str(const char *start, const char *end)
{
	size_t size = end - start;
	return str_ndup(start, size);
}

uri_t *uri_parse(const char *str)
{
	uri_t *uri = malloc(sizeof(uri_t));
	if (uri == NULL)
		return NULL;
	memset(uri, 0, sizeof(uri_t));
	
	/* scheme ":" */
	const char *scheme = str;
	while (*str != 0 && *str != ':') str++;
	if (*str == 0) {
		uri_destroy(uri);
		return NULL;
	}
	uri->scheme = cut_str(scheme, str);
	
	/* skip the colon */
	str++;
	
	if (*str == '/' && str[1] == '/') {
		/* "//" [user-info [":" user-credential] "@"] host [":" port] */
		str++;
		str++;
		const char *authority_start = str;
	
		char *host_or_user_info = NULL;
		char *port_or_user_credential = NULL;
	
		while (*str != 0 && *str != '?' && *str != '#' && *str != '@'
			&& *str != ':' && *str != '/')
			str++;
	
		host_or_user_info = cut_str(authority_start, str);
		if (*str == ':') {
			str++;
			const char *second_part = str;
			while (*str != 0 && *str != '?' && *str != '#' &&
				*str != '@' && *str != '/') str++;
			port_or_user_credential = cut_str(second_part, str);
		}
	
		if (*str == '@') {
			uri->user_info = host_or_user_info;
			uri->user_credential = port_or_user_credential;
		
			str++;
			const char *host_start = str;
			while (*str != 0 && *str != '?' && *str != '#'
				&& *str != ':' && *str != '/') str++;
			uri->host = cut_str(host_start, str);
		
			if (*str == ':') {
				str++;
				const char *port_start = str;
				while (*str != 0 && *str != '?' && *str != '#' && *str != '/')
					str++;
				uri->port = cut_str(port_start, str);
			}
		}
		else {
			uri->host = host_or_user_info;
			uri->port = port_or_user_credential;
		}
	}
	
	const char *path_start = str;
	while (*str != 0 && *str != '?' && *str != '#') str++;
	uri->path = cut_str(path_start, str);
	
	if (*str == '?') {
		str++;
		const char *query_start = str;
		while (*str != 0 && *str != '#') str++;
		uri->query = cut_str(query_start, str);
	}
	
	if (*str == '#') {
		str++;
		const char *fragment_start = str;
		while (*str != 0) str++;
		uri->fragment = cut_str(fragment_start, str);
	}
	
	assert(*str == 0);
	return uri;
}

/** Parse the URI scheme
 *
 * @param endptr The position of the first character after the scheme
 *               is stored there.
 * @return EOK on success
 */
errno_t uri_scheme_parse(const char *str, const char **endptr)
{
	if (*str == 0) {
		*endptr = str;
		return ELIMIT;
	}
	
	if (!isalpha(*str)) {
		*endptr = str;
		return EINVAL;
	}
	
	while (isalpha(*str) || isdigit(*str) ||
	    *str == '+' || *str == '-' || *str == '.') {
		str++;
	}
	
	*endptr = str;
	return EOK;
}

/** Determine if the URI scheme is valid
 *
 */
bool uri_scheme_validate(const char *str)
{
	const char *endptr = NULL;
	if (uri_scheme_parse(str, &endptr) != EOK)
		return false;
	return *endptr == 0;
}

errno_t uri_percent_parse(const char *str, const char **endptr,
    uint8_t *decoded)
{
	*endptr = str;
	if (str[0] == 0 || str[1] == 0 || str[2] == 0)
		return ELIMIT;
	
	if (str[0] != '%' || !is_hexdig(str[1]) || !is_hexdig(str[2]))
		return EINVAL;
	
	if (decoded != NULL) {
		errno_t rc = str_uint8_t(str + 1, NULL, 16, true, decoded);
		if (rc != EOK)
			return rc;
	}
	
	*endptr = str + 3;
	return EOK;
}

errno_t uri_user_info_parse(const char *str, const char **endptr)
{
	while (*str != 0) {
		while (is_unreserved(*str) || is_subdelim(*str) || *str == ':') str++;
		if (*str == 0)
			break;
		errno_t rc = uri_percent_parse(str, &str, NULL);
		if (rc != EOK) {
			*endptr = str;
			return rc;
		}
	}
	
	*endptr = str;
	return EOK;
}

bool uri_user_info_validate(const char *str)
{
	const char *endptr = NULL;
	if (uri_user_info_parse(str, &endptr) != EOK)
		return false;
	return *endptr == 0;
}

errno_t uri_port_parse(const char *str, const char **endptr)
{
	if (*str == 0)
		return ELIMIT;
	if (!isdigit(*str))
		return EINVAL;
	while (isdigit(*str)) str++;
	*endptr = str;
	return EOK;
}

bool uri_port_validate(const char *str)
{
	const char *endptr = NULL;
	if (uri_port_parse(str, &endptr) != EOK)
		return false;
	return *endptr == 0;
}

bool uri_validate(uri_t *uri)
{
	if (uri->scheme && !uri_scheme_validate(uri->scheme))
		return false;
	
	if (uri->user_info && !uri_user_info_validate(uri->user_info))
		return false;
	
	if (uri->user_credential && !uri_user_info_validate(uri->user_credential))
		return false;
	
	if (uri->port && !uri_port_validate(uri->port))
		return false;
	
	return true;
}

void uri_destroy(uri_t *uri)
{
	free(uri->scheme);
	free(uri->user_info);
	free(uri->user_credential);
	free(uri->host);
	free(uri->port);
	free(uri->path);
	free(uri->query);
	free(uri->fragment);
	free(uri);
}

/** @}
 */
