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
#include <assert.h>
#include <str.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <async.h>
#include <io/kio.h>
#include <vfs/vfs.h>
#include <vfs/vfs_sess.h>
#include <vfs/inbox.h>
#include <ipc/loc.h>
#include <adt/list.h>
#include "../private/io.h"
#include "../private/stdio.h"

static void _ffillbuf(FILE *stream);
static void _fflushbuf(FILE *stream);

static FILE stdin_null = {
	.fd = -1,
	.pos = 0,
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
	.pos = 0,
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
	.pos = 0,
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

void __stdio_init(void)
{
	/* The first three standard file descriptors are assigned for compatibility.
	 * This will probably be removed later.
	 */
	int infd = inbox_get("stdin");
	if (infd >= 0) {
		int stdinfd = -1;
		(void) vfs_clone(infd, -1, false, &stdinfd);
		assert(stdinfd == 0);
		vfs_open(stdinfd, MODE_READ);
		stdin = fdopen(stdinfd, "r");
	} else {
		stdin = &stdin_null;
		list_append(&stdin->link, &files);
	}

	int outfd = inbox_get("stdout");
	if (outfd >= 0) {
		int stdoutfd = -1;
		(void) vfs_clone(outfd, -1, false, &stdoutfd);
		assert(stdoutfd <= 1);
		while (stdoutfd < 1)
			(void) vfs_clone(outfd, -1, false, &stdoutfd);
		vfs_open(stdoutfd, MODE_APPEND);
		stdout = fdopen(stdoutfd, "a");
	} else {
		stdout = &stdout_kio;
		list_append(&stdout->link, &files);
	}

	int errfd = inbox_get("stderr");
	if (errfd >= 0) {
		int stderrfd = -1;
		(void) vfs_clone(errfd, -1, false, &stderrfd);
		assert(stderrfd <= 2);
		while (stderrfd < 2)
			(void) vfs_clone(errfd, -1, false, &stderrfd);
		vfs_open(stderrfd, MODE_APPEND);
		stderr = fdopen(stderrfd, "a");
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

static bool parse_mode(const char *fmode, int *mode, bool *create, bool *truncate)
{
	/* Parse mode except first character. */
	const char *mp = fmode;
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

	*create = false;
	*truncate = false;

	/* Parse first character of fmode and determine mode for vfs_open(). */
	switch (fmode[0]) {
	case 'r':
		*mode = plus ? MODE_READ | MODE_WRITE : MODE_READ;
		break;
	case 'w':
		*mode = plus ? MODE_READ | MODE_WRITE : MODE_WRITE;
		*create = true;
		if (!plus)
			*truncate = true;
		break;
	case 'a':
		/* TODO: a+ must read from beginning, append to the end. */
		if (plus) {
			errno = ENOTSUP;
			return false;
		}

		*mode = MODE_APPEND | (plus ? MODE_READ | MODE_WRITE : MODE_WRITE);
		*create = true;
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
		return EOF;
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
FILE *fopen(const char *path, const char *fmode)
{
	int mode;
	bool create;
	bool truncate;

	if (!parse_mode(fmode, &mode, &create, &truncate))
		return NULL;

	/* Open file. */
	FILE *stream = malloc(sizeof(FILE));
	if (stream == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	int flags = WALK_REGULAR;
	if (create)
		flags |= WALK_MAY_CREATE;
	int file;
	errno_t rc = vfs_lookup(path, flags, &file);
	if (rc != EOK) {
		errno = rc;
		free(stream);
		return NULL;
	}

	rc = vfs_open(file, mode);
	if (rc != EOK) {
		errno = rc;
		vfs_put(file);
		free(stream);
		return NULL;
	}

	if (truncate) {
		rc = vfs_resize(file, 0);
		if (rc != EOK) {
			errno = rc;
			vfs_put(file);
			free(stream);
			return NULL;
		}
	}

	stream->fd = file;
	stream->pos = 0;
	stream->error = false;
	stream->eof = false;
	stream->kio = false;
	stream->sess = NULL;
	stream->need_sync = false;
	_setvbuf(stream);
	stream->ungetc_chars = 0;

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
	stream->pos = 0;
	stream->error = false;
	stream->eof = false;
	stream->kio = false;
	stream->sess = NULL;
	stream->need_sync = false;
	_setvbuf(stream);
	stream->ungetc_chars = 0;

	list_append(&stream->link, &files);

	return stream;
}


static int _fclose_nofree(FILE *stream)
{
	errno_t rc = 0;

	fflush(stream);

	if (stream->sess != NULL)
		async_hangup(stream->sess);

	if (stream->fd >= 0)
		rc = vfs_put(stream->fd);

	list_remove(&stream->link);

	if (rc != EOK) {
		errno = rc;
		return EOF;
	}

	return 0;
}

int fclose(FILE *stream)
{
	int rc = _fclose_nofree(stream);

	if ((stream != &stdin_null)
	    && (stream != &stdout_kio)
	    && (stream != &stderr_kio))
		free(stream);

	return rc;
}

FILE *freopen(const char *path, const char *mode, FILE *stream)
{
	FILE *nstr;

	if (path == NULL) {
		/* Changing mode is not supported */
		return NULL;
	}

	(void) _fclose_nofree(stream);
	nstr = fopen(path, mode);
	if (nstr == NULL) {
		free(stream);
		return NULL;
	}

	list_remove(&nstr->link);
	*stream = *nstr;
	list_append(&stream->link, &files);

	free(nstr);

	return stream;
}

/** Read from a stream (unbuffered).
 *
 * @param buf    Destination buffer.
 * @param size   Size of each record.
 * @param nmemb  Number of records to read.
 * @param stream Pointer to the stream.
 *
 * @return Number of elements successfully read. On error this is less than
 *         nmemb, stream error indicator is set and errno is set.
 */
static size_t _fread(void *buf, size_t size, size_t nmemb, FILE *stream)
{
	errno_t rc;
	size_t nread;

	if (size == 0 || nmemb == 0)
		return 0;

	rc = vfs_read(stream->fd, &stream->pos, buf, size * nmemb, &nread);
	if (rc != EOK) {
		errno = rc;
		stream->error = true;
	} else if (nread == 0) {
		stream->eof = true;
	}

	return (nread / size);
}

/** Write to a stream (unbuffered).
 *
 * @param buf    Source buffer.
 * @param size   Size of each record.
 * @param nmemb  Number of records to write.
 * @param stream Pointer to the stream.
 *
 * @return Number of elements successfully written. On error this is less than
 *         nmemb, stream error indicator is set and errno is set.
 */
static size_t _fwrite(const void *buf, size_t size, size_t nmemb, FILE *stream)
{
	errno_t rc;
	size_t nwritten;

	if (size == 0 || nmemb == 0)
		return 0;

	if (stream->kio) {
		rc = kio_write(buf, size * nmemb, &nwritten);
		if (rc != EOK) {
			stream->error = true;
			nwritten = 0;
		}
	} else {
		rc = vfs_write(stream->fd, &stream->pos, buf, size * nmemb,
		    &nwritten);
		if (rc != EOK) {
			errno = rc;
			stream->error = true;
		}
	}

	if (nwritten > 0)
		stream->need_sync = true;

	return (nwritten / size);
}

/** Read some data in stream buffer.
 *
 * On error, stream error indicator is set and errno is set.
 */
static void _ffillbuf(FILE *stream)
{
	errno_t rc;
	size_t nread;

	stream->buf_head = stream->buf_tail = stream->buf;

	rc = vfs_read(stream->fd, &stream->pos, stream->buf, stream->buf_size,
	    &nread);
	if (rc != EOK) {
		errno = rc;
		stream->error = true;
		return;
	}

	if (nread == 0) {
		stream->eof = true;
		return;
	}

	stream->buf_head += nread;
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
		stream->pos -= bytes_used;

	/* If buffer has unwritten data, we need to write them out. */
	if (bytes_used > 0 && stream->buf_state == _bs_write) {
		(void) _fwrite(stream->buf_tail, 1, bytes_used, stream);
		/* On error stream error indicator and errno are set by _fwrite */
		if (stream->error)
			return;
	}

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

	bytes_left = size * nmemb;
	total_read = 0;
	dp = (uint8_t *) dest;

	/* Bytes from ungetc() buffer */
	while (stream->ungetc_chars > 0 && bytes_left > 0) {
		*dp++ = stream->ungetc_buf[--stream->ungetc_chars];
		++total_read;
		--bytes_left;
	}

	/* If not buffered stream, read in directly. */
	if (stream->btype == _IONBF) {
		total_read += _fread(dest, 1, bytes_left, stream);
		return total_read / size;
	}

	/* Make sure no data is pending write. */
	if (stream->buf_state == _bs_write)
		_fflushbuf(stream);

	/* Perform lazy allocation of stream buffer. */
	if (stream->buf == NULL) {
		if (_fallocbuf(stream) != 0)
			return 0; /* Errno set by _fallocbuf(). */
	}

	while ((!stream->error) && (!stream->eof) && (bytes_left > 0)) {
		if (stream->buf_head == stream->buf_tail)
			_ffillbuf(stream);

		if (stream->error || stream->eof) {
			/* On error errno was set by _ffillbuf() */
			break;
		}

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
			if (!stream->error)
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
	(void) fwrite(str, str_size(str), 1, stream);
	if (ferror(stream))
		return EOF;
	return 0;
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

int ungetc(int c, FILE *stream)
{
	if (c == EOF)
		return EOF;

	if (stream->ungetc_chars >= UNGETC_MAX)
		return EOF;

	stream->ungetc_buf[stream->ungetc_chars++] =
	    (uint8_t)c;

	stream->eof = false;
	return (uint8_t)c;
}

int fseek64(FILE *stream, off64_t offset, int whence)
{
	errno_t rc;

	if (stream->error)
		return -1;

	_fflushbuf(stream);
	if (stream->error) {
		/* errno was set by _fflushbuf() */
		return -1;
	}

	stream->ungetc_chars = 0;

	vfs_stat_t st;
	switch (whence) {
	case SEEK_SET:
		stream->pos = offset;
		break;
	case SEEK_CUR:
		stream->pos += offset;
		break;
	case SEEK_END:
		rc = vfs_stat(stream->fd, &st);
		if (rc != EOK) {
			errno = rc;
			stream->error = true;
			return -1;
		}
		stream->pos = st.size + offset;
		break;
	}

	stream->eof = false;
	return 0;
}

off64_t ftell64(FILE *stream)
{
	if (stream->error)
		return EOF;

	_fflushbuf(stream);
	if (stream->error) {
		/* errno was set by _fflushbuf() */
		return EOF;
	}

	return stream->pos - stream->ungetc_chars;
}

int fseek(FILE *stream, long offset, int whence)
{
	return fseek64(stream, offset, whence);
}

long ftell(FILE *stream)
{
	off64_t off = ftell64(stream);

	/* The native position is too large for the C99-ish interface. */
	if (off > LONG_MAX)
		return EOF;

	return off;
}

void rewind(FILE *stream)
{
	(void) fseek(stream, 0, SEEK_SET);
}

int fflush(FILE *stream)
{
	if (stream->error)
		return EOF;

	_fflushbuf(stream);
	if (stream->error) {
		/* errno was set by _fflushbuf() */
		return EOF;
	}

	if (stream->kio) {
		kio_update();
		return 0;
	}

	if ((stream->fd >= 0) && (stream->need_sync)) {
		errno_t rc;

		/**
		 * Better than syncing always, but probably still not the
		 * right thing to do.
		 */
		stream->need_sync = false;
		rc = vfs_sync(stream->fd);
		if (rc != EOK) {
			errno = rc;
			return EOF;
		}

		return 0;
	}

	return 0;
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
		return EOF;
	}

	return stream->fd;
}

async_sess_t *vfs_fsession(FILE *stream, iface_t iface)
{
	if (stream->fd >= 0) {
		if (stream->sess == NULL)
			stream->sess = vfs_fd_session(stream->fd, iface);

		return stream->sess;
	}

	return NULL;
}

errno_t vfs_fhandle(FILE *stream, int *handle)
{
	if (stream->fd >= 0) {
		*handle = stream->fd;
		return EOK;
	}

	return ENOENT;
}

/** @}
 */
