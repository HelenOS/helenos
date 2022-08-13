/*
 * SPDX-FileCopyrightText: 2013 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 *
 * String operation functions for systems with a standard C library.
 */

#pragma warning(push, 0)
#include <string.h>
#pragma warning(pop)

#include "../internal.h"

int pcut_str_equals(const char *a, const char *b) {
	return strcmp(a, b) == 0;
}

int pcut_str_start_equals(const char *a, const char *b, int len) {
	return strncmp(a, b, len) == 0;
}

int pcut_str_size(const char *s) {
	return strlen(s);
}

int pcut_str_to_int(const char *s) {
	return atoi(s);
}

char *pcut_str_find_char(const char *haystack, const char needle) {
	return strchr(haystack, needle);
}

void pcut_str_error(int error, char *buffer, int size) {
	const char *str = strerror(error);
	if (str == NULL) {
		str = "(strerror failure)";
	}
	strncpy(buffer, str, size - 1);
	/* Ensure correct termination. */
	buffer[size - 1] = 0;
}
