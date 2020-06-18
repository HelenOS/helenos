/*
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
