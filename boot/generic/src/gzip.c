/*
 * Copyright (c) 2014 Martin Decky
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

// XXX: This file is a duplicate of the same in uspace/lib/compress

#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <mem.h>
#include <byteorder.h>
#include <gzip.h>
#include <inflate.h>

#define GZIP_ID1  UINT8_C(0x1f)
#define GZIP_ID2  UINT8_C(0x8b)

#define GZIP_METHOD_DEFLATE  UINT8_C(0x08)

#define GZIP_FLAGS_MASK     UINT8_C(0x1f)
#define GZIP_FLAG_FHCRC     UINT8_C(1 << 1)
#define GZIP_FLAG_FEXTRA    UINT8_C(1 << 2)
#define GZIP_FLAG_FNAME     UINT8_C(1 << 3)
#define GZIP_FLAG_FCOMMENT  UINT8_C(1 << 4)

typedef struct {
	uint8_t id1;
	uint8_t id2;
	uint8_t method;
	uint8_t flags;
	uint32_t mtime;
	uint8_t extra_flags;
	uint8_t os;
} __attribute__((packed)) gzip_header_t;

typedef struct {
	uint32_t crc32;
	uint32_t size;
} __attribute__((packed)) gzip_footer_t;

/** Check GZIP signature
 *
 * Checks whether the source buffer start with a GZIP signature.
 *
 * @param[in] src    Source data buffer.
 * @param[in] srclen Source buffer size (bytes).
 *
 * @return True if GZIP signature found.
 * @return False if no GZIP signature found.
 *
 */
bool gzip_check(const void *src, size_t srclen)
{
	if ((srclen < (sizeof(gzip_header_t) + sizeof(gzip_footer_t))))
		return false;

	gzip_header_t header;
	memcpy(&header, src, sizeof(header));

	if ((header.id1 != GZIP_ID1) ||
	    (header.id2 != GZIP_ID2) ||
	    (header.method != GZIP_METHOD_DEFLATE) ||
	    ((header.flags & (~GZIP_FLAGS_MASK)) != 0))
		return false;

	return true;
}

/** Get uncompressed size
 *
 * Note that the uncompressed size is read from the GZIP footer
 * (and not calculated by acutally decompressing the GZip archive).
 * Thus the source of the GZip archive needs to be trusted.
 *
 * @param[in]  src    Source data buffer.
 * @param[out] srclen Source buffer size (bytes).
 *
 * @return Uncompressed size.
 *
 */
size_t gzip_size(const void *src, size_t srclen)
{
	if (!gzip_check(src, srclen))
		return 0;

	gzip_footer_t footer;
	memcpy(&footer, src + srclen - sizeof(footer), sizeof(footer));

	return uint32_t_le2host(footer.size);
}

/** Expand GZIP compressed data
 *
 * The routine compares the output buffer size with
 * the size encoded in the input stream. This
 * effectively limits the size of the uncompressed
 * data to 4 GiB (expanding input streams that actually
 * encode more data will always fail).
 *
 * So far, no CRC is perfomed.
 *
 * @param[in]  src     Source data buffer.
 * @param[in]  srclen  Source buffer size (bytes).
 * @param[out] dest    Destination data buffer.
 * @param[out] destlen Destination buffer size (bytes).
 *
 * @return EOK on success.
 * @return ENOENT on distance too large.
 * @return EINVAL on invalid Huffman code, invalid deflate data,
 *                   invalid compression method or invalid stream.
 * @return ELIMIT on input buffer overrun.
 * @return ENOMEM on output buffer overrun.
 *
 */
int gzip_expand(const void *src, size_t srclen, void *dest, size_t destlen)
{
	if (!gzip_check(src, srclen))
		return EINVAL;

	/* Decode header and footer */

	gzip_header_t header;
	memcpy(&header, src, sizeof(header));

	gzip_footer_t footer;
	memcpy(&footer, src + srclen - sizeof(footer), sizeof(footer));

	if (destlen != uint32_t_le2host(footer.size))
		return EINVAL;

	/* Ignore extra metadata */

	const void *stream = src + sizeof(header);
	size_t stream_length = srclen - sizeof(header) - sizeof(footer);

	if ((header.flags & GZIP_FLAG_FEXTRA) != 0) {
		uint16_t extra_length;

		if (stream_length < sizeof(extra_length))
			return EINVAL;

		memcpy(&extra_length, stream, sizeof(extra_length));
		stream += sizeof(extra_length);
		stream_length -= sizeof(extra_length);

		if (stream_length < extra_length)
			return EINVAL;

		stream += extra_length;
		stream_length -= extra_length;
	}

	if ((header.flags & GZIP_FLAG_FNAME) != 0) {
		while (*((uint8_t *) stream) != 0) {
			if (stream_length == 0)
				return EINVAL;

			stream++;
			stream_length--;
		}

		if (stream_length == 0)
			return EINVAL;

		stream++;
		stream_length--;
	}

	if ((header.flags & GZIP_FLAG_FCOMMENT) != 0) {
		while (*((uint8_t *) stream) != 0) {
			if (stream_length == 0)
				return EINVAL;

			stream++;
			stream_length--;
		}

		if (stream_length == 0)
			return EINVAL;

		stream++;
		stream_length--;
	}

	if ((header.flags & GZIP_FLAG_FHCRC) != 0) {
		if (stream_length < 2)
			return EINVAL;

		stream += 2;
		stream_length -= 2;
	}

	/* Inflate the data */
	return inflate(stream, stream_length, dest, destlen);
}
