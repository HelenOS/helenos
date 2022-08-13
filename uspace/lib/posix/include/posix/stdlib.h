/*
 * SPDX-FileCopyrightText: 2011 Petr Koupy
 * SPDX-FileCopyrightText: 2011 Jiri Zarevucky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libposix
 * @{
 */
/** @file Standard library definitions.
 */

#ifndef POSIX_STDLIB_H_
#define POSIX_STDLIB_H_

#include <libc/stdlib.h>
#include <sys/types.h>

#include <stddef.h>

__C_DECLS_BEGIN;

/* Environment Access */
extern int putenv(char *string);

/* Symbolic Links */
extern char *realpath(const char *__restrict__ name, char *__restrict__ resolved);

/* Floating Point Conversion */
extern double atof(const char *nptr);
extern float strtof(const char *__restrict__ nptr, char **__restrict__ endptr);
extern double strtod(const char *__restrict__ nptr, char **__restrict__ endptr);

/* Temporary Files */
extern int mkstemp(char *tmpl);

/* Legacy Declarations */
extern char *mktemp(char *tmpl) __attribute__((deprecated));
extern int bsd_getloadavg(double loadavg[], int nelem);

__C_DECLS_END;

#endif  // POSIX_STDLIB_H_

/** @}
 */
