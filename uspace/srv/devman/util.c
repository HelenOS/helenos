/*
 * Copyright (c) 2010 Lenka Trochtova
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

/** @addtogroup devman
 * @{
 */

#include <stdlib.h>
#include <str.h>

#include "util.h"

char *get_abs_path(const char *base_path, const char *name, const char *ext)
{
	char *res;
	int base_len = str_size(base_path);
	int size = base_len + 2 * str_size(name) + str_size(ext) + 3;

	res = malloc(size);

	if (res) {
		str_cpy(res, size, base_path);
		if (base_path[base_len - 1] != '/')
			str_append(res, size, "/");
		str_append(res, size, name);
		str_append(res, size, "/");
		str_append(res, size, name);
		if (ext[0] != '.')
			str_append(res, size, ".");
		str_append(res, size, ext);
	}

	return res;
}

char *get_path_elem_end(char *path)
{
	while (*path != '\0' && *path != '/')
		path++;
	return path;
}

bool skip_spaces(char **buf)
{
	while (isspace(**buf))
		(*buf)++;
	return *buf != 0;
}

void skip_line(char **buf)
{
	while (**buf && **buf != '\n')
		(*buf)++;
}

size_t get_nonspace_len(const char *str)
{
	size_t len = 0;

	while (*str != '\0' && !isspace(*str)) {
		len++;
		str++;
	}

	return len;
}

void replace_char(char *str, char orig, char repl)
{
	while (*str) {
		if (*str == orig)
			*str = repl;
		str++;
	}
}

/** @}
 */
