/*
 * SPDX-FileCopyrightText: 2014 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <mem.h>
#include <byteorder.h>
#include <stdlib.h>
#include "gzip.h"
#include "inflate.h"

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

/** Expand GZIP compressed data
 *
 * The routine allocates the output buffer based
 * on the size encoded in the input stream. This
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
errno_t gzip_expand(void *src, size_t srclen, void **dest, size_t *destlen)
{
	gzip_header_t header;
	gzip_footer_t footer;

	if ((srclen < sizeof(header)) || (srclen < sizeof(footer)))
		return EINVAL;

	/* Decode header and footer */

	memcpy(&header, src, sizeof(header));
	memcpy(&footer, src + srclen - sizeof(footer), sizeof(footer));

	if ((header.id1 != GZIP_ID1) ||
	    (header.id2 != GZIP_ID2) ||
	    (header.method != GZIP_METHOD_DEFLATE) ||
	    ((header.flags & (~GZIP_FLAGS_MASK)) != 0))
		return EINVAL;

	*destlen = uint32_t_le2host(footer.size);

	/* Ignore extra metadata */

	void *stream = src + sizeof(header);
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

	/* Allocate output buffer and inflate the data */

	*dest = malloc(*destlen);
	if (*dest == NULL)
		return ENOMEM;

	errno_t ret = inflate(stream, stream_length, *dest, *destlen);
	if (ret != EOK) {
		free(dest);
		return ret;
	}

	return EOK;
}
