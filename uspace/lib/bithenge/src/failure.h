/*
 * Copyright (c) 2012 Sean Bartell
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

