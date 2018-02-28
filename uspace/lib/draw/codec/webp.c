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

/** @addtogroup draw
 * @{
 */
/**
 * @file
 */

#include <stdlib.h>
#include <byteorder.h>
#include <align.h>
#include <stdbool.h>
#include <pixconv.h>
#include <fourcc.h>
#include <stdint.h>
#include <abi/fourcc.h>
#include "webp.h"

/** Check for input buffer overrun condition */
#define CHECK_OVERRUN(state, retval) \
	do { \
		if ((state).overrun) \
			return (retval); \
	} while (false)

#define SIGNATURE_WEBP_LOSSLESS  UINT8_C(0x2f)

enum {
	FOURCC_RIFF          = FOURCC('R', 'I', 'F', 'F'),
	FOURCC_WEBP          = FOURCC('W', 'E', 'B', 'P'),
	FOURCC_WEBP_LOSSLESS = FOURCC('V', 'P', '8', 'L')
};

typedef enum {
	TRANSFORM_PREDICTOR      = 0,
	TRANSFORM_COLOR          = 1,
	TRANSFORM_SUBTRACT       = 2,
	TRANSFORM_COLOR_INDEXING = 3
} webp_transform_t;

typedef struct {
	fourcc_t fourcc;
	uint32_t payload_size;
} __attribute__((packed)) riff_header_t;

typedef struct {
	fourcc_t fourcc;
	fourcc_t encoding;
	uint32_t stream_size;
	uint8_t signature;
} __attribute__((packed)) webp_header_t;

typedef struct {
	uint32_t stream_size;
	uint16_t width;
	uint16_t height;
	bool alpha_used;
	uint8_t version;

	uint8_t *src;     /**< Input buffer */
	size_t srclen;    /**< Input buffer size */
	size_t srccnt;    /**< Position in the input buffer */

	uint32_t bitbuf;  /**< Bit buffer */
	size_t bitlen;    /**< Number of bits in the bit buffer */

	bool overrun;     /**< Overrun condition */
} webp_t;

/** Get bits from the bit buffer
 *
 * @param state WebP state.
 * @param cnt   Number of bits to return (at most 32).
 *
 * @return Returned bits.
 *
 */
static inline uint32_t get_bits(webp_t *state, size_t cnt)
{
	/* Bit accumulator for at least 36 bits */
	uint64_t val = state->bitbuf;

	while (state->bitlen < cnt) {
		if (state->srccnt == state->srclen) {
			state->overrun = true;
			return 0;
		}

		/* Load 8 more bits */
		val |= ((uint64_t) state->src[state->srccnt]) << state->bitlen;
		state->srccnt++;
		state->bitlen += 8;
	}

	/* Update bits in the buffer */
	state->bitbuf = (uint32_t) (val >> cnt);
	state->bitlen -= cnt;

	return ((uint32_t) (val & ((1 << cnt) - 1)));
}

/** Decode WebP header
 *
 * @param[in]  data Memory representation of WebP.
 * @param[in]  size Size of the representation (in bytes).
 * @param[out] webp Decoded WebP.
 *
 * @return True on succesful decoding.
 * @return False on failure.
 *
 */
static bool decode_webp_header(void *data, size_t size, webp_t *webp)
{
	/* Headers sanity check */
	if ((size < sizeof(riff_header_t)) ||
	    (size - sizeof(riff_header_t) < sizeof(webp_header_t)))
		return false;

	riff_header_t *riff_header = (riff_header_t *) data;
	if (riff_header->fourcc != FOURCC_RIFF)
		return false;

	/* Check payload size */
	size_t payload_size = uint32_t_le2host(riff_header->payload_size);
	if (payload_size + sizeof(riff_header_t) > size)
		return false;

	data += sizeof(riff_header_t);
	webp_header_t *webp_header = (webp_header_t *) data;
	if (webp_header->fourcc != FOURCC_WEBP)
		return false;

	/* Only lossless encoding supported so far */
	if (webp_header->encoding != FOURCC_WEBP_LOSSLESS)
		return false;

	webp->stream_size = uint32_t_le2host(webp_header->stream_size);
	if (webp->stream_size + sizeof(riff_header_t) +
	    sizeof(webp_header_t) > size)
		return false;

	if (webp_header->signature != SIGNATURE_WEBP_LOSSLESS)
		return false;

	data += sizeof(webp_header_t);

	/* Setup decoding state */
	webp->src = (uint8_t *) data;
	webp->srclen = webp->stream_size - 1;
	webp->srccnt = 0;
	webp->bitbuf = 0;
	webp->bitlen = 0;
	webp->overrun = false;

	/* Decode the rest of the metadata */
	webp->width = get_bits(webp, 14) + 1;
	CHECK_OVERRUN(*webp, false);

	webp->height = get_bits(webp, 14) + 1;
	CHECK_OVERRUN(*webp, false);

	webp->alpha_used = get_bits(webp, 1);
	CHECK_OVERRUN(*webp, false);

	webp->version = get_bits(webp, 3);
	CHECK_OVERRUN(*webp, false);

	if (webp->version != 0)
		return false;

	return true;
}

/** Decode WebP format
 *
 * Decode WebP format and create a surface from it. The supported
 * variants of WebP are currently limited to losslessly compressed
 * ARGB images.
 *
 * @param[in] data  Memory representation of WebP.
 * @param[in] size  Size of the representation (in bytes).
 * @param[in] flags Surface creation flags.
 *
 * @return Newly allocated surface with the decoded content.
 * @return NULL on error or unsupported format.
 *
 */
surface_t *decode_webp(void *data, size_t size, surface_flags_t flags)
{
	webp_t webp;
	if (!decode_webp_header(data, size, &webp))
		return NULL;

	bool transform_present = false;

	do {
		transform_present = get_bits(&webp, 1);
		CHECK_OVERRUN(webp, NULL);

		if (transform_present) {
			webp_transform_t transform = get_bits(&webp, 2);
			CHECK_OVERRUN(webp, NULL);

			if (transform == TRANSFORM_PREDICTOR) {
				// FIXME TODO
			} else
				return NULL;

			// FIXME: decode other transforms
		}
	} while (transform_present);

	// FIXME: decode image data

	return NULL;
}

/** Encode WebP format
 *
 * Encode WebP format into an array.
 *
 * @param[in]  surface Surface to be encoded into WebP.
 * @param[out] pdata   Pointer to the resulting array.
 * @param[out] psize   Pointer to the size of the resulting array.
 *
 * @return True on succesful encoding.
 * @return False on failure.
 *
 */
bool encode_webp(surface_t *surface, void **pdata, size_t *psize)
{
	// TODO
	return false;
}

/** @}
 */
