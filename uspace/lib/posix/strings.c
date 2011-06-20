/*
 * Copyright (c) 2011 Jiri Zarevucky
 * Copyright (c) 2011 Petr Koupy
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

/** @addtogroup libposix
 * @{
 */
/** @file
 */

#define LIBPOSIX_INTERNAL

#include "internal/common.h"
#include "strings.h"
#include "string.h"
#include "ctype.h"

/**
 *
 * @param i
 * @return
 */
int posix_ffs(int i)
{
	// TODO
	not_implemented();
}

/**
 *
 * @param s1
 * @param s2
 * @return
 */
int posix_strcasecmp(const char *s1, const char *s2)
{
	return posix_strncasecmp(s1, s2, STR_NO_LIMIT);
}

/**
 *
 * @param s1
 * @param s2
 * @param n
 * @return
 */
int posix_strncasecmp(const char *s1, const char *s2, size_t n)
{
	for (size_t i = 0; i < n; ++i) {
		int cmp = tolower(s1[i]) - tolower(s2[i]);
		if (cmp != 0) {
			return cmp;
		}
		
		if (s1[i] == 0) {
			return 0;
		}
	}
	
	return 0;
}

/**
 *
 * @param mem1
 * @param mem2
 * @param n
 * @return
 */
int posix_bcmp(const void *mem1, const void *mem2, size_t n)
{
	// TODO
	not_implemented();
}

/**
 *
 * @param dest
 * @param src
 * @param n
 */
void posix_bcopy(const void *dest, void *src, size_t n)
{
	// TODO
	not_implemented();
}

/**
 *
 * @param mem
 * @param n
 */
void posix_bzero(void *mem, size_t n)
{
	// TODO
	not_implemented();
}

/**
 *
 * @param s
 * @param c
 * @return
 */
char *posix_index(const char *s, int c)
{
	return posix_strchr(s, c);
}

/**
 * 
 * @param s
 * @param c
 * @return
 */
char *posix_rindex(const char *s, int c)
{
	return posix_strrchr(s, c);
}

/** @}
 */
