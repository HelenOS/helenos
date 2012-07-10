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

#include "stddef.h"
#include "unistd.h"
#include "libc/stdio.h"
#include "sys/types.h"
#include "libc/stdarg.h"
#include "limits.h"

/* Identifying the Terminal */
#undef L_ctermid
#define L_ctermid PATH_MAX
extern char *posix_ctermid(char *s);

/* Error Recovery */
extern void posix_clearerr(FILE *stream);

/* Input/Output */
#undef putc
#define putc fputc
extern int posix_fputs(const char *restrict s, FILE *restrict stream);
#undef getc
#define getc fgetc
extern int posix_ungetc(int c, FILE *stream);
extern ssize_t posix_getdelim(char **restrict lineptr, size_t *restrict n,
    int delimiter, FILE *restrict stream);
extern ssize_t posix_getline(char **restrict lineptr, size_t *restrict n,
    FILE *restrict stream);

/* Opening Streams */
extern FILE *posix_freopen(const char *restrict filename,
    const char *restrict mode, FILE *restrict stream);

/* Error Messages */
extern void posix_perror(const char *s);

/* File Positioning */
typedef struct _posix_fpos posix_fpos_t;
extern int posix_fsetpos(FILE *stream, const posix_fpos_t *pos);
extern int posix_fgetpos(FILE *restrict stream, posix_fpos_t *restrict pos);
extern int posix_fseek(FILE *stream, long offset, int whence);
extern int posix_fseeko(FILE *stream, posix_off_t offset, int whence);
extern long posix_ftell(FILE *stream);
extern posix_off_t posix_ftello(FILE *stream);

/* Flushing Buffers */
extern int posix_fflush(FILE *stream);

/* Formatted Output */
extern int posix_dprintf(int fildes, const char *restrict format, ...)
    PRINTF_ATTRIBUTE(2, 3);
extern int posix_vdprintf(int fildes, const char *restrict format, va_list ap);
extern int posix_sprintf(char *restrict s, const char *restrict format, ...)
    PRINTF_ATTRIBUTE(2, 3);
extern int posix_vsprintf(char *restrict s, const char *restrict format, va_list ap);

/* Formatted Input */
extern int posix_fscanf(
    FILE *restrict stream, const char *restrict format, ...);
extern int posix_vfscanf(
    FILE *restrict stream, const char *restrict format, va_list arg);
extern int posix_scanf(const char *restrict format, ...);
extern int posix_vscanf(const char *restrict format, va_list arg);
extern int posix_sscanf(
    const char *restrict s, const char *restrict format, ...);
extern int posix_vsscanf(
    const char *restrict s, const char *restrict format, va_list arg);

/* File Locking */
extern void posix_flockfile(FILE *file);
extern int posix_ftrylockfile(FILE *file);
extern void posix_funlockfile(FILE *file);
extern int posix_getc_unlocked(FILE *stream);
extern int posix_getchar_unlocked(void);
extern int posix_putc_unlocked(int c, FILE *stream);
extern int posix_putchar_unlocked(int c);

/* Deleting Files */
extern int posix_remove(const char *path);

/* Renaming Files */
extern int posix_rename(const char *old, const char *new);

/* Temporary Files */
#undef L_tmpnam
#define L_tmpnam PATH_MAX
extern char *posix_tmpnam(char *s);
extern char *posix_tempnam(const char *dir, const char *pfx);
extern FILE *posix_tmpfile(void);

#ifndef LIBPOSIX_INTERNAL
	/* DEBUG macro does not belong to POSIX stdio.h. Its unconditional
	 * definition in the native stdio.h causes unexpected behaviour of
	 * applications which uses their own DEBUG macro (e.g. debugging
	 * output is printed even if not desirable). */
	#undef DEBUG

	#define ctermid posix_ctermid

	#define clearerr posix_clearerr

	#define fputs posix_fputs
	#define ungetc posix_ungetc
	#define getdelim posix_getdelim
	#define getline posix_getline

	#define freopen posix_freopen

	#define perror posix_perror

	#define fpos_t posix_fpos_t
	#define fsetpos posix_fsetpos
	#define fgetpos posix_fgetpos
	#define fseek posix_fseek
	#define fseeko posix_fseeko
	#define ftell posix_ftell
	#define ftello posix_ftello

	#define fflush posix_fflush

	#define dprintf posix_dprintf
	#define vdprintf posix_vdprintf
	#define sprintf posix_sprintf
	#define vsprintf posix_vsprintf

	#define fscanf posix_fscanf
	#define vfscanf posix_vfscanf
	#define vscanf posix_vscanf
	#define scanf posix_scanf
	#define sscanf posix_sscanf
	#define vsscanf posix_vsscanf

	#define flockfile posix_flockfile
	#define ftrylockfile posix_ftrylockfile
	#define funlockfile posix_funlockfile

	#define getc_unlocked posix_getc_unlocked
	#define getchar_unlocked posix_getchar_unlocked
	#define putc_unlocked posix_putc_unlocked
	#define putchar_unlocked posix_putchar_unlocked

	#define remove posix_remove

	#define rename posix_rename

	#define tmpnam posix_tmpnam
	#define tempnam posix_tempnam
	#define tmpfile posix_tmpfile
#endif

#endif /* POSIX_STDIO_H_ */

/** @}
 */
