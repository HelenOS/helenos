/*
 * Copyright (c) 2013 Vojtech Horky
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
