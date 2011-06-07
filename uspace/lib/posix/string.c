/*
 * Copyright (c) 2011 Petr Koupy
 * Copyright (c) 2011 Jiri Zarevucky
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

#include "string.h"

#include <libc/assert.h>

#include <str_error.h>

/**
 *
 * @param dest
 * @param src
 * @return
 */
char *posix_strcpy(char *dest, const char *src)
{
	// TODO
	return 0;
}

/**
 *
 * @param dest
 * @param src
 * @param n
 * @return
 */
char *posix_strncpy(char *dest, const char *src, size_t n)
{
	// TODO
	return 0;
}

/**
 *
 * @param dest
 * @param src
 * @return
 */
char *posix_strcat(char *dest, const char *src)
{
	// TODO
	return 0;
}

/**
 *
 * @param dest
 * @param src
 * @param n
 * @return
 */
char *posix_strncat(char *dest, const char *src, size_t n)
{
	// TODO
	return 0;
}

/**
 * 
 * @param dest
 * @param src
 * @param n
 * @return
 */
void *posix_mempcpy(void *dest, const void *src, size_t n)
{
	// TODO
	return 0;
}

/**
 * 
 * @param s
 * @return
 */
char *posix_strdup(const char *s)
{
	// TODO
	return 0;
}

/**
 *
 * @param mem1
 * @param mem2
 * @param n
 * @return
 */
int posix_memcmp(const void *mem1, const void *mem2, size_t n)
{
	// TODO
	return 0;
}

/**
 *
 * @param s1
 * @param s2
 * @return
 */
int posix_strcmp(const char *s1, const char *s2)
{
	// TODO
	return 0;
}

/**
 *
 * @param s1
 * @param s2
 * @param n
 * @return
 */
int posix_strncmp(const char *s1, const char *s2, size_t n)
{
	// TODO
	return 0;
}

/**
 * 
 * @param s1
 * @param s2
 * @return
 */
int posix_strcasecmp(const char *s1, const char *s2)
{
	// TODO
	return 0;
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
	// TODO
	return 0;
}

/**
 *
 * @param mem
 * @param c
 * @param n
 * @return
 */
void *posix_memchr(const void *mem, int c, size_t n)
{
	// TODO
	return 0;
}

/**
 * 
 * @param mem
 * @param c
 * @return
 */
void *posix_rawmemchr(const void *mem, int c)
{
	// TODO
	return 0;
}

/**
 *
 * @param s
 * @param c
 * @return
 */
char *posix_strchr(const char *s, int c)
{
	// TODO
	return 0;
}

/**
 *
 * @param s
 * @param c
 * @return
 */
char *posix_strrchr(const char *s, int c)
{
	// TODO
	return 0;
}

/**
 *
 * @param s1
 * @param s2
 * @return
 */
char *posix_strpbrk(const char *s1, const char *s2)
{
	// TODO
	return 0;
}

/**
 *
 * @param s1
 * @param s2
 * @return
 */
size_t posix_strcspn(const char *s1, const char *s2)
{
	// TODO
	return 0;
}

/**
 *
 * @param s1
 * @param s2
 * @return
 */
size_t posix_strspn(const char *s1, const char *s2)
{
	// TODO
	return 0;
}

/**
 *
 * @param s1
 * @param s2
 * @return
 */
char *posix_strstr(const char *s1, const char *s2)
{
	// TODO
	return 0;
}

/**
 *
 * @param s1
 * @param s2
 * @return
 */
int posix_strcoll(const char *s1, const char *s2)
{
	// TODO
	return 0;
}

/**
 *
 * @param s1
 * @param s2
 * @param n
 * @return
 */
size_t posix_strxfrm(char *s1, const char *s2, size_t n)
{
	// TODO
	return 0;
}

/**
 *
 * @param errnum
 * @return
 */
char *posix_strerror(int errnum)
{
	// TODO
	return 0;
}

/**
 *
 * @param s
 * @return
 */
size_t posix_strlen(const char *s)
{
	// TODO
	return 0;
}

/** @}
 */
