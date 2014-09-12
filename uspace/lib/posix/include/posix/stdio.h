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

#ifndef __POSIX_DEF__
#define __POSIX_DEF__(x) x
/* DEBUG macro does not belong to POSIX stdio.h. Its unconditional
 * definition in the native stdio.h causes unexpected behaviour of
 * applications which uses their own DEBUG macro (e.g. debugging
 * output is printed even if not desirable). */
#undef DEBUG
#endif

#include "stddef.h"
#include "unistd.h"
#include "libc/io/verify.h"
#include "sys/types.h"
#include "stdarg.h"
#include "limits.h"

/*
 * These are the same as in HelenOS libc.
 * It would be possible to directly include <stdio.h> but
 * it is better not to pollute POSIX namespace with other functions
 * defined in that header.
 *
 * Because libposix is always linked with libc, providing only these
 * forward declarations ought to be enough.
 */
#define EOF (-1)

/** Size of buffers used in stdio header. */
#define BUFSIZ  4096

/** Maximum size in bytes of the longest filename. */
#define FILENAME_MAX 4096

typedef struct _IO_FILE FILE;

#ifndef LIBPOSIX_INTERNAL
	enum _buffer_type {
		/** No buffering */
		_IONBF,
		/** Line buffering */
		_IOLBF,
		/** Full buffering */
		_IOFBF
	};
#endif

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

extern int fgetc(FILE *);
extern char *fgets(char *, int, FILE *);

extern int getchar(void);
extern char *gets(char *, size_t);

extern int fputc(wchar_t, FILE *);
extern int fputs(const char *, FILE *);

extern int putchar(wchar_t);
extern int puts(const char *);

extern int fprintf(FILE *, const char*, ...) PRINTF_ATTRIBUTE(2, 3);
extern int vfprintf(FILE *, const char *, va_list);

extern int printf(const char *, ...) PRINTF_ATTRIBUTE(1, 2);
extern int vprintf(const char *, va_list);

extern int snprintf(char *, size_t , const char *, ...) PRINTF_ATTRIBUTE(3, 4);
#ifdef _GNU_SOURCE
extern int vasprintf(char **, const char *, va_list);
extern int asprintf(char **, const char *, ...) PRINTF_ATTRIBUTE(2, 3);
#endif
extern int vsnprintf(char *, size_t, const char *, va_list);

extern FILE *fopen(const char *, const char *);
extern FILE *fdopen(int, const char *);
extern int fclose(FILE *);

extern size_t fread(void *, size_t, size_t, FILE *);
extern size_t fwrite(const void *, size_t, size_t, FILE *);

extern void rewind(FILE *);
extern int feof(FILE *);
extern int fileno(FILE *);

extern int fflush(FILE *);
extern int ferror(FILE *);
extern void clearerr(FILE *);

extern void setvbuf(FILE *, void *, int, size_t);
extern void setbuf(FILE *, void *);

/* POSIX specific stuff. */

/* Identifying the Terminal */
#undef L_ctermid
#define L_ctermid PATH_MAX
extern char *__POSIX_DEF__(ctermid)(char *s);

/* Error Recovery */
extern void __POSIX_DEF__(clearerr)(FILE *stream);

/* Input/Output */
#undef putc
#define putc fputc
extern int __POSIX_DEF__(fputs)(const char *restrict s, FILE *restrict stream);
#undef getc
#define getc fgetc
extern int __POSIX_DEF__(ungetc)(int c, FILE *stream);
extern ssize_t __POSIX_DEF__(getdelim)(char **restrict lineptr, size_t *restrict n,
    int delimiter, FILE *restrict stream);
extern ssize_t __POSIX_DEF__(getline)(char **restrict lineptr, size_t *restrict n,
    FILE *restrict stream);

/* Opening Streams */
extern FILE *__POSIX_DEF__(freopen)(const char *restrict filename,
    const char *restrict mode, FILE *restrict stream);

/* Error Messages */
extern void __POSIX_DEF__(perror)(const char *s);

/* File Positioning */
typedef struct {
	off64_t offset;
} __POSIX_DEF__(fpos_t);

extern int __POSIX_DEF__(fsetpos)(FILE *stream, const __POSIX_DEF__(fpos_t) *pos);
extern int __POSIX_DEF__(fgetpos)(FILE *restrict stream, __POSIX_DEF__(fpos_t) *restrict pos);
extern int __POSIX_DEF__(fseek)(FILE *stream, long offset, int whence);
extern int __POSIX_DEF__(fseeko)(FILE *stream, __POSIX_DEF__(off_t) offset, int whence);
extern long __POSIX_DEF__(ftell)(FILE *stream);
extern __POSIX_DEF__(off_t) __POSIX_DEF__(ftello)(FILE *stream);

/* Flushing Buffers */
extern int __POSIX_DEF__(fflush)(FILE *stream);

/* Formatted Output */
extern int __POSIX_DEF__(dprintf)(int fildes, const char *restrict format, ...)
    PRINTF_ATTRIBUTE(2, 3);
extern int __POSIX_DEF__(vdprintf)(int fildes, const char *restrict format, va_list ap);
extern int __POSIX_DEF__(sprintf)(char *restrict s, const char *restrict format, ...)
    PRINTF_ATTRIBUTE(2, 3);
extern int __POSIX_DEF__(vsprintf)(char *restrict s, const char *restrict format, va_list ap);

/* Formatted Input */
extern int __POSIX_DEF__(fscanf)(
    FILE *restrict stream, const char *restrict format, ...);
extern int __POSIX_DEF__(vfscanf)(
    FILE *restrict stream, const char *restrict format, va_list arg);
extern int __POSIX_DEF__(scanf)(const char *restrict format, ...);
extern int __POSIX_DEF__(vscanf)(const char *restrict format, va_list arg);
extern int __POSIX_DEF__(sscanf)(
    const char *restrict s, const char *restrict format, ...);
extern int __POSIX_DEF__(vsscanf)(
    const char *restrict s, const char *restrict format, va_list arg);

/* File Locking */
extern void __POSIX_DEF__(flockfile)(FILE *file);
extern int __POSIX_DEF__(ftrylockfile)(FILE *file);
extern void __POSIX_DEF__(funlockfile)(FILE *file);
extern int __POSIX_DEF__(getc_unlocked)(FILE *stream);
extern int __POSIX_DEF__(getchar_unlocked)(void);
extern int __POSIX_DEF__(putc_unlocked)(int c, FILE *stream);
extern int __POSIX_DEF__(putchar_unlocked)(int c);

/* Deleting Files */
extern int __POSIX_DEF__(remove)(const char *path);

/* Renaming Files */
extern int __POSIX_DEF__(rename)(const char *oldname, const char *newname);

/* Temporary Files */
#undef L_tmpnam
#define L_tmpnam PATH_MAX
extern char *__POSIX_DEF__(tmpnam)(char *s);
extern char *__POSIX_DEF__(tempnam)(const char *dir, const char *pfx);
extern FILE *__POSIX_DEF__(tmpfile)(void);


#endif /* POSIX_STDIO_H_ */

/** @}
 */
