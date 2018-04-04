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

#include "libc/stdio.h"

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

extern int fprintf(FILE *, const char *, ...) _HELENOS_PRINTF_ATTRIBUTE(2, 3);
extern int vfprintf(FILE *, const char *, va_list);

extern int printf(const char *, ...) _HELENOS_PRINTF_ATTRIBUTE(1, 2);
extern int vprintf(const char *, va_list);

extern int snprintf(char *, size_t, const char *, ...) _HELENOS_PRINTF_ATTRIBUTE(3, 4);
#ifdef _GNU_SOURCE
extern int vasprintf(char **, const char *, va_list);
extern int asprintf(char **, const char *, ...) _HELENOS_PRINTF_ATTRIBUTE(2, 3);
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
extern char *ctermid(char *s);

/* Error Recovery */
extern void clearerr(FILE *stream);

/* Input/Output */
#undef putc
#define putc fputc
extern int fputs(const char *__restrict__ s, FILE *__restrict__ stream);
#undef getc
#define getc fgetc
extern int ungetc(int c, FILE *stream);
extern ssize_t getdelim(char **__restrict__ lineptr, size_t *__restrict__ n,
    int delimiter, FILE *__restrict__ stream);
extern ssize_t getline(char **__restrict__ lineptr, size_t *__restrict__ n,
    FILE *__restrict__ stream);

/* Opening Streams */
extern FILE *freopen(const char *__restrict__ filename,
    const char *__restrict__ mode, FILE *__restrict__ stream);

/* Error Messages */
extern void perror(const char *s);

/* File Positioning */
typedef struct {
	off64_t offset;
} fpos_t;

extern int fsetpos(FILE *stream, const fpos_t *pos);
extern int fgetpos(FILE *__restrict__ stream, fpos_t *__restrict__ pos);
extern int fseek(FILE *stream, long offset, int whence);
extern int fseeko(FILE *stream, off_t offset, int whence);
extern long ftell(FILE *stream);
extern off_t ftello(FILE *stream);

/* Flushing Buffers */
extern int fflush(FILE *stream);

/* Formatted Output */
extern int dprintf(int fildes, const char *__restrict__ format, ...)
    _HELENOS_PRINTF_ATTRIBUTE(2, 3);
extern int vdprintf(int fildes, const char *__restrict__ format, va_list ap);
extern int sprintf(char *__restrict__ s, const char *__restrict__ format, ...)
    _HELENOS_PRINTF_ATTRIBUTE(2, 3);
extern int vsprintf(char *__restrict__ s, const char *__restrict__ format, va_list ap);

/* Formatted Input */
extern int fscanf(
    FILE *__restrict__ stream, const char *__restrict__ format, ...);
extern int vfscanf(
    FILE *__restrict__ stream, const char *__restrict__ format, va_list arg);
extern int scanf(const char *__restrict__ format, ...);
extern int vscanf(const char *__restrict__ format, va_list arg);
extern int sscanf(
    const char *__restrict__ s, const char *__restrict__ format, ...);
extern int vsscanf(
    const char *__restrict__ s, const char *__restrict__ format, va_list arg);

/* File Locking */
extern void flockfile(FILE *file);
extern int ftrylockfile(FILE *file);
extern void funlockfile(FILE *file);
extern int getc_unlocked(FILE *stream);
extern int getchar_unlocked(void);
extern int putc_unlocked(int c, FILE *stream);
extern int putchar_unlocked(int c);

/* Deleting Files */
extern int remove(const char *path);

/* Renaming Files */
extern int rename(const char *oldname, const char *newname);

/* Temporary Files */
#undef L_tmpnam
#define L_tmpnam PATH_MAX
extern char *tmpnam(char *s);
extern char *tempnam(const char *dir, const char *pfx);
extern FILE *tmpfile(void);


#endif /* POSIX_STDIO_H_ */

/** @}
 */
