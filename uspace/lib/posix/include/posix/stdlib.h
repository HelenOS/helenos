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

#ifndef __POSIX_DEF__
#define __POSIX_DEF__(x) x
#endif

#include "sys/types.h"

#ifndef NULL
	#define NULL  ((void *) 0)
#endif

#define RAND_MAX  714025

/* Process Termination */
#undef EXIT_FAILURE
#define EXIT_FAILURE 1
#undef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#define _Exit exit
extern int __POSIX_DEF__(atexit)(void (*func)(void));
extern void exit(int status) __attribute__((noreturn));
extern void abort(void) __attribute__((noreturn));

/* Absolute Value */
extern int __POSIX_DEF__(abs)(int i);
extern long __POSIX_DEF__(labs)(long i);
extern long long __POSIX_DEF__(llabs)(long long i);

/* Integer Division */

typedef struct {
	int quot, rem;
} __POSIX_DEF__(div_t);

typedef struct {
	long quot, rem;
} __POSIX_DEF__(ldiv_t);

typedef struct {
	long long quot, rem;
} __POSIX_DEF__(lldiv_t);

extern __POSIX_DEF__(div_t) __POSIX_DEF__(div)(int numer, int denom);
extern __POSIX_DEF__(ldiv_t) __POSIX_DEF__(ldiv)(long numer, long denom);
extern __POSIX_DEF__(lldiv_t) __POSIX_DEF__(lldiv)(long long numer, long long denom);

/* Array Functions */
extern void __POSIX_DEF__(qsort)(void *array, size_t count, size_t size,
    int (*compare)(const void *, const void *));
extern void *__POSIX_DEF__(bsearch)(const void *key, const void *base,
    size_t nmemb, size_t size, int (*compar)(const void *, const void *));

/* Environment Access */
extern char *__POSIX_DEF__(getenv)(const char *name);
extern int __POSIX_DEF__(putenv)(char *string);
extern int __POSIX_DEF__(system)(const char *string);

/* Symbolic Links */
extern char *__POSIX_DEF__(realpath)(const char *restrict name, char *restrict resolved);

/* Floating Point Conversion */
extern double __POSIX_DEF__(atof)(const char *nptr);
extern float __POSIX_DEF__(strtof)(const char *restrict nptr, char **restrict endptr);
extern double __POSIX_DEF__(strtod)(const char *restrict nptr, char **restrict endptr);
extern long double __POSIX_DEF__(strtold)(const char *restrict nptr, char **restrict endptr);

/* Integer Conversion */
extern int __POSIX_DEF__(atoi)(const char *nptr);
extern long int __POSIX_DEF__(atol)(const char *nptr);
extern long long int __POSIX_DEF__(atoll)(const char *nptr);
extern long int __POSIX_DEF__(strtol)(const char *restrict nptr,
    char **restrict endptr, int base);
extern long long int __POSIX_DEF__(strtoll)(const char *restrict nptr,
    char **restrict endptr, int base);
extern unsigned long int __POSIX_DEF__(strtoul)(const char *restrict nptr,
    char **restrict endptr, int base);
extern unsigned long long int __POSIX_DEF__(strtoull)(
    const char *restrict nptr, char **restrict endptr, int base);

/* Memory Allocation */
extern void *__POSIX_DEF__(malloc)(size_t size);
extern void *__POSIX_DEF__(calloc)(size_t nelem, size_t elsize);
extern void *__POSIX_DEF__(realloc)(void *ptr, size_t size);
extern void __POSIX_DEF__(free)(void *ptr);

/* Temporary Files */
extern int __POSIX_DEF__(mkstemp)(char *tmpl);

/* Pseudo-random number generator */
extern int __POSIX_DEF__(rand)(void);
extern void __POSIX_DEF__(srand)(unsigned int seed);

/* Legacy Declarations */
extern char *__POSIX_DEF__(mktemp)(char *tmpl);
extern int bsd_getloadavg(double loadavg[], int nelem);

#endif  // POSIX_STDLIB_H_

/** @}
 */
