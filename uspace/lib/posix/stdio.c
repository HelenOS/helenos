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

#define LIBPOSIX_INTERNAL

#include <assert.h>
#include <errno.h>

#include "internal/common.h"
#include "stdio.h"
#include "libc/io/printf_core.h"
#include "string.h"
#include "libc/str.h"

/* not the best of solutions, but freopen will eventually
 * need to be implemented in libc anyway
 */
#include "../c/generic/private/stdio.h"

/** Clears the stream's error and end-of-file indicators.
 *
 * @param stream
 */
void posix_clearerr(FILE *stream)
{
	stream->error = 0;
	stream->eof = 0;
}

/**
 *
 * @param s
 * @return
 */
char *posix_ctermid(char *s)
{
	/* Currently always returns an error value (empty string). */
	// TODO: return a real terminal path

	static char dummy_path[L_ctermid] = {'\0'};

	if (s == NULL) {
		return dummy_path;
	}

	s[0] = '\0';
	return s;
}

/**
 * 
 * @param c
 * @param stream
 * @return 
 */
int posix_ungetc(int c, FILE *stream)
{
	// TODO
	not_implemented();
}

ssize_t posix_getdelim(char **restrict lineptr, size_t *restrict n,
    int delimiter, FILE *restrict stream)
{
	// TODO
	not_implemented();
}

ssize_t posix_getline(char **restrict lineptr, size_t *restrict n,
    FILE *restrict stream)
{
	// TODO
	not_implemented();
}

/**
 * 
 * @param filename
 * @param mode
 * @param stream
 * @return
 */
FILE *posix_freopen(
    const char *restrict filename,
    const char *restrict mode,
    FILE *restrict stream)
{
	assert(mode != NULL);
	assert(stream != NULL);

	if (filename == NULL) {
		// TODO
		
		/* print error to stderr as well, to avoid hard to find problems
		 * with buggy apps that expect this to work
		 */
		fprintf(stderr,
		    "ERROR: Application wants to use freopen() to change mode of opened stream.\n"
		    "       libposix does not support that yet, the application may function improperly.\n");
		errno = ENOTSUP;
		return NULL;
	}

	FILE* copy = malloc(sizeof(FILE));
	if (copy == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	memcpy(copy, stream, sizeof(FILE));
	fclose(copy); /* copy is now freed */
	
	copy = fopen(filename, mode); /* open new stream */
	if (copy == NULL) {
		/* fopen() sets errno */
		return NULL;
	}
	
	/* move the new stream to the original location */
	memcpy(stream, copy, sizeof (FILE));
	free(copy);
	
	/* update references in the file list */
	stream->link.next->prev = &stream->link;
	stream->link.prev->next = &stream->link;
	
	return stream;
}

FILE *posix_fmemopen(void *restrict buf, size_t size,
    const char *restrict mode)
{
	// TODO
	not_implemented();
}

FILE *posix_open_memstream(char **bufp, size_t *sizep)
{
	// TODO
	not_implemented();
}

/**
 *
 * @param s
 */
void posix_perror(const char *s)
{
	if (s == NULL || s[0] == '\0') {
		fprintf(stderr, "%s\n", posix_strerror(errno));
	} else {
		fprintf(stderr, "%s: %s\n", s, posix_strerror(errno));
	}
}

struct _posix_fpos {
	off64_t offset;
};

/** Restores stream a to position previously saved with fgetpos().
 *
 * @param stream Stream to restore
 * @param pos Position to restore
 * @return Zero on success, non-zero (with errno set) on failure
 */
int posix_fsetpos(FILE *stream, const posix_fpos_t *pos)
{
	return fseek(stream, pos->offset, SEEK_SET);
}

/** Saves the stream's position for later use by fsetpos().
 *
 * @param stream Stream to save
 * @param pos Place to store the position
 * @return Zero on success, non-zero (with errno set) on failure
 */
int posix_fgetpos(FILE *restrict stream, posix_fpos_t *restrict pos)
{
	off64_t ret = ftell(stream);
	if (ret == -1) {
		return errno;
	}
	pos->offset = ret;
	return 0;
}

/**
 * 
 * @param stream
 * @param offset
 * @param whence
 * @return
 */
int posix_fseek(FILE *stream, long offset, int whence)
{
	return fseek(stream, (off64_t) offset, whence);
}

/**
 * 
 * @param stream
 * @param offset
 * @param whence
 * @return
 */
int posix_fseeko(FILE *stream, posix_off_t offset, int whence)
{
	return fseek(stream, (off64_t) offset, whence);
}

/**
 * 
 * @param stream
 * @return
 */
long posix_ftell(FILE *stream)
{
	return (long) ftell(stream);
}

/**
 * 
 * @param stream
 * @return
 */
posix_off_t posix_ftello(FILE *stream)
{
	return (posix_off_t) ftell(stream);
}

int posix_dprintf(int fildes, const char *restrict format, ...)
{
	va_list list;
	va_start(list, format);
	int result = posix_vdprintf(fildes, format, list);
	va_end(list);
	return result;
}

static int _dprintf_str_write(const char *str, size_t size, void *fd)
{
	ssize_t wr = write(*(int *) fd, str, size);
	return str_nlength(str, wr);
}

static int _dprintf_wstr_write(const wchar_t *str, size_t size, void *fd)
{
	size_t offset = 0;
	size_t chars = 0;
	size_t sz;
	char buf[4];
	
	while (offset < size) {
		sz = 0;
		if (chr_encode(str[chars], buf, &sz, sizeof(buf)) != EOK) {
			break;
		}
		
		if (write(*(int *) fd, buf, sz) != (ssize_t) sz) {
			break;
		}
		
		chars++;
		offset += sizeof(wchar_t);
	}
	
	return chars;
}

int posix_vdprintf(int fildes, const char *restrict format, va_list ap)
{
	printf_spec_t spec = {
		.str_write = _dprintf_str_write,
		.wstr_write = _dprintf_wstr_write,
		.data = &fildes
	};
	
	return printf_core(format, &spec, ap);
}

/**
 * 
 * @param s
 * @param format
 * @param ...
 * @return
 */
int posix_sprintf(char *s, const char *format, ...)
{
	va_list list;
	va_start(list, format);
	int result = posix_vsprintf(s, format, list);
	va_end(list);
	return result;
}

/**
 * 
 * @param s
 * @param format
 * @param ap
 * @return
 */
int posix_vsprintf(char *s, const char *format, va_list ap)
{
	return vsnprintf(s, STR_NO_LIMIT, format, ap);
}

/**
 * 
 * @param stream
 * @param format
 * @param ...
 * @return
 */
int posix_fscanf(FILE *restrict stream, const char *restrict format, ...)
{
	va_list list;
	va_start(list, format);
	int result = posix_vfscanf(stream, format, list);
	va_end(list);
	return result;
}

int posix_vfscanf(FILE *restrict stream, const char *restrict format, va_list arg)
{
	// TODO
	not_implemented();
}

/**
 * 
 * @param format
 * @param ...
 * @return
 */
int posix_scanf(const char *restrict format, ...)
{
	va_list list;
	va_start(list, format);
	int result = posix_vscanf(format, list);
	va_end(list);
	return result;
}

/**
 * 
 * @param format
 * @param arg
 * @return
 */
int posix_vscanf(const char *restrict format, va_list arg)
{
	return posix_vfscanf(stdin, format, arg);
}

/**
 * 
 * @param s
 * @param format
 * @param ...
 * @return
 */
int posix_sscanf(const char *s, const char *format, ...)
{
	va_list list;
	va_start(list, format);
	int result = posix_vsscanf(s, format, list);
	va_end(list);
	return result;
}

/**
 * 
 * @param s
 * @param format
 * @param arg
 * @return
 */
int posix_vsscanf(
    const char *restrict s, const char *restrict format, va_list arg)
{
	// TODO
	not_implemented();
}

void posix_flockfile(FILE *file)
{
	/* dummy */
}

int posix_ftrylockfile(FILE *file)
{
	/* dummy */
	return 0;
}

void posix_funlockfile(FILE *file)
{
	/* dummy */
}

int posix_getc_unlocked(FILE *stream)
{
	return getc(stream);
}

int posix_getchar_unlocked(void)
{
	return getchar();
}

int posix_putc_unlocked(int c, FILE *stream)
{
	return putc(c, stream);
}

int posix_putchar_unlocked(int c)
{
	return putchar(c);
}

/**
 *
 * @param path
 * @return
 */
int posix_remove(const char *path)
{
	// FIXME: unlink() and rmdir() seem to be equivalent at the moment,
	//        but that does not have to be true forever
	return unlink(path);
}

/**
 * 
 * @param s
 * @return
 */
char *posix_tmpnam(char *s)
{
	// TODO: low priority, just a compile-time dependency of binutils
	not_implemented();
}

/** @}
 */
