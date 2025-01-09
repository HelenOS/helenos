/*
 * Copyright (c) 2005 Martin Decky
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_STDLIB_H_
#define _LIBC_STDLIB_H_

#include <_bits/size_t.h>
#include <_bits/wchar_t.h>
#include <_bits/uchar.h>
#include <_bits/decls.h>
#include <bsearch.h>
#include <qsort.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define RAND_MAX  714025

#define MB_CUR_MAX 4

__C_DECLS_BEGIN;

/** Type returned by the div function */
typedef struct {
	/** Quotient */
	int quot;
	/** Remainder */
	int rem;
} div_t;

/** Type returned by the ldiv function */
typedef struct {
	/** Quotient */
	long quot;
	/** Remainder */
	long rem;
} ldiv_t;

/** Type returned by the lldiv function */
typedef struct {
	/** Quotient */
	long long quot;
	/** Remainder */
	long long rem;
} lldiv_t;

extern long double strtold(const char *, char **);

extern int rand(void);
extern void srand(unsigned int);

extern void abort(void) __attribute__((noreturn));
extern int atexit(void (*)(void));
extern void exit(int) __attribute__((noreturn));
extern void _Exit(int) __attribute__((noreturn));
extern int at_quick_exit(void (*)(void));
extern void quick_exit(int);

extern char *getenv(const char *);
extern int system(const char *);

extern int abs(int);
extern long labs(long);
extern long long llabs(long long);

extern int atoi(const char *);
extern long atol(const char *);
extern long long atoll(const char *);

extern long strtol(const char *__restrict__, char **__restrict__, int);
extern long long strtoll(const char *__restrict__, char **__restrict__, int);
extern unsigned long strtoul(const char *__restrict__, char **__restrict__, int);
extern unsigned long long strtoull(const char *__restrict__, char **__restrict__, int);

extern div_t div(int, int);
extern ldiv_t ldiv(long, long);
extern lldiv_t lldiv(long long, long long);

extern void *malloc(size_t size)
    __attribute__((malloc));
extern void *calloc(size_t nmemb, size_t size)
    __attribute__((malloc));
extern void *realloc(void *addr, size_t size)
    __attribute__((warn_unused_result));
extern void free(void *addr);

#ifdef _HELENOS_SOURCE
__HELENOS_DECLS_BEGIN;

extern void *memalign(size_t align, size_t size)
    __attribute__((malloc));

extern void *reallocarray(void *ptr, size_t nelem, size_t elsize)
    __attribute__((warn_unused_result));

__HELENOS_DECLS_END;
#endif

__C_DECLS_END;

#endif

/** @}
 */
