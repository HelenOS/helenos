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
#include <string.h>
#include <errno.h>
#include <bool.h>
#include <malloc.h>
#include <io/klog.h>
#include <vfs/vfs.h>
#include <ipc/devmap.h>
#include <adt/list.h>

static void _fflushbuf(FILE *stream);

static FILE stdin_null = {
	.fd = -1,
	.error = true,
	.eof = true,
	.klog = false,
	.phone = -1,
	.btype = _IONBF,
	.buf = NULL,
	.buf_size = 0,
	.buf_head = NULL
};

static FILE stdout_klog = {
	.fd = -1,
	.error = false,
	.eof = false,
	.klog = true,
	.phone = -1,
	.btype = _IOLBF,
	.buf = NULL,
	.buf_size = BUFSIZ,
	.buf_head = NULL
};

static FILE stderr_klog = {
	.fd = -1,
	.error = false,
	.eof = false,
	.klog = true,
	.phone = -1,
	.btype = _IONBF,
	.buf = NULL,
	.buf_size = 0,
	.buf_head = NULL
};

FILE *stdin = NULL;
FILE *stdout = NULL;
FILE *stderr = NULL;

static LIST_INITIALIZE(files);

void __stdio_init(int filc, fdi_node_t *filv[])
{
	if (filc > 0) {
		stdin = fopen_node(filv[0], "r");
	} else {
		stdin = &stdin_null;
		list_append(&stdin->link, &files);
	}
	
	if (filc > 1) {
		stdout = fopen_node(filv[1], "w");
	} else {
		stdout = &stdout_klog;
		list_append(&stdout->link, &files);
	}
	
	if (filc > 2) {
		stderr = fopen_node(filv[2], "w");
	} else {
		stderr = &stderr_klog;
		list_append(&stderr->link, &files);
	}
}

void __stdio_done(void)
{
	link_t *link = files.next;
	
	while (link != &files) {
		FILE *file = list_get_instance(link, FILE, link);
		fclose(file);
		link = files.next;
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
	stream->klog = false;
	stream->phone = -1;
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
	stream->klog = false;
	stream->phone = -1;
	_setvbuf(stream);
	
	list_append(&stream->link, &files);
	
	return stream;
}

FILE *fopen_node(fdi_node_t *node, const char *mode)
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
	
	stream->fd = open_node(node, flags);
	if (stream->fd < 0) {
		/* errno was set by open_node() */
		free(stream);
		return NULL;
	}
	
	stream->error = false;
	stream->eof = false;
	stream->klog = false;
	stream->phone = -1;
	_setvbuf(stream);
	
	list_append(&stream->link, &files);
	
	return stream;
}

int fclose(FILE *stream)
{
	int rc = 0;
	
	fflush(stream);
	
	if (stream->phone >= 0)
		ipc_hangup(stream->phone);
	
	if (stream->fd >= 0)
		rc = close(stream->fd);
	
	list_remove(&stream->link);
	
	if ((stream != &stdin_null)
	    && (stream != &stdout_klog)
	    && (stream != &stderr_klog))
		free(stream);
	
	stream = NULL;
	
	if (rc != 0) {
		/* errno was set by close() */
		return EOF;
	}
	
	return 0;
}

/** Read from a stream.
 *
 * @param buf    Destination buffer.
 * @param size   Size of each record.
 * @param nmemb  Number of records to read.
 * @param stream Pointer to the stream.
 *
 */
size_t fread(void *buf, size_t size, size_t nmemb, FILE *stream)
{
	size_t left, done;

	if (size == 0 || nmemb == 0)
		return 0;

	/* Make sure no data is pending write. */
	_fflushbuf(stream);

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
		
		if (stream->klog)
			wr = klog_write(buf + done, left);
		else
			wr = write(stream->fd, buf + done, left);
		
		if (wr <= 0)
			stream->error = true;
		else {
			left -= wr;
			done += wr;
		}
	}
	
	return (done / size);
}

/** Drain stream buffer, do not sync stream. */
static void _fflushbuf(FILE *stream)
{
	size_t bytes_used;
	
	if ((!stream->buf) || (stream->btype == _IONBF) || (stream->error))
		return;
	
	bytes_used = stream->buf_head - stream->buf;
	if (bytes_used == 0)
		return;
	
	(void) _fwrite(stream->buf, 1, bytes_used, stream);
	stream->buf_head = stream->buf;
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
		
		buf += now;
		stream->buf_head += now;
		buf_free -= now;
		bytes_left -= now;
		total_written += now;
		
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
		size_t wr = fwrite(buf, sz, 1, stream);
		
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

int getchar(void)
{
	return fgetc(stdin);
}

int fseek(FILE *stream, long offset, int origin)
{
	off_t rc = lseek(stream->fd, offset, origin);
	if (rc == (off_t) (-1)) {
		/* errno has been set by lseek. */
		return -1;
	}
	
	stream->eof = false;
	
	return 0;
}

void rewind(FILE *stream)
{
	(void) fseek(stream, 0, SEEK_SET);
}

int fflush(FILE *stream)
{
	_fflushbuf(stream);
	
	if (stream->klog) {
		klog_update();
		return EOK;
	}
	
	if (stream->fd >= 0)
		return fsync(stream->fd);
	
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

int fphone(FILE *stream)
{
	if (stream->fd >= 0) {
		if (stream->phone < 0)
			stream->phone = fd_phone(stream->fd);
		
		return stream->phone;
	}
	
	return -1;
}

int fnode(FILE *stream, fdi_node_t *node)
{
	if (stream->fd >= 0)
		return fd_node(stream->fd, node);
	
	return ENOENT;
}

/** @}
 */
