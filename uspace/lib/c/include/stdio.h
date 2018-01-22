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

#ifndef LIBC_STDIO_H_
#define LIBC_STDIO_H_

#include <stdarg.h>
#include <io/verify.h>
#include <_bits/size_t.h>
#include <_bits/wchar_t.h>

#define EOF  (-1)

#ifndef SEEK_SET
	#define SEEK_SET  0
#endif

#ifndef SEEK_CUR
	#define SEEK_CUR  1
#endif

#ifndef SEEK_END
	#define SEEK_END  2
#endif

/** Default size for stream I/O buffers */
#define BUFSIZ  4096

/** Forward declaration */
struct _IO_FILE;
typedef struct _IO_FILE FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

/* Character and string input functions */
extern int fgetc(FILE *);
extern char *fgets(char *, int, FILE *);

extern int getchar(void);

/* Character and string output functions */
extern int fputc(wchar_t, FILE *);
extern int fputs(const char *, FILE *);

extern int putchar(wchar_t);
extern int puts(const char *);

extern int ungetc(int, FILE *);

/* Formatted string output functions */
extern int fprintf(FILE *, const char*, ...)
    _HELENOS_PRINTF_ATTRIBUTE(2, 3);
extern int vfprintf(FILE *, const char *, va_list);

extern int printf(const char *, ...)
    _HELENOS_PRINTF_ATTRIBUTE(1, 2);
extern int vprintf(const char *, va_list);

extern int snprintf(char *, size_t , const char *, ...)
    _HELENOS_PRINTF_ATTRIBUTE(3, 4);
extern int vasprintf(char **, const char *, va_list);
extern int asprintf(char **, const char *, ...)
    _HELENOS_PRINTF_ATTRIBUTE(2, 3);
extern int vsnprintf(char *, size_t, const char *, va_list);

/* File stream functions */
extern FILE *fopen(const char *, const char *);
extern FILE *freopen(const char *, const char *, FILE *);
extern int fclose(FILE *);

extern size_t fread(void *, size_t, size_t, FILE *);
extern size_t fwrite(const void *, size_t, size_t, FILE *);

extern int fseek(FILE *, long, int);
extern void rewind(FILE *);
extern long ftell(FILE *);
extern int feof(FILE *);

extern int fflush(FILE *);
extern int ferror(FILE *);
extern void clearerr(FILE *);

extern void setvbuf(FILE *, void *, int, size_t);
extern void setbuf(FILE *, void *);

/* Misc file functions */
extern int rename(const char *, const char *);
extern int remove(const char *);

#ifndef _HELENOS_SOURCE
#define _IONBF 0
#define _IOLBF 1
#define _IOFBF 2
#endif

#ifdef _HELENOS_SOURCE

/* Nonstandard extensions. */

enum _buffer_type {
	/** No buffering */
	_IONBF,
	/** Line buffering */
	_IOLBF,
	/** Full buffering */
	_IOFBF
};

enum _buffer_state {
	/** Buffer is empty */
	_bs_empty,

	/** Buffer contains data to be written */
	_bs_write,

	/** Buffer contains prefetched data for reading */
	_bs_read
};

extern int vprintf_size(const char *, va_list);
extern int printf_size(const char *, ...)
    _HELENOS_PRINTF_ATTRIBUTE(1, 2);
extern FILE *fdopen(int, const char *);
extern int fileno(FILE *);
extern char *gets(char *, size_t);

#include <offset.h>

extern int fseek64(FILE *, off64_t, int);
extern off64_t ftell64(FILE *);

#endif


#endif

/** @}
 */
