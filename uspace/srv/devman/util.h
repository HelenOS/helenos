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
 
#ifndef DEVMAN_UTIL_H_
#define DEVMAN_UTIL_H_

#include <ctype.h>
#include <str.h>
#include <malloc.h>


char * get_abs_path(const char *base_path, const char *name, const char *ext);
char * get_path_elem_end(char *path);

static inline bool skip_spaces(char **buf) 
{
	while (isspace(**buf)) {
		(*buf)++;		
	}
	return *buf != 0;	
}

static inline size_t get_nonspace_len(const char *str) 
{
	size_t len = 0;
	while(*str != 0 && !isspace(*str)) {
		len++;
		str++;
	}
	return len;
}

static inline void free_not_null(const void *ptr)
{
	if (NULL != ptr) {
		free(ptr);
	}
}

static inline char * clone_string(const char *s) 
{
	size_t size = str_size(s) + 1;
	char *str = (char *)malloc(size);
	if (NULL != str) {
		str_cpy(str, size, s);
	}
	return str;
}

static inline void replace_char(char *str, char orig, char repl)
{
	while (*str) {
		if (orig == *str) {
			*str = repl;
		}
		str++;
	}
}

#endif