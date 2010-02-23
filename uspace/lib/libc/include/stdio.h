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
#include <string.h>
#include <adt/list.h>

#define EOF  (-1)

/** Default size for stream I/O buffers */
#define BUFSIZ  4096

#define DEBUG(fmt, ...)se\
	{ \
		char _buf[256]; \
		int _n = snprintf(_buf, sizeof(_buf), fmt, ##__VA_ARGS__); \
		if (_n > 0) \
			(void) __SYSCALL3(SYS_KLOG, 1, (sysarg_t) _buf, str_size(_buf)); \
	}

#ifndef SEEK_SET
	#define SEEK_SET  0
#endif

#ifndef SEEK_CUR
	#define SEEK_CUR  1
#endif

#ifndef SEEK_END
	#define SEEK_END  2
#endif

enum _buffer_type {
	/** No buffering */
	_IONBF,
	/** Line buffering */
	_IOLBF,
	/** Full buffering */
	_IOFBF
};

typedef struct {
	/** Linked list pointer. */
	link_t link;
	
	/** Underlying file descriptor. */
	int fd;
	
	/** Error indicator. */
	int error;
	
	/** End-of-file indicator. */
	int eof;
	
	/** Klog indicator */
	int klog;
	
	/** Phone to the file provider */
	int phone;

	/** Buffering type */
	enum _buffer_type btype;
	/** Buffer */
	uint8_t *buf;
	/** Buffer size */
	size_t buf_size;
	/** Buffer I/O pointer */
	uint8_t *buf_head;
} FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

/* Character and string input functions */
extern int fgetc(FILE *);
extern char *fgets(char *, int, FILE *);

extern int getchar(void);
extern char *gets(char *, size_t);

/* Character and string output functions */
extern int fputc(wchar_t, FILE *);
extern int fputs(const char *, FILE *);

extern int putchar(wchar_t);
extern int puts(const char *);

/* Formatted string output functions */
extern int fprintf(FILE *, const char*, ...);
extern int vfprintf(FILE *, const char *, va_list);

extern int printf(const char *, ...);
extern int vprintf(const char *, va_list);

extern int snprintf(char *, size_t , const char *, ...);
extern int asprintf(char **, const char *, ...);
extern int vsnprintf(char *, size_t, const char *, va_list);

/* File stream functions */
extern FILE *fopen(const char *, const char *);
extern FILE *fdopen(int, const char *);
extern int fclose(FILE *);

extern size_t fread(void *, size_t, size_t, FILE *);
extern size_t fwrite(const void *, size_t, size_t, FILE *);

extern int fseek(FILE *, off64_t, int);
extern void rewind(FILE *);
extern off64_t ftell(FILE *);
extern int feof(FILE *);

extern int fflush(FILE *);
extern int ferror(FILE *);
extern void clearerr(FILE *);

extern void setvbuf(FILE *, void *, int, size_t);

/* Misc file functions */
extern int rename(const char *, const char *);

#endif

/** @}
 */
