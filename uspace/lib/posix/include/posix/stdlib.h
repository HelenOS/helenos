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

#include "sys/types.h"

#include <_bits/NULL.h>

#define RAND_MAX  714025

/* Process Termination */
#undef EXIT_FAILURE
#define EXIT_FAILURE 1
#undef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#define _Exit exit
extern int atexit(void (*func)(void));
extern void exit(int status) __attribute__((noreturn));
extern void abort(void) __attribute__((noreturn));

/* Absolute Value */
extern int abs(int i);
extern long labs(long i);
extern long long llabs(long long i);

/* Integer Division */

typedef struct {
	int quot, rem;
} div_t;

typedef struct {
	long quot, rem;
} ldiv_t;

typedef struct {
	long long quot, rem;
} lldiv_t;

extern div_t div(int numer, int denom);
extern ldiv_t ldiv(long numer, long denom);
extern lldiv_t lldiv(long long numer, long long denom);

/* Array Functions */
extern void qsort(void *array, size_t count, size_t size,
    int (*compare)(const void *, const void *));
extern void *bsearch(const void *key, const void *base,
    size_t nmemb, size_t size, int (*compar)(const void *, const void *));

/* Environment Access */
extern char *getenv(const char *name);
extern int putenv(char *string);
extern int system(const char *string);

/* Symbolic Links */
extern char *realpath(const char *__restrict__ name, char *__restrict__ resolved);

/* Floating Point Conversion */
extern double atof(const char *nptr);
extern float strtof(const char *__restrict__ nptr, char **__restrict__ endptr);
extern double strtod(const char *__restrict__ nptr, char **__restrict__ endptr);
extern long double strtold(const char *__restrict__ nptr, char **__restrict__ endptr);

/* Integer Conversion */
extern int atoi(const char *nptr);
extern long int atol(const char *nptr);
extern long long int atoll(const char *nptr);
extern long int strtol(const char *__restrict__ nptr,
    char **__restrict__ endptr, int base);
extern long long int strtoll(const char *__restrict__ nptr,
    char **__restrict__ endptr, int base);
extern unsigned long int strtoul(const char *__restrict__ nptr,
    char **__restrict__ endptr, int base);
extern unsigned long long int strtoull(
    const char *__restrict__ nptr, char **__restrict__ endptr, int base);

/* Memory Allocation */
extern void *malloc(size_t size)
    __attribute__((malloc));
extern void *calloc(size_t nelem, size_t elsize)
    __attribute__((malloc));
extern void *realloc(void *ptr, size_t size)
    __attribute__((warn_unused_result));
extern void free(void *ptr);

/* Temporary Files */
extern int mkstemp(char *tmpl);

/* Pseudo-random number generator */
extern int rand(void);
extern void srand(unsigned int seed);

/* Legacy Declarations */
extern char *mktemp(char *tmpl);
extern int bsd_getloadavg(double loadavg[], int nelem);

#endif  // POSIX_STDLIB_H_

/** @}
 */
