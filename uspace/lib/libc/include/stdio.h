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

#include <sys/types.h>
#include <stdarg.h>

#define EOF (-1)

#include <string.h>
#include <io/stream.h>

#define DEBUG(fmt, ...) \
{ \
	char buf[256]; \
	int n; \
	n = snprintf(buf, sizeof(buf), fmt, ##__VA_ARGS__); \
	if (n > 0) \
		(void) __SYSCALL3(SYS_KLOG, 1, (sysarg_t) buf, strlen(buf)); \
}

typedef struct {
	/** Underlying file descriptor. */
	int fd;

	/** Error indicator. */
	int error;

	/** End-of-file indicator. */
	int eof;
} FILE;

extern int getchar(void);

extern int puts(const char *);
extern int putchar(int);

extern int printf(const char *, ...);
extern int asprintf(char **, const char *, ...);
extern int sprintf(char *, const char *fmt, ...);
extern int snprintf(char *, size_t , const char *, ...);

extern int vprintf(const char *, va_list);
extern int vsprintf(char *, const char *, va_list);
extern int vsnprintf(char *, size_t, const char *, va_list);

#define fprintf(f, fmt, ...) printf(fmt, ##__VA_ARGS__)

extern int rename(const char *, const char *);

extern FILE *fopen(const char *, const char *);
extern int fclose(FILE *);
extern size_t fread(void *, size_t, size_t, FILE *);
extern size_t fwrite(const void *, size_t, size_t, FILE *);
extern int feof(FILE *);
extern int ferror(FILE *);
extern void clearerr(FILE *);

extern int fgetc(FILE *);;
extern int fputc(int, FILE *);
extern int fputs(const char *, FILE *);

#define getc fgetc
#define putc fputc

extern int fseek(FILE *, long, int);

#ifndef SEEK_SET
	#define SEEK_SET	0
	#define SEEK_CUR	1
	#define SEEK_END	2
#endif

#endif

/** @}
 */
