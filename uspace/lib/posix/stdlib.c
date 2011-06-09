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

#include "stdlib.h"
#include "common.h"

/**
 *
 * @param array
 * @param count
 * @param size
 * @param compare
 */
void posix_qsort(void *array, size_t count, size_t size, int (*compare)(const void *, const void *))
{
	// TODO
	not_implemented();
}

/**
 *
 * @param name
 * @return
 */
char *posix_getenv(const char *name)
{
	// TODO
	not_implemented();
}

/**
 * 
 * @param name
 * @param resolved
 * @return
 */
char *posix_realpath(const char *name, char *resolved)
{
	// TODO
	not_implemented();
}

/**
 * 
 * @param nptr
 * @param endptr
 * @return
 */
float posix_strtof(const char *restrict nptr, char **restrict endptr)
{
	// TODO
	not_implemented();
}

/**
 * 
 * @param nptr
 * @param endptr
 * @return
 */
double posix_strtod(const char *restrict nptr, char **restrict endptr)
{
	// TODO
	not_implemented();
}

/**
 * 
 * @param nptr
 * @param endptr
 * @return
 */
long double posix_strtold(const char *restrict nptr, char **restrict endptr)
{
	// TODO
	not_implemented();
}

/** @}
 */
