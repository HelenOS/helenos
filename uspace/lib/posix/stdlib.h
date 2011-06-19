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

#ifndef POSIX_STDLIB_H_
#define POSIX_STDLIB_H_

#include "libc/stdlib.h"

#ifndef NULL
	#define NULL  ((void *) 0)
#endif

/* Process Termination */
#undef EXIT_FAILURE
#define EXIT_FAILURE 1
#undef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#define _Exit exit

/* Array Sort Function */
extern void posix_qsort(void *array, size_t count, size_t size,
    int (*compare)(const void *, const void *));

/* Environment Access */
extern char *posix_getenv(const char *name);

/* Symbolic Links */
extern char *posix_realpath(const char *restrict name, char *restrict resolved);

/* Floating Point Conversion */
extern float posix_strtof(const char *restrict nptr, char **restrict endptr);
extern double posix_strtod(const char *restrict nptr, char **restrict endptr);
extern long double posix_strtold(const char *restrict nptr, char **restrict endptr);

/* Integer Conversion */
extern int posix_atoi(const char *str);

#ifndef LIBPOSIX_INTERNAL
	#define qsort posix_qsort
	#define getenv posix_getenv
	#define realpath posix_realpath
	
	#define strtof posix_strtof
	#define strtod posix_strtod
	#define strtold posix_strtold
	
	#define atoi posix_atoi
#endif

#endif  // POSIX_STDLIB_H_

/** @}
 */
