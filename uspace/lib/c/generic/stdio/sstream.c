/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <stdio.h>
#include <str.h>
#include <errno.h>
#include <adt/list.h>
#include <uchar.h>
#include "../private/stdio.h"
#include "../private/sstream.h"

static size_t stdio_str_read(void *, size_t, size_t, FILE *);
static size_t stdio_str_write(const void *, size_t, size_t, FILE *);
static int stdio_str_flush(FILE *);

static __stream_ops_t stdio_str_ops = {
	.read = stdio_str_read,
	.write = stdio_str_write,
	.flush = stdio_str_flush
};

/** Read from string stream. */
static size_t stdio_str_read(void *buf, size_t size, size_t nmemb, FILE *stream)
{
	size_t nread;
	char *cp = (char *)stream->arg;
	char *bp = (char *)buf;

	nread = 0;
	while (nread < size * nmemb) {
		if (*cp == '\0') {
			stream->eof = true;
			break;
		}

		bp[nread] = *cp;
		++nread;
		++cp;
		stream->arg = (void *)cp;
	}

	return (nread / size);
}

/** Write to string stream. */
static size_t stdio_str_write(const void *buf, size_t size, size_t nmemb,
    FILE *stream)
{
	return 0;
}

/** Flush string stream. */
static int stdio_str_flush(FILE *stream)
{
	return EOF;
}

/** Initialize string stream.
 *
 * @param str String used as backend for reading
 * @param stream Stream to initialize
 */
void __sstream_init(const char *str, FILE *stream)
{
	memset(stream, 0, sizeof(FILE));
	stream->ops = &stdio_str_ops;
	stream->arg = (void *)str;
}

/** Return current string stream position.
 *
 * @param stream String stream
 * @return Pointer into the backing string at the current position
 */
const char *__sstream_getpos(FILE *stream)
{
	assert(stream->ops == &stdio_str_ops);
	return (char *) stream->arg;
}

/** @}
 */
