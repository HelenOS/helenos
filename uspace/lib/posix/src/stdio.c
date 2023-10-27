/*
 * Copyright (c) 2011 Jiri Zarevucky
 * Copyright (c) 2011 Petr Koupy
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

/** @addtogroup libposix
 * @{
 */
/** @file Standard buffered input/output.
 */

#define _LARGEFILE64_SOURCE
#undef _FILE_OFFSET_BITS

#include "internal/common.h"
#include <stdio.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <tmpfile.h>

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <printf_core.h>
#include <str.h>
#include <stdlib.h>
#include <adt/list.h>

/**
 * Generate a pathname for the controlling terminal.
 *
 * @param s Allocated buffer to which the pathname shall be put.
 * @return Either s or static location filled with the requested pathname.
 */
char *ctermid(char *s)
{
	/* Currently always returns an error value (empty string). */
	// TODO: return a real terminal path

	static char dummy_path[L_ctermid] = { '\0' };

	if (s == NULL) {
		return dummy_path;
	}

	s[0] = '\0';
	return s;
}

/**
 * Read a stream until the delimiter (or EOF) is encountered.
 *
 * @param lineptr Pointer to the output buffer in which there will be stored
 *     nul-terminated string together with the delimiter (if encountered).
 *     Will be resized if necessary.
 * @param n Pointer to the size of the output buffer. Will be increased if
 *     necessary.
 * @param delimiter Delimiter on which to finish reading the stream.
 * @param stream Input stream.
 * @return Number of fetched characters (including delimiter if encountered)
 *     or -1 on error (set in errno).
 */
ssize_t getdelim(char **restrict lineptr, size_t *restrict n,
    int delimiter, FILE *restrict stream)
{
	/* Check arguments for sanity. */
	if (!lineptr || !n) {
		errno = EINVAL;
		return -1;
	}

	size_t alloc_step = 80; /* Buffer size gain during reallocation. */
	char *pos = *lineptr; /* Next free byte of the output buffer. */
	size_t cnt = 0; /* Number of fetched characters. */
	int c = fgetc(stream); /* Current input character. Might be EOF. */

	do {
		/* Mask EOF as NUL to terminate string. */
		if (c == EOF) {
			c = '\0';
		}

		/* Ensure there is still space left in the buffer. */
		if (pos == *lineptr + *n) {
			*lineptr = realloc(*lineptr, *n + alloc_step);
			if (*lineptr) {
				pos = *lineptr + *n;
				*n += alloc_step;
			} else {
				errno = ENOMEM;
				return -1;
			}
		}

		/* Store the fetched character. */
		*pos = c;

		/* Fetch the next character according to the current character. */
		if (c != '\0') {
			++pos;
			++cnt;
			if (c == delimiter) {
				/*
				 * Delimiter was just stored. Provide EOF as the next
				 * character - it will be masked as NUL and output string
				 * will be properly terminated.
				 */
				c = EOF;
			} else {
				/*
				 * Neither delimiter nor EOF were encountered. Just fetch
				 * the next character from the stream.
				 */
				c = fgetc(stream);
			}
		}
	} while (c != '\0');

	if (errno == EOK && cnt > 0) {
		return cnt;
	} else {
		/* Either some error occured or the stream was already at EOF. */
		return -1;
	}
}

/**
 * Read a stream until the newline (or EOF) is encountered.
 *
 * @param lineptr Pointer to the output buffer in which there will be stored
 *     nul-terminated string together with the delimiter (if encountered).
 *     Will be resized if necessary.
 * @param n Pointer to the size of the output buffer. Will be increased if
 *     necessary.
 * @param stream Input stream.
 * @return Number of fetched characters (including newline if encountered)
 *     or -1 on error (set in errno).
 */
ssize_t getline(char **restrict lineptr, size_t *restrict n,
    FILE *restrict stream)
{
	return getdelim(lineptr, n, '\n', stream);
}

/**
 * Reposition a file-position indicator in a stream.
 *
 * @param stream Stream to seek in.
 * @param offset Direction and amount of bytes to seek.
 * @param whence From where to seek.
 * @return Zero on success, -1 otherwise.
 */
int fseeko(FILE *stream, off_t offset, int whence)
{
	return fseek(stream, offset, whence);
}

/**
 * Discover current file offset in a stream.
 *
 * @param stream Stream for which the offset shall be retrieved.
 * @return Current offset or -1 if not possible.
 */
off_t ftello(FILE *stream)
{
	return ftell(stream);
}

int fseeko64(FILE *stream, off64_t offset, int whence)
{
	return fseek64(stream, offset, whence);
}

off64_t ftello64(FILE *stream)
{
	return ftell64(stream);
}

/**
 * Print formatted output to the opened file.
 *
 * @param fildes File descriptor of the opened file.
 * @param format Format description.
 * @return Either the number of printed characters or negative value on error.
 */
int dprintf(int fildes, const char *restrict format, ...)
{
	va_list list;
	va_start(list, format);
	int result = vdprintf(fildes, format, list);
	va_end(list);
	return result;
}

/**
 * Write ordinary string to the opened file.
 *
 * @param str String to be written.
 * @param size Size of the string (in bytes)..
 * @param fd File descriptor of the opened file.
 * @return The number of written characters.
 */
static int _dprintf_str_write(const char *str, size_t size, void *fd)
{
	const int fildes = *(int *) fd;
	size_t wr;
	if (failed(vfs_write(fildes, &posix_pos[fildes], str, size, &wr)))
		return -1;
	return str_nlength(str, wr);
}

/**
 * Write wide string to the opened file.
 *
 * @param str String to be written.
 * @param size Size of the string (in bytes).
 * @param fd File descriptor of the opened file.
 * @return The number of written characters.
 */
static int _dprintf_wstr_write(const char32_t *str, size_t size, void *fd)
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

		const int fildes = *(int *) fd;
		size_t nwr;
		if (vfs_write(fildes, &posix_pos[fildes], buf, sz, &nwr) != EOK)
			break;

		chars++;
		offset += sizeof(char32_t);
	}

	return chars;
}

/**
 * Print formatted output to the opened file.
 *
 * @param fildes File descriptor of the opened file.
 * @param format Format description.
 * @param ap Print arguments.
 * @return Either the number of printed characters or negative value on error.
 */
int vdprintf(int fildes, const char *restrict format, va_list ap)
{
	printf_spec_t spec = {
		.str_write = _dprintf_str_write,
		.wstr_write = _dprintf_wstr_write,
		.data = &fildes
	};

	return printf_core(format, &spec, ap);
}

/**
 * Acquire file stream for the thread.
 *
 * @param file File stream to lock.
 */
void flockfile(FILE *file)
{
	/* dummy */
}

/**
 * Acquire file stream for the thread (non-blocking).
 *
 * @param file File stream to lock.
 * @return Zero for success and non-zero if the lock cannot be acquired.
 */
int ftrylockfile(FILE *file)
{
	/* dummy */
	return 0;
}

/**
 * Relinquish the ownership of the locked file stream.
 *
 * @param file File stream to unlock.
 */
void funlockfile(FILE *file)
{
	/* dummy */
}

/**
 * Get a byte from a stream (thread-unsafe).
 *
 * @param stream Input file stream.
 * @return Either read byte or EOF.
 */
int getc_unlocked(FILE *stream)
{
	return getc(stream);
}

/**
 * Get a byte from the standard input stream (thread-unsafe).
 *
 * @return Either read byte or EOF.
 */
int getchar_unlocked(void)
{
	return getchar();
}

/**
 * Put a byte on a stream (thread-unsafe).
 *
 * @param c Byte to output.
 * @param stream Output file stream.
 * @return Either written byte or EOF.
 */
int putc_unlocked(int c, FILE *stream)
{
	return putc(c, stream);
}

/**
 * Put a byte on the standard output stream (thread-unsafe).
 *
 * @param c Byte to output.
 * @return Either written byte or EOF.
 */
int putchar_unlocked(int c)
{
	return putchar(c);
}

/** Determine if directory is an 'appropriate' temporary directory.
 *
 * @param dir Directory path
 * @return @c true iff directory is appropriate.
 */
static bool is_appropriate_tmpdir(const char *dir)
{
	struct stat sbuf;

	/* Must not be NULL */
	if (dir == NULL)
		return false;

	/* Must not be empty */
	if (dir[0] == '\0')
		return false;

	if (stat(dir, &sbuf) != 0)
		return false;

	/* Must be a directory */
	if ((sbuf.st_mode & S_IFMT) != S_IFDIR)
		return false;

	/* Must be writable */
	if (access(dir, W_OK) != 0)
		return false;

	return true;
}

/** Construct unique file name.
 *
 * Never use this function.
 *
 * @param dir Path to directory, where the file should be created.
 * @param pfx Optional prefix up to 5 characters long.
 * @return Newly allocated unique path for temporary file. NULL on failure.
 */
char *tempnam(const char *dir, const char *pfx)
{
	const char *dpref;
	char *d;
	char *buf;
	int rc;

	d = getenv("TMPDIR");
	if (is_appropriate_tmpdir(d))
		dpref = d;
	else if (is_appropriate_tmpdir(dir))
		dpref = dir;
	else if (is_appropriate_tmpdir(P_tmpdir))
		dpref = P_tmpdir;
	else
		dpref = "/";

	if (dpref[strlen(dpref) - 1] != '/')
		rc = asprintf(&buf, "%s/%sXXXXXX", dpref, pfx);
	else
		rc = asprintf(&buf, "%s%sXXXXXX", dpref, pfx);

	if (rc < 0)
		return NULL;

	rc = __tmpfile_templ(buf, false);
	if (rc != 0) {
		free(buf);
		return NULL;
	}

	return buf;
}

/** @}
 */
