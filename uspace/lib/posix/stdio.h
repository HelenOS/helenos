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
/** @file
 */

#ifndef POSIX_STDIO_H_
#define POSIX_STDIO_H_

#include "libc/stdio.h"
#include "sys/types.h"
#include "libc/stdarg.h"

/* Character Input/Output */
#undef putc
#define putc fputc
#undef getc
#define getc fgetc
extern int posix_ungetc(int c, FILE *stream);

/* Opening Streams */
extern FILE *posix_freopen(
   const char *restrict filename,
   const char *restrict mode,
   FILE *restrict stream);

/* Error Messages */
extern void posix_perror(const char *s);

/* File Positioning */
extern int posix_fseeko(FILE *stream, posix_off_t offset, int whence);
extern posix_off_t posix_ftello(FILE *stream);

/* Formatted Input/Output */
extern int posix_sprintf(char *restrict s, const char *restrict format, ...);
extern int posix_vsprintf(char *restrict s, const char *restrict format, va_list ap);
extern int posix_sscanf(const char *restrict s, const char *restrict format, ...);

/* Deleting Files */
extern int posix_remove(const char *path);

/* Temporary Files */
#undef L_tmpnam
#define L_tmpnam PATH_MAX

extern char *posix_tmpnam(char *s);

#ifndef LIBPOSIX_INTERNAL
	#define ungetc posix_ungetc

	#define freopen posix_freopen

	#define perror posix_perror

	#define fseeko posix_fseeko
	#define ftello posix_ftello

	#define sprintf posix_sprintf
	#define vsprintf posix_vsprintf
	#define sscanf posix_sscanf

	#define remove posix_remove

	#define tmpnam posix_tmpnam
#endif

#endif /* POSIX_STDIO_H_ */

/** @}
 */
