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
/** @file Standard library definitions.
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
extern int posix_atexit(void (*func)(void));

/* Absolute Value */
extern int posix_abs(int i);
extern long posix_labs(long i);
extern long long posix_llabs(long long i);

/* Integer Division */

typedef struct {
	int quot, rem;
} posix_div_t;

typedef struct {
	long quot, rem;
} posix_ldiv_t;

typedef struct {
	long long quot, rem;
} posix_lldiv_t;

extern posix_div_t posix_div(int numer, int denom);
extern posix_ldiv_t posix_ldiv(long numer, long denom);
extern posix_lldiv_t posix_lldiv(long long numer, long long denom);

/* Array Functions */
extern void posix_qsort(void *array, size_t count, size_t size,
    int (*compare)(const void *, const void *));
extern void *posix_bsearch(const void *key, const void *base,
    size_t nmemb, size_t size, int (*compar)(const void *, const void *));

/* Environment Access */
extern char *posix_getenv(const char *name);
extern int posix_putenv(char *string);
extern int posix_system(const char *string);

/* Symbolic Links */
extern char *posix_realpath(const char *restrict name, char *restrict resolved);

/* Floating Point Conversion */
extern double posix_atof(const char *nptr);
extern float posix_strtof(const char *restrict nptr, char **restrict endptr);
extern double posix_strtod(const char *restrict nptr, char **restrict endptr);
extern long double posix_strtold(const char *restrict nptr, char **restrict endptr);

/* Integer Conversion */
extern int posix_atoi(const char *nptr);
extern long int posix_atol(const char *nptr);
extern long long int posix_atoll(const char *nptr);
extern long int posix_strtol(const char *restrict nptr,
    char **restrict endptr, int base);
extern long long int posix_strtoll(const char *restrict nptr,
    char **restrict endptr, int base);
extern unsigned long int posix_strtoul(const char *restrict nptr,
    char **restrict endptr, int base);
extern unsigned long long int posix_strtoull(
    const char *restrict nptr, char **restrict endptr, int base);

/* Memory Allocation */
extern void *posix_malloc(size_t size);
extern void *posix_calloc(size_t nelem, size_t elsize);
extern void *posix_realloc(void *ptr, size_t size);
extern void posix_free(void *ptr);

/* Temporary Files */
extern int posix_mkstemp(char *tmpl);

/* Legacy Declarations */
extern char *posix_mktemp(char *tmpl);
extern int bsd_getloadavg(double loadavg[], int nelem);

#ifndef LIBPOSIX_INTERNAL
	#define atexit posix_atexit

	#define abs posix_abs
	#define labs posix_labs
	#define llabs posix_llabs

	#define div_t posix_div_t
	#define ldiv_t posix_ldiv_t
	#define lldiv_t posix_lldiv_t
	#define div posix_div
	#define ldiv posix_ldiv
	#define lldiv posix_lldiv

	#define qsort posix_qsort
	#define bsearch posix_bsearch

	#define getenv posix_getenv
	#define putenv posix_putenv
	#define system posix_system

	#define realpath posix_realpath
	
	#define atof posix_atof
	#define strtof posix_strtof
	#define strtod posix_strtod
	#define strtold posix_strtold
	
	#define atoi posix_atoi
	#define atol posix_atol
	#define atoll posix_atoll
	#define strtol posix_strtol
	#define strtoll posix_strtoll
	#define strtoul posix_strtoul
	#define strtoull posix_strtoull

	#define malloc posix_malloc
	#define calloc posix_calloc
	#define realloc posix_realloc
	#define free posix_free

	#define mkstemp posix_mkstemp

	#define mktemp posix_mktemp
	#define getloadavg bsd_getloadavg
#endif

#endif  // POSIX_STDLIB_H_

/** @}
 */
