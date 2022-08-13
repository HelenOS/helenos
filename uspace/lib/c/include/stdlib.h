/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
#include <malloc.h>
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

__C_DECLS_END;

#endif

/** @}
 */
