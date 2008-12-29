/*
 * Copyright (c) 2008 Jiri Svoboda
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
/**
 * @file
 * @brief ANSI C Stream I/O.
 */ 

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <bool.h>
#include <stdio.h>

/**
 * Open a stream.
 *
 * @param file_name	Name of the file to open.
 * @param mode		Mode string, (r|w|a)[b|t][+].
 */
FILE *fopen(const char *file_name, const char *mode)
{
	FILE *f;
	int flags;
	bool plus;
	const char *mp;

	/* Parse mode except first character. */

	mp = mode;
	if (*mp++ == '\0') {
		errno = EINVAL;
		return NULL;
	}

	if (*mp == 'b' || *mp == 't') ++mp;

	if (*mp == '+') {
		++mp;
		plus = true;
	} else {
		plus = false;
	}

	if (*mp != '\0') {
		errno = EINVAL;
		return NULL;
	}

	/* Parse first character of mode and determine flags for open(). */

	switch (mode[0]) {
	case 'r':
		flags = plus ? O_RDWR : O_RDONLY;
		break;
	case 'w':
		flags = (O_TRUNC | O_CREAT) | (plus ? O_RDWR : O_WRONLY);
		break;
	case 'a':
		/* TODO: a+ must read from beginning, append to the end. */
		if (plus) {
			errno = ENOTSUP;
			return NULL;
		}
		flags = (O_APPEND | O_CREAT) | (plus ? O_RDWR : O_WRONLY);
	default:
		errno = EINVAL;
		return NULL;
	}

	/* Open file. */
	
	f = malloc(sizeof(FILE));
	if (f == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	f->fd = open(file_name, flags, 0666);
	if (f->fd < 0) {
		free(f);
		return NULL; /* errno was set by open() */
	}

	f->error = 0;
	f->eof = 0;

	return f;
}

/** Close a stream.
 *
 * @param f	Pointer to stream.
 * @return	0 on success, EOF on error.
 */
int fclose(FILE *f)
{
	int rc;

	rc = close(f->fd);
	free(f);

	if (rc != 0)
		return EOF; /* errno was set by close() */

	return 0;
}

/** Read from a stream.
 *
 * @param buf	Destination buffer.
 * @param size	Size of each record.
 * @param nmemb Number of records to read.
 * @param f	Pointer to the stream.
 */
size_t fread(void *buf, size_t size, size_t nmemb, FILE *f)
{
	size_t left, done, n;

	left = size * nmemb;
	done = 0;

	while (left > 0 && !f->error && !f->eof) {
		n = read(f->fd, buf + done, left);

		if (n < 0) {
			f->error = 1;
		} else if (n == 0) {
			f->eof = 1;
		} else {
			left -= n;
			done += n;
		}
	}

	return done / size;
}


/** Write to a stream.
 *
 * @param buf	Source buffer.
 * @param size	Size of each record.
 * @param nmemb Number of records to write.
 * @param f	Pointer to the stream.
 */
size_t fwrite(const void *buf, size_t size, size_t nmemb, FILE *f)
{
	size_t left, done, n;

	left = size * nmemb;
	done = 0;

	while (left > 0 && !f->error) {
		n = write(f->fd, buf + done, left);

		if (n <= 0) {
			f->error = 1;
		} else {
			left -= n;
			done += n;
		}
	} 

	return done / size;
}

/** Return the end-of-file indicator of a stream. */
int feof(FILE *f)
{
	return f->eof;
}

/** Return the error indicator of a stream. */
int ferror(FILE *f)
{
	return f->error;
}

/** Clear the error and end-of-file indicators of a stream. */
void clearerr(FILE *f)
{
	f->eof = 0;
	f->error = 0;
}

/** Read character from a stream. */
int fgetc(FILE *f)
{
	unsigned char c;
	size_t n;

	n = fread(&c, sizeof(c), 1, f);
	if (n < 1) return EOF;

	return (int) c;
}

/** Write character to a stream. */
int fputc(int c, FILE *f)
{
	unsigned char cc;
	size_t n;

	cc = (unsigned char) c;
	n = fwrite(&cc, sizeof(cc), 1, f);
	if (n < 1) return EOF;

	return (int) cc;
}

/** Write string to a stream. */
int fputs(const char *s, FILE *f)
{
	int rc;

	rc = 0;

	while (*s && rc >= 0) {
		rc = fputc(*s++, f);
	}

	if (rc < 0) return EOF;

	return 0;
}

/** Seek to position in stream. */
int fseek(FILE *f, long offset, int origin)
{
	off_t rc;

	rc = lseek(f->fd, offset, origin);
	if (rc == (off_t) (-1)) {
		/* errno has been set by lseek. */
		return -1;
	}

	f->eof = 0;

	return 0;
}

/** @}
 */
