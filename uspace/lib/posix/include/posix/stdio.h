/*
 * Copyright (c) 2011 Jiri Zarevucky
 * Copyright (c) 2011 Petr Koupy
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
/** @file Standard buffered input/output.
 */

#ifndef POSIX_STDIO_H_
#define POSIX_STDIO_H_

#include <libc/stdio.h>

#include <stddef.h>
#include <io/verify.h>
#include <sys/types.h>
#include <stdarg.h>
#include <limits.h>

#define P_tmpdir "/tmp"

#define L_ctermid PATH_MAX

__C_DECLS_BEGIN;

extern FILE *fdopen(int, const char *);
extern int fileno(FILE *);

/* Identifying the Terminal */
extern char *ctermid(char *s);

/* Input/Output */
extern ssize_t getdelim(char **__restrict__ lineptr, size_t *__restrict__ n,
    int delimiter, FILE *__restrict__ stream);
extern ssize_t getline(char **__restrict__ lineptr, size_t *__restrict__ n,
    FILE *__restrict__ stream);

#if defined(_LARGEFILE64_SOURCE) || defined(_GNU_SOURCE)
extern int fseeko64(FILE *stream, off64_t offset, int whence);
extern off64_t ftello64(FILE *stream);
#endif

#if _FILE_OFFSET_BITS == 64 && LONG_MAX == INT_MAX
#ifdef __GNUC__
extern int fseeko(FILE *stream, off_t offset, int whence) __asm__("fseeko64");
extern off_t ftello(FILE *stream) __asm__("ftello64");
#else
extern int fseeko64(FILE *stream, off_t offset, int whence);
extern off_t ftello64(FILE *stream);
#define fseeko fseeko64
#define ftello ftello64
#endif
#else
extern int fseeko(FILE *stream, off_t offset, int whence);
extern off_t ftello(FILE *stream);
#endif

/* Formatted Output */
extern int dprintf(int fildes, const char *__restrict__ format, ...)
    _HELENOS_PRINTF_ATTRIBUTE(2, 3);
extern int vdprintf(int fildes, const char *__restrict__ format, va_list ap);

/* File Locking */
extern void flockfile(FILE *file);
extern int ftrylockfile(FILE *file);
extern void funlockfile(FILE *file);
extern int getc_unlocked(FILE *stream);
extern int getchar_unlocked(void);
extern int putc_unlocked(int c, FILE *stream);
extern int putchar_unlocked(int c);

/* Temporary Files */
extern char *tempnam(const char *dir, const char *pfx);

__C_DECLS_END;

#endif /* POSIX_STDIO_H_ */

/** @}
 */
