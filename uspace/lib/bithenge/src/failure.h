/*
 * SPDX-FileCopyrightText: 2012 Sean Bartell
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @cond internal */
/** @addtogroup bithenge
 * @{
 */
/**
 * @file
 * Fake system call errors for testing.
 */

#ifndef BITHENGE_FAILURE_H_
#define BITHENGE_FAILURE_H_

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

errno_t bithenge_should_fail(void);
void *bithenge_failure_malloc(size_t);
void *bithenge_failure_realloc(void *, size_t);
ssize_t bithenge_failure_read(int, void *, size_t);
off_t bithenge_failure_lseek(int, off_t, int);
errno_t bithenge_failure_ferror(FILE *);
char *bithenge_failure_str_ndup(const char *, size_t);
errno_t bithenge_failure_open(const char *, int);
errno_t bithenge_failure_fstat(int, vfs_stat_t *);

#ifndef BITHENGE_FAILURE_DECLS_ONLY
#define malloc bithenge_failure_malloc
#define realloc bithenge_failure_realloc
#define read bithenge_failure_read
#define lseek bithenge_failure_lseek
#define ferror bithenge_failure_ferror
#define str_ndup bithenge_failure_str_ndup
#define open bithenge_failure_open
#define fstat bithenge_failure_fstat
#endif

#endif

/** @}
 */

/** @endcond */
