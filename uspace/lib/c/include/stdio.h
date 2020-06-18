/*
 * Copyright (c) 2005 Martin Decky
 * Copyright (c) 2018 Jiri Svoboda
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

#ifndef _LIBC_STDIO_H_
#define _LIBC_STDIO_H_

#include <stdarg.h>
#include <io/verify.h>
#include <_bits/NULL.h>
#include <_bits/size_t.h>
#include <_bits/wchar_t.h>
#include <_bits/uchar.h>
#include <_bits/wint_t.h>
#include <_bits/decls.h>

#ifndef _HELENOS_SOURCE
#define _IONBF 0
#define _IOLBF 1
#define _IOFBF 2
#endif

/** Default size for stream I/O buffers */
#define BUFSIZ  4096

#define EOF  (-1)

/** Max number of files that is guaranteed to be able to open at the same time */
#define FOPEN_MAX 16

/** Recommended size of fixed-size array for holding file names. */
#define FILENAME_MAX 4096

/** Length of "/tmp/tmp.XXXXXX" + 1 */
#define L_tmpnam 16

#ifndef SEEK_SET
#define SEEK_SET  0
#endif

#ifndef SEEK_CUR
#define SEEK_CUR  1
#endif

#ifndef SEEK_END
#define SEEK_END  2
#endif

/** Minimum number of unique temporary file names */
#define TMP_MAX 1000000

__C_DECLS_BEGIN;

/** Forward declaration */
struct _IO_FILE;
typedef struct _IO_FILE FILE;

/** File position */
typedef struct {
	long long pos;
} fpos_t;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

/* Character and string input functions */
extern int fgetc(FILE *);
extern char *fgets(char *, int, FILE *);
extern char *gets(char *, size_t) __attribute__((deprecated));

static inline int getc(FILE *f)
{
	return fgetc(f);
}

extern int getchar(void);

/* Character and string output functions */
extern int fputc(int, FILE *);
extern int fputs(const char *, FILE *);

static inline int putc(int i, FILE *f)
{
	return fputc(i, f);
}

extern int putchar(int);
extern int puts(const char *);

extern int ungetc(int, FILE *);

extern wint_t fputwc(wchar_t, FILE *);
extern wint_t putwchar(wchar_t);

extern wint_t fputuc(char32_t, FILE *);
extern wint_t putuchar(char32_t);

/* Formatted string output functions */
extern int fprintf(FILE *, const char *, ...)
    _HELENOS_PRINTF_ATTRIBUTE(2, 3);
extern int vfprintf(FILE *, const char *, va_list);

extern int printf(const char *, ...)
    _HELENOS_PRINTF_ATTRIBUTE(1, 2);
extern int vprintf(const char *, va_list);

extern int snprintf(char *, size_t, const char *, ...)
    _HELENOS_PRINTF_ATTRIBUTE(3, 4);
#if defined(_HELENOS_SOURCE) || defined(_GNU_SOURCE)
extern int vasprintf(char **, const char *, va_list);
extern int asprintf(char **, const char *, ...)
    _HELENOS_PRINTF_ATTRIBUTE(2, 3);
#endif
extern int vsnprintf(char *, size_t, const char *, va_list);

extern int sprintf(char *, const char *, ...)
    __attribute__((deprecated)) _HELENOS_PRINTF_ATTRIBUTE(2, 3);
extern int vsprintf(char *, const char *, va_list) __attribute__((deprecated));

/* Formatted input */
extern int scanf(const char *, ...);
extern int vscanf(const char *, va_list);
extern int fscanf(FILE *, const char *, ...);
extern int vfscanf(FILE *, const char *, va_list);
extern int sscanf(const char *, const char *, ...);
extern int vsscanf(const char *, const char *, va_list);

/* File stream functions */
extern FILE *fopen(const char *, const char *);
extern FILE *freopen(const char *, const char *, FILE *);
extern int fclose(FILE *);

extern size_t fread(void *, size_t, size_t, FILE *);
extern size_t fwrite(const void *, size_t, size_t, FILE *);

extern int fgetpos(FILE *, fpos_t *);
extern int fsetpos(FILE *, const fpos_t *);

extern int fseek(FILE *, long, int);
extern void rewind(FILE *);
extern long ftell(FILE *);
extern int feof(FILE *);

extern int fflush(FILE *);
extern int ferror(FILE *);
extern void clearerr(FILE *);

extern void perror(const char *);

extern int setvbuf(FILE *, void *, int, size_t);
extern void setbuf(FILE *, void *);

/* Misc file functions */
extern int remove(const char *);
extern int rename(const char *, const char *);

extern FILE *tmpfile(void);
extern char *tmpnam(char *s);

__C_DECLS_END;

#ifdef _HELENOS_SOURCE

#include <_bits/off64_t.h>

__HELENOS_DECLS_BEGIN;

/* Nonstandard extensions. */

enum __buffer_type {
	/** No buffering */
	_IONBF,
	/** Line buffering */
	_IOLBF,
	/** Full buffering */
	_IOFBF
};

extern int vprintf_length(const char *, va_list);
extern int printf_length(const char *, ...)
    _HELENOS_PRINTF_ATTRIBUTE(1, 2);
extern FILE *fdopen(int, const char *);
extern int fileno(FILE *);

extern int fseek64(FILE *, off64_t, int);
extern off64_t ftell64(FILE *);

__HELENOS_DECLS_END;
#endif

#endif

/** @}
 */
