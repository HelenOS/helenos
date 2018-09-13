/*
 * Copyright (c) 2010 Mark Adler
 * Copyright (c) 2010 Martin Decky
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

/** @file
 * @brief Implementation of inflate decompression
 *
 * A simple inflate implementation (decompression of `deflate' stream as
 * described by RFC 1951) based on puff.c by Mark Adler. This code is
 * currently not optimized for speed but for readability.
 *
 * All dynamically allocated memory memory is taken from the stack. The
 * stack usage should be typically bounded by 2 KB.
 *
 * Original copyright notice:
 *
 *  Copyright (C) 2002-2010 Mark Adler, all rights reserved
 *  version 2.1, 4 Apr 2010
 *
 *  This software is provided 'as-is', without any express or implied
 *  warranty. In no event will the author be held liable for any damages
 *  arising from the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *
 *  1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 *  2. Altered source versions must be plainly marked as such, and must not
 *     be misrepresented as being the original software.
 *  3. This notice may not be removed or altered from any source
 *     distribution.
 *
 *   Mark Adler <madler@alumni.caltech.edu>
 *
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <mem.h>
#include "inflate.h"

/** Maximum bits in the Huffman code */
#define MAX_HUFFMAN_BIT  15

/** Number of length codes */
#define MAX_LEN           29
/** Number of distance codes */
#define MAX_DIST          30
/** Number of order codes */
#define MAX_ORDER         19
/** Number of literal/length codes */
#define MAX_LITLEN        286
/** Number of fixed literal/length codes */
#define MAX_FIXED_LITLEN  288

/** Number of all codes */
#define MAX_CODE  (MAX_LITLEN + MAX_DIST)

/** Check for input buffer overrun condition */
#define CHECK_OVERRUN(state) \
	do { \
		if ((state).overrun) \
			return ELIMIT; \
	} while (false)

/** Inflate algorithm state
 *
 */
typedef struct {
	uint8_t *dest;    /**< Output buffer */
	size_t destlen;   /**< Output buffer size */
	size_t destcnt;   /**< Position in the output buffer */

	uint8_t *src;     /**< Input buffer */
	size_t srclen;    /**< Input buffer size */
	size_t srccnt;    /**< Position in the input buffer */

	uint16_t bitbuf;  /**< Bit buffer */
	size_t bitlen;    /**< Number of bits in the bit buffer */

	bool overrun;     /**< Overrun condition */
} inflate_state_t;

/** Huffman code description
 *
 */
typedef struct {
	uint16_t *count;   /**< Array of symbol counts */
	uint16_t *symbol;  /**< Array of symbols */
} huffman_t;

/** Length codes
 *
 */
static const uint16_t lens[MAX_LEN] = {
	3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
	35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
};

/** Extended length codes
 *
 */
static const uint16_t lens_ext[MAX_LEN] = {
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
	3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
};

/** Distance codes
 *
 */
static const uint16_t dists[MAX_DIST] = {
	1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
	257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
	8193, 12289, 16385, 24577
};

/** Extended distance codes
 *
 */
static const uint16_t dists_ext[MAX_DIST] = {
	0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
	7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
	12, 12, 13, 13
};

/** Order codes
 *
 */
static const short order[MAX_ORDER] = {
	16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

/** Static length symbol counts
 *
 */
static uint16_t len_count[MAX_HUFFMAN_BIT + 1] = {
	0, 0, 0, 0, 0, 0, 0, 24, 152, 112, 0, 0, 0, 0, 0, 0
};

/** Static length symbols
 *
 */
static uint16_t len_symbol[MAX_FIXED_LITLEN] = {
	256, 257, 258, 259, 260, 261, 262, 263, 264, 265, 266, 267, 268,
	269, 270, 271, 272, 273, 274, 275, 276, 277, 278, 279, 0, 1, 2,
	3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
	21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
	37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52,
	53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68,
	69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84,
	85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100,
	101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113,
	114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126,
	127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139,
	140, 141, 142, 143, 280, 281, 282, 283, 284, 285, 286, 287, 144,
	145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157,
	158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170,
	171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183,
	184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196,
	197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
	210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222,
	223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235,
	236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248,
	249, 250, 251, 252, 253, 254, 255
};

/** Static distance symbol counts
 *
 */
static uint16_t dist_count[MAX_HUFFMAN_BIT + 1] = {
	0, 0, 0, 0, 0, 30, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/** Static distance symbols
 *
 */
static uint16_t dist_symbol[MAX_DIST] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29
};

/** Huffman code for lengths
 *
 */
static huffman_t len_code = {
	.count = len_count,
	.symbol = len_symbol
};

/** Huffman code for distances
 *
 */
static huffman_t dist_code = {
	.count = dist_count,
	.symbol = dist_symbol
};

/** Get bits from the bit buffer
 *
 * @param state Inflate state.
 * @param cnt   Number of bits to return (at most 16).
 *
 * @return Returned bits.
 *
 */
static inline uint16_t get_bits(inflate_state_t *state, size_t cnt)
{
	/* Bit accumulator for at least 20 bits */
	uint32_t val = state->bitbuf;

	while (state->bitlen < cnt) {
		if (state->srccnt == state->srclen) {
			state->overrun = true;
			return 0;
		}

		/* Load 8 more bits */
		val |= ((uint32_t) state->src[state->srccnt]) << state->bitlen;
		state->srccnt++;
		state->bitlen += 8;
	}

	/* Update bits in the buffer */
	state->bitbuf = (uint16_t) (val >> cnt);
	state->bitlen -= cnt;

	return ((uint16_t) (val & ((1 << cnt) - 1)));
}

/** Decode `stored' block
 *
 * @param state Inflate state.
 *
 * @return EOK on success.
 * @return ELIMIT on input buffer overrun.
 * @return ENOMEM on output buffer overrun.
 * @return EINVAL on invalid data.
 *
 */
static errno_t inflate_stored(inflate_state_t *state)
{
	/* Discard bits in the bit buffer */
	state->bitbuf = 0;
	state->bitlen = 0;

	if (state->srccnt + 4 > state->srclen)
		return ELIMIT;

	uint16_t len =
	    state->src[state->srccnt] | (state->src[state->srccnt + 1] << 8);
	uint16_t len_compl =
	    state->src[state->srccnt + 2] | (state->src[state->srccnt + 3] << 8);

	/* Check block length and its complement */
	if (((int16_t) len) != ~((int16_t) len_compl))
		return EINVAL;

	state->srccnt += 4;

	/* Check input buffer size */
	if (state->srccnt + len > state->srclen)
		return ELIMIT;

	/* Check output buffer size */
	if (state->destcnt + len > state->destlen)
		return ENOMEM;

	/* Copy data */
	memcpy(state->dest + state->destcnt, state->src + state->srccnt, len);
	state->srccnt += len;
	state->destcnt += len;

	return EOK;
}

/** Decode a symbol using the Huffman code
 *
 * @param state   Inflate state.
 * @param huffman Huffman code.
 * @param symbol  Decoded symbol.
 *
 * @param EOK on success.
 * @param EINVAL on invalid Huffman code.
 *
 */
static errno_t huffman_decode(inflate_state_t *state, huffman_t *huffman,
    uint16_t *symbol)
{
	/* Decode bits */
	uint16_t code = 0;

	/* First code of the given length */
	size_t first = 0;

	/*
	 * Index of the first code of the given length
	 * in the symbol table
	 */
	size_t index = 0;

	/* Current number of bits in the code */
	size_t len;

	for (len = 1; len <= MAX_HUFFMAN_BIT; len++) {
		/* Get next bit */
		code |= get_bits(state, 1);
		CHECK_OVERRUN(*state);

		uint16_t count = huffman->count[len];
		if (code < first + count) {
			/* Return decoded symbol */
			*symbol = huffman->symbol[index + code - first];
			return EOK;
		}

		/* Update for next length */
		index += count;
		first += count;
		first <<= 1;
		code <<= 1;
	}

	return EINVAL;
}

/** Construct Huffman tables from canonical Huffman code
 *
 * @param huffman Constructed Huffman tables.
 * @param length  Lengths of the canonical Huffman code.
 * @param n       Number of lengths.
 *
 * @return 0 if the Huffman code set is complete.
 * @return Negative value for an over-subscribed code set.
 * @return Positive value for an incomplete code set.
 *
 */
static int16_t huffman_construct(huffman_t *huffman, uint16_t *length, size_t n)
{
	/* Count number of codes for each length */
	size_t len;
	for (len = 0; len <= MAX_HUFFMAN_BIT; len++)
		huffman->count[len] = 0;

	/* We assume that the lengths are within bounds */
	size_t symbol;
	for (symbol = 0; symbol < n; symbol++)
		huffman->count[length[symbol]]++;

	if (huffman->count[0] == n) {
		/* The code is complete, but decoding will fail */
		return 0;
	}

	/* Check for an over-subscribed or incomplete set of lengths */
	int16_t left = 1;
	for (len = 1; len <= MAX_HUFFMAN_BIT; len++) {
		left <<= 1;
		left -= huffman->count[len];
		if (left < 0) {
			/* Over-subscribed */
			return left;
		}
	}

	/* Generate offsets into symbol table */
	uint16_t offs[MAX_HUFFMAN_BIT + 1];

	offs[1] = 0;
	for (len = 1; len < MAX_HUFFMAN_BIT; len++)
		offs[len + 1] = offs[len] + huffman->count[len];

	for (symbol = 0; symbol < n; symbol++) {
		if (length[symbol] != 0) {
			huffman->symbol[offs[length[symbol]]] = symbol;
			offs[length[symbol]]++;
		}
	}

	return left;
}

/** Decode literal/length and distance codes
 *
 * Decode until end-of-block code.
 *
 * @param state     Inflate state.
 * @param len_code  Huffman code for literal/length.
 * @param dist_code Huffman code for distance.
 *
 * @return EOK on success.
 * @return ENOENT on distance too large.
 * @return EINVAL on invalid Huffman code.
 * @return ELIMIT on input buffer overrun.
 * @return ENOMEM on output buffer overrun.
 *
 */
static errno_t inflate_codes(inflate_state_t *state, huffman_t *len_code,
    huffman_t *dist_code)
{
	uint16_t symbol;

	do {
		errno_t err = huffman_decode(state, len_code, &symbol);
		if (err != EOK) {
			/* Error decoding */
			return err;
		}

		if (symbol < 256) {
			/* Write out literal */
			if (state->destcnt == state->destlen)
				return ENOMEM;

			state->dest[state->destcnt] = (uint8_t) symbol;
			state->destcnt++;
		} else if (symbol > 256) {
			/* Compute length */
			symbol -= 257;
			if (symbol >= 29)
				return EINVAL;

			size_t len = lens[symbol] + get_bits(state, lens_ext[symbol]);
			CHECK_OVERRUN(*state);

			/* Get distance */
			err = huffman_decode(state, dist_code, &symbol);
			if (err != EOK)
				return err;

			size_t dist = dists[symbol] + get_bits(state, dists_ext[symbol]);
			if (dist > state->destcnt)
				return ENOENT;

			if (state->destcnt + len > state->destlen)
				return ENOMEM;

			while (len > 0) {
				/* Copy len bytes from distance bytes back */
				state->dest[state->destcnt] =
				    state->dest[state->destcnt - dist];
				state->destcnt++;
				len--;
			}
		}
	} while (symbol != 256);

	return EOK;
}

/** Decode `fixed codes' block
 *
 * @param state     Inflate state.
 * @param len_code  Huffman code for literal/length.
 * @param dist_code Huffman code for distance.
 *
 * @return EOK on success.
 * @return ENOENT on distance too large.
 * @return EINVAL on invalid Huffman code.
 * @return ELIMIT on input buffer overrun.
 * @return ENOMEM on output buffer overrun.
 *
 */
static errno_t inflate_fixed(inflate_state_t *state, huffman_t *len_code,
    huffman_t *dist_code)
{
	return inflate_codes(state, len_code, dist_code);
}

/** Decode `dynamic codes' block
 *
 * @param state     Inflate state.
 *
 * @return EOK on success.
 * @return ENOENT on distance too large.
 * @return EINVAL on invalid Huffman code.
 * @return ELIMIT on input buffer overrun.
 * @return ENOMEM on output buffer overrun.
 *
 */
static errno_t inflate_dynamic(inflate_state_t *state)
{
	uint16_t length[MAX_CODE];
	uint16_t dyn_len_count[MAX_HUFFMAN_BIT + 1];
	uint16_t dyn_len_symbol[MAX_LITLEN];
	uint16_t dyn_dist_count[MAX_HUFFMAN_BIT + 1];
	uint16_t dyn_dist_symbol[MAX_DIST];
	huffman_t dyn_len_code;
	huffman_t dyn_dist_code;

	dyn_len_code.count = dyn_len_count;
	dyn_len_code.symbol = dyn_len_symbol;

	dyn_dist_code.count = dyn_dist_count;
	dyn_dist_code.symbol = dyn_dist_symbol;

	/* Get number of bits in each table */
	uint16_t nlen = get_bits(state, 5) + 257;
	CHECK_OVERRUN(*state);

	uint16_t ndist = get_bits(state, 5) + 1;
	CHECK_OVERRUN(*state);

	uint16_t ncode = get_bits(state, 4) + 4;
	CHECK_OVERRUN(*state);

	if ((nlen > MAX_LITLEN) || (ndist > MAX_DIST) ||
	    (ncode > MAX_ORDER))
		return EINVAL;

	/* Read code length code lengths */
	uint16_t index;
	for (index = 0; index < ncode; index++) {
		length[order[index]] = get_bits(state, 3);
		CHECK_OVERRUN(*state);
	}

	/* Set missing lengths to zero */
	for (index = ncode; index < MAX_ORDER; index++)
		length[order[index]] = 0;

	/* Build Huffman code */
	int16_t rc = huffman_construct(&dyn_len_code, length, MAX_ORDER);
	if (rc != 0)
		return EINVAL;

	/* Read length/literal and distance code length tables */
	index = 0;
	while (index < nlen + ndist) {
		uint16_t symbol;
		errno_t err = huffman_decode(state, &dyn_len_code, &symbol);
		if (err != EOK)
			return EOK;

		if (symbol < 16) {
			length[index] = symbol;
			index++;
		} else {
			uint16_t len = 0;

			if (symbol == 16) {
				if (index == 0)
					return EINVAL;

				len = length[index - 1];
				symbol = get_bits(state, 2) + 3;
				CHECK_OVERRUN(*state);
			} else if (symbol == 17) {
				symbol = get_bits(state, 3) + 3;
				CHECK_OVERRUN(*state);
			} else {
				symbol = get_bits(state, 7) + 11;
				CHECK_OVERRUN(*state);
			}

			if (index + symbol > nlen + ndist)
				return EINVAL;

			while (symbol > 0) {
				length[index] = len;
				index++;
				symbol--;
			}
		}
	}

	/* Check for end-of-block code */
	if (length[256] == 0)
		return EINVAL;

	/* Build Huffman tables for literal/length codes */
	rc = huffman_construct(&dyn_len_code, length, nlen);
	if ((rc < 0) || ((rc > 0) && (dyn_len_code.count[0] + 1 != nlen)))
		return EINVAL;

	/* Build Huffman tables for distance codes */
	rc = huffman_construct(&dyn_dist_code, length + nlen, ndist);
	if ((rc < 0) || ((rc > 0) && (dyn_dist_code.count[0] + 1 != ndist)))
		return EINVAL;

	return inflate_codes(state, &dyn_len_code, &dyn_dist_code);
}

/** Inflate data
 *
 * @param src     Source data buffer.
 * @param srclen  Source buffer size (bytes).
 * @param dest    Destination data buffer.
 * @param destlen Destination buffer size (bytes).
 *
 * @return EOK on success.
 * @return ENOENT on distance too large.
 * @return EINVAL on invalid Huffman code or invalid deflate data.
 * @return ELIMIT on input buffer overrun.
 * @return ENOMEM on output buffer overrun.
 *
 */
errno_t inflate(void *src, size_t srclen, void *dest, size_t destlen)
{
	/* Initialize the state */
	inflate_state_t state;

	state.dest = (uint8_t *) dest;
	state.destlen = destlen;
	state.destcnt = 0;

	state.src = (uint8_t *) src;
	state.srclen = srclen;
	state.srccnt = 0;

	state.bitbuf = 0;
	state.bitlen = 0;

	state.overrun = false;

	uint16_t last;
	errno_t ret = EOK;

	do {
		/* Last block is indicated by a non-zero bit */
		last = get_bits(&state, 1);
		CHECK_OVERRUN(state);

		/* Block type */
		uint16_t type = get_bits(&state, 2);
		CHECK_OVERRUN(state);

		switch (type) {
		case 0:
			ret = inflate_stored(&state);
			break;
		case 1:
			ret = inflate_fixed(&state, &len_code, &dist_code);
			break;
		case 2:
			ret = inflate_dynamic(&state);
			break;
		default:
			ret = EINVAL;
		}
	} while ((!last) && (ret == 0));

	return ret;
}
