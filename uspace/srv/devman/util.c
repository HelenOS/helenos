/*
 * SPDX-FileCopyrightText: 2010 Lenka Trochtova
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
