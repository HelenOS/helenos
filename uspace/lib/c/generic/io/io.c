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

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <str.h>
#include <errno.h>
#include <stdbool.h>
#include <malloc.h>
#include <async.h>
#include <io/kio.h>
#include <vfs/vfs.h>
#include <vfs/vfs_sess.h>
#include <ipc/loc.h>
#include <adt/list.h>
#include "../private/io.h"
#include "../private/stdio.h"

static void _ffillbuf(FILE *stream);
static void _fflushbuf(FILE *stream);

static FILE stdin_null = {
	.fd = -1,
	.error = true,
	.eof = true,
	.kio = false,
	.sess = NULL,
	.btype = _IONBF,
	.buf = NULL,
	.buf_size = 0,
	.buf_head = NULL,
	.buf_tail = NULL,
	.buf_state = _bs_empty
};

static FILE stdout_kio = {
	.fd = -1,
	.error = false,
	.eof = false,
	.kio = true,
	.sess = NULL,
	.btype = _IOLBF,
	.buf = NULL,
	.buf_size = BUFSIZ,
	.buf_head = NULL,
	.buf_tail = NULL,
	.buf_state = _bs_empty
};

static FILE stderr_kio = {
	.fd = -1,
	.error = false,
	.eof = false,
	.kio = true,
	.sess = NULL,
	.btype = _IONBF,
	.buf = NULL,
	.buf_size = 0,
	.buf_head = NULL,
	.buf_tail = NULL,
	.buf_state = _bs_empty
};

FILE *stdin = NULL;
FILE *stdout = NULL;
FILE *stderr = NULL;

static LIST_INITIALIZE(files);

void __stdio_init(int filc)
{
	if (filc > 0) {
		stdin = fdopen(0, "r");
	} else {
		stdin = &stdin_null;
		list_append(&stdin->link, &files);
	}
	
	if (filc > 1) {
		stdout = fdopen(1, "w");
	} else {
		stdout = &stdout_kio;
		list_append(&stdout->link, &files);
	}
	
	if (filc > 2) {
		stderr = fdopen(2, "w");
	} else {
		stderr = &stderr_kio;
		list_append(&stderr->link, &files);
	}
}

void __stdio_done(void)
{
	while (!list_empty(&files)) {
		FILE *file = list_get_instance(list_first(&files), FILE, link);
		fclose(file);
	}
}

static bool parse_mode(const char *mode, int *flags)
{
	/* Parse mode except first character. */
	const char *mp = mode;
	if (*mp++ == 0) {
		errno = EINVAL;
		return false;
	}
	
	if ((*mp == 'b') || (*mp == 't'))
		mp++;
	
	bool plus;
	if (*mp == '+') {
		mp++;
		plus = true;
	} else
		plus = false;
	
	if (*mp != 0) {
		errno = EINVAL;
		return false;
	}
	
	/* Parse first character of mode and determine flags for open(). */
	switch (mode[0]) {
	case 'r':
		*flags = plus ? O_RDWR : O_RDONLY;
		break;
	case 'w':
		*flags = (O_TRUNC | O_CREAT) | (plus ? O_RDWR : O_WRONLY);
		break;
	case 'a':
		/* TODO: a+ must read from beginning, append to the end. */
		if (plus) {
			errno = ENOTSUP;
			return false;
		}
		*flags = (O_APPEND | O_CREAT) | (plus ? O_RDWR : O_WRONLY);
		break;
	default:
		errno = EINVAL;
		return false;
	}
	
	return true;
}

/** Set stream buffer. */
void setvbuf(FILE *stream, void *buf, int mode, size_t size)
{
	stream->btype = mode;
	stream->buf = buf;
	stream->buf_size = size;
	stream->buf_head = stream->buf;
	stream->buf_tail = stream->buf;
	stream->buf_state = _bs_empty;
}

/** Set stream buffer.
 *
 * When @p buf is NULL, the stream is set as unbuffered, otherwise
 * full buffering is enabled.
 */
void setbuf(FILE *stream, void *buf)
{
	if (buf == NULL) {
		setvbuf(stream, NULL, _IONBF, BUFSIZ);
	} else {
		setvbuf(stream, buf, _IOFBF, BUFSIZ);
	}
}

static void _setvbuf(FILE *stream)
{
	/* FIXME: Use more complex rules for setting buffering options. */
	
	switch (stream->fd) {
	case 1:
		setvbuf(stream, NULL, _IOLBF, BUFSIZ);
		break;
	case 0:
	case 2:
		setvbuf(stream, NULL, _IONBF, 0);
		break;
	default:
		setvbuf(stream, NULL, _IOFBF, BUFSIZ);
	}
}

/** Allocate stream buffer. */
static int _fallocbuf(FILE *stream)
{
	assert(stream->buf == NULL);
	
	stream->buf = malloc(stream->buf_size);
	if (stream->buf == NULL) {
		errno = ENOMEM;
		return -1;
	}
	
	stream->buf_head = stream->buf;
	stream->buf_tail = stream->buf;
	return 0;
}

/** Open a stream.
 *
 * @param path Path of the file to open.
 * @param mode Mode string, (r|w|a)[b|t][+].
 *
 */
FILE *fopen(const char *path, const char *mode)
{
	int flags;
	if (!parse_mode(mode, &flags))
		return NULL;
	
	/* Open file. */
	FILE *stream = malloc(sizeof(FILE));
	if (stream == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	
	stream->fd = open(path, flags, 0666);
	if (stream->fd < 0) {
		/* errno was set by open() */
		free(stream);
		return NULL;
	}
	
	stream->error = false;
	stream->eof = false;
	stream->kio = false;
	stream->sess = NULL;
	stream->need_sync = false;
	_setvbuf(stream);
	
	list_append(&stream->link, &files);
	
	return stream;
}

FILE *fdopen(int fd, const char *mode)
{
	/* Open file. */
	FILE *stream = malloc(sizeof(FILE));
	if (stream == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	
	stream->fd = fd;
	stream->error = false;
	stream->eof = false;
	stream->kio = false;
	stream->sess = NULL;
	stream->need_sync = false;
	_setvbuf(stream);
	
	list_append(&stream->link, &files);
	
	return stream;
}

int fclose(FILE *stream)
{
	int rc = 0;
	
	fflush(stream);
	
	if (stream->sess != NULL)
		async_hangup(stream->sess);
	
	if (stream->fd >= 0)
		rc = close(stream->fd);
	
	list_remove(&stream->link);
	
	if ((stream != &stdin_null)
	    && (stream != &stdout_kio)
	    && (stream != &stderr_kio))
		free(stream);
	
	stream = NULL;
	
	if (rc != 0) {
		/* errno was set by close() */
		return EOF;
	}
	
	return 0;
}

/** Read from a stream (unbuffered).
 *
 * @param buf    Destination buffer.
 * @param size   Size of each record.
 * @param nmemb  Number of records to read.
 * @param stream Pointer to the stream.
 */
static size_t _fread(void *buf, size_t size, size_t nmemb, FILE *stream)
{
	size_t left, done;

	if (size == 0 || nmemb == 0)
		return 0;

	left = size * nmemb;
	done = 0;
	
	while ((left > 0) && (!stream->error) && (!stream->eof)) {
		ssize_t rd = read(stream->fd, buf + done, left);
		
		if (rd < 0)
			stream->error = true;
		else if (rd == 0)
			stream->eof = true;
		else {
			left -= rd;
			done += rd;
		}
	}
	
	return (done / size);
}

/** Write to a stream (unbuffered).
 *
 * @param buf    Source buffer.
 * @param size   Size of each record.
 * @param nmemb  Number of records to write.
 * @param stream Pointer to the stream.
 */
static size_t _fwrite(const void *buf, size_t size, size_t nmemb, FILE *stream)
{
	size_t left;
	size_t done;

	if (size == 0 || nmemb == 0)
		return 0;

	left = size * nmemb;
	done = 0;

	while ((left > 0) && (!stream->error)) {
		ssize_t wr;
		
		if (stream->kio)
			wr = kio_write(buf + done, left);
		else
			wr = write(stream->fd, buf + done, left);
		
		if (wr <= 0)
			stream->error = true;
		else {
			left -= wr;
			done += wr;
		}
	}

	if (done > 0)
		stream->need_sync = true;
	
	return (done / size);
}

/** Read some data in stream buffer. */
static void _ffillbuf(FILE *stream)
{
	ssize_t rc;

	stream->buf_head = stream->buf_tail = stream->buf;

	rc = read(stream->fd, stream->buf, stream->buf_size);
	if (rc < 0) {
		stream->error = true;
		return;
	}

	if (rc == 0) {
		stream->eof = true;
		return;
	}

	stream->buf_head += rc;
	stream->buf_state = _bs_read;
}

/** Write out stream buffer, do not sync stream. */
static void _fflushbuf(FILE *stream)
{
	size_t bytes_used;

	if ((!stream->buf) || (stream->btype == _IONBF) || (stream->error))
		return;

	bytes_used = stream->buf_head - stream->buf_tail;

	/* If buffer has prefetched read data, we need to seek back. */
	if (bytes_used > 0 && stream->buf_state == _bs_read)
		lseek(stream->fd, - (ssize_t) bytes_used, SEEK_CUR);

	/* If buffer has unwritten data, we need to write them out. */
	if (bytes_used > 0 && stream->buf_state == _bs_write)
		(void) _fwrite(stream->buf_tail, 1, bytes_used, stream);

	stream->buf_head = stream->buf;
	stream->buf_tail = stream->buf;
	stream->buf_state = _bs_empty;
}

/** Read from a stream.
 *
 * @param dest   Destination buffer.
 * @param size   Size of each record.
 * @param nmemb  Number of records to read.
 * @param stream Pointer to the stream.
 *
 */
size_t fread(void *dest, size_t size, size_t nmemb, FILE *stream)
{
	uint8_t *dp;
	size_t bytes_left;
	size_t now;
	size_t data_avail;
	size_t total_read;
	size_t i;

	if (size == 0 || nmemb == 0)
		return 0;

	/* If not buffered stream, read in directly. */
	if (stream->btype == _IONBF) {
		now = _fread(dest, size, nmemb, stream);
		return now;
	}

	/* Make sure no data is pending write. */
	if (stream->buf_state == _bs_write)
		_fflushbuf(stream);

	/* Perform lazy allocation of stream buffer. */
	if (stream->buf == NULL) {
		if (_fallocbuf(stream) != 0)
			return 0; /* Errno set by _fallocbuf(). */
	}

	bytes_left = size * nmemb;
	total_read = 0;
	dp = (uint8_t *) dest;

	while ((!stream->error) && (!stream->eof) && (bytes_left > 0)) {
		if (stream->buf_head == stream->buf_tail)
			_ffillbuf(stream);

		if (stream->error || stream->eof)
			break;

		data_avail = stream->buf_head - stream->buf_tail;

		if (bytes_left > data_avail)
			now = data_avail;
		else
			now = bytes_left;

		for (i = 0; i < now; i++) {
			dp[i] = stream->buf_tail[i];
		}

		dp += now;
		stream->buf_tail += now;
		bytes_left -= now;
		total_read += now;
	}

	return (total_read / size);
}


/** Write to a stream.
 *
 * @param buf    Source buffer.
 * @param size   Size of each record.
 * @param nmemb  Number of records to write.
 * @param stream Pointer to the stream.
 *
 */
size_t fwrite(const void *buf, size_t size, size_t nmemb, FILE *stream)
{
	uint8_t *data;
	size_t bytes_left;
	size_t now;
	size_t buf_free;
	size_t total_written;
	size_t i;
	uint8_t b;
	bool need_flush;

	if (size == 0 || nmemb == 0)
		return 0;

	/* If not buffered stream, write out directly. */
	if (stream->btype == _IONBF) {
		now = _fwrite(buf, size, nmemb, stream);
		fflush(stream);
		return now;
	}

	/* Make sure buffer contains no prefetched data. */
	if (stream->buf_state == _bs_read)
		_fflushbuf(stream);


	/* Perform lazy allocation of stream buffer. */
	if (stream->buf == NULL) {
		if (_fallocbuf(stream) != 0)
			return 0; /* Errno set by _fallocbuf(). */
	}
	
	data = (uint8_t *) buf;
	bytes_left = size * nmemb;
	total_written = 0;
	need_flush = false;
	
	while ((!stream->error) && (bytes_left > 0)) {
		buf_free = stream->buf_size - (stream->buf_head - stream->buf);
		if (bytes_left > buf_free)
			now = buf_free;
		else
			now = bytes_left;
		
		for (i = 0; i < now; i++) {
			b = data[i];
			stream->buf_head[i] = b;
			
			if ((b == '\n') && (stream->btype == _IOLBF))
				need_flush = true;
		}
		
		data += now;
		stream->buf_head += now;
		buf_free -= now;
		bytes_left -= now;
		total_written += now;
		stream->buf_state = _bs_write;
		
		if (buf_free == 0) {
			/* Only need to drain buffer. */
			_fflushbuf(stream);
			need_flush = false;
		}
	}

	if (need_flush)
		fflush(stream);
	
	return (total_written / size);
}

int fputc(wchar_t c, FILE *stream)
{
	char buf[STR_BOUNDS(1)];
	size_t sz = 0;
	
	if (chr_encode(c, buf, &sz, STR_BOUNDS(1)) == EOK) {
		size_t wr = fwrite(buf, 1, sz, stream);
		
		if (wr < sz)
			return EOF;
		
		return (int) c;
	}
	
	return EOF;
}

int putchar(wchar_t c)
{
	return fputc(c, stdout);
}

int fputs(const char *str, FILE *stream)
{
	return fwrite(str, str_size(str), 1, stream);
}

int puts(const char *str)
{
	return fputs(str, stdout);
}

int fgetc(FILE *stream)
{
	char c;
	
	/* This could be made faster by only flushing when needed. */
	if (stdout)
		fflush(stdout);
	if (stderr)
		fflush(stderr);
	
	if (fread(&c, sizeof(char), 1, stream) < sizeof(char))
		return EOF;
	
	return (int) c;
}

char *fgets(char *str, int size, FILE *stream)
{
	int c;
	int idx;

	idx = 0;
	while (idx < size - 1) {
		c = fgetc(stream);
		if (c == EOF)
			break;

		str[idx++] = c;

		if (c == '\n')
			break;
	}

	if (ferror(stream))
		return NULL;

	if (idx == 0)
		return NULL;

	str[idx] = '\0';
	return str;
}

int getchar(void)
{
	return fgetc(stdin);
}

int fseek(FILE *stream, off64_t offset, int whence)
{
	off64_t rc;

	_fflushbuf(stream);

	rc = lseek(stream->fd, offset, whence);
	if (rc == (off64_t) (-1)) {
		/* errno has been set by lseek64. */
		return -1;
	}

	stream->eof = false;
	return 0;
}

off64_t ftell(FILE *stream)
{
	_fflushbuf(stream);
	return lseek(stream->fd, 0, SEEK_CUR);
}

void rewind(FILE *stream)
{
	(void) fseek(stream, 0, SEEK_SET);
}

int fflush(FILE *stream)
{
	_fflushbuf(stream);
	
	if (stream->kio) {
		kio_update();
		return EOK;
	}
	
	if ((stream->fd >= 0) && (stream->need_sync)) {
		/**
		 * Better than syncing always, but probably still not the
		 * right thing to do.
		 */
		stream->need_sync = false;
		return fsync(stream->fd);
	}
	
	return ENOENT;
}

int feof(FILE *stream)
{
	return stream->eof;
}

int ferror(FILE *stream)
{
	return stream->error;
}

void clearerr(FILE *stream)
{
	stream->eof = false;
	stream->error = false;
}

int fileno(FILE *stream)
{
	if (stream->kio) {
		errno = EBADF;
		return -1;
	}
	
	return stream->fd;
}

async_sess_t *fsession(exch_mgmt_t mgmt, FILE *stream)
{
	if (stream->fd >= 0) {
		if (stream->sess == NULL)
			stream->sess = fd_session(mgmt, stream->fd);
		
		return stream->sess;
	}
	
	return NULL;
}

int fhandle(FILE *stream, int *handle)
{
	if (stream->fd >= 0) {
		*handle = stream->fd;
		return EOK;
	}
	
	return ENOENT;
}

/** @}
 */
