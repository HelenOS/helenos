/*
 * Copyright (c) 2006 Jakub Jermar
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

/** @addtogroup genericadt
 * @{
 */
/**
 * @file
 * @brief Implementation of bitmap ADT.
 *
 * This file implements bitmap ADT and provides functions for
 * setting and clearing ranges of bits.
 */

#include <adt/bitmap.h>
#include <typedefs.h>
#include <align.h>
#include <debug.h>
#include <macros.h>

#define ALL_ONES 	0xff
#define ALL_ZEROES	0x00

/** Initialize bitmap.
 *
 * No portion of the bitmap is set or cleared by this function.
 *
 * @param bitmap	Bitmap structure.
 * @param map		Address of the memory used to hold the map.
 * @param bits		Number of bits stored in bitmap.
 */
void bitmap_initialize(bitmap_t *bitmap, uint8_t *map, size_t bits)
{
	bitmap->map = map;
	bitmap->bits = bits;
}

/** Set range of bits.
 *
 * @param bitmap	Bitmap structure.
 * @param start		Starting bit.
 * @param bits		Number of bits to set.
 */
void bitmap_set_range(bitmap_t *bitmap, size_t start, size_t bits)
{
	size_t i = 0;
	size_t aligned_start;
	size_t lub;	/* leading unaligned bits */
	size_t amb;	/* aligned middle bits */
	size_t tab;	/* trailing aligned bits */
	
	ASSERT(start + bits <= bitmap->bits);
	
	aligned_start = ALIGN_UP(start, 8);
	lub = min(aligned_start - start, bits);
	amb = bits > lub ? bits - lub : 0;
	tab = amb % 8;
	
	if (!bits)
		return;

	if (start + bits < aligned_start) {
		/* Set bits in the middle of byte. */
		bitmap->map[start / 8] |= ((1 << lub) - 1) << (start & 7);
		return;
	}
	
	if (lub) {
		/* Make sure to set any leading unaligned bits. */
		bitmap->map[start / 8] |= ~((1 << (8 - lub)) - 1);
	}
	for (i = 0; i < amb / 8; i++) {
		/* The middle bits can be set byte by byte. */
		bitmap->map[aligned_start / 8 + i] = ALL_ONES;
	}
	if (tab) {
		/* Make sure to set any trailing aligned bits. */
		bitmap->map[aligned_start / 8 + i] |= (1 << tab) - 1;
	}
	
}

/** Clear range of bits.
 *
 * @param bitmap	Bitmap structure.
 * @param start		Starting bit.
 * @param bits		Number of bits to clear.
 */
void bitmap_clear_range(bitmap_t *bitmap, size_t start, size_t bits)
{
	size_t i = 0;
	size_t aligned_start;
	size_t lub;	/* leading unaligned bits */
	size_t amb;	/* aligned middle bits */
	size_t tab;	/* trailing aligned bits */
	
	ASSERT(start + bits <= bitmap->bits);
	
	aligned_start = ALIGN_UP(start, 8);
	lub = min(aligned_start - start, bits);
	amb = bits > lub ? bits - lub : 0;
	tab = amb % 8;

	if (!bits)
		return;

	if (start + bits < aligned_start) {
		/* Set bits in the middle of byte */
		bitmap->map[start / 8] &= ~(((1 << lub) - 1) << (start & 7));
		return;
	}

	if (lub) {
		/* Make sure to clear any leading unaligned bits. */
		bitmap->map[start / 8] &= (1 << (8 - lub)) - 1;
	}
	for (i = 0; i < amb / 8; i++) {
		/* The middle bits can be cleared byte by byte. */
		bitmap->map[aligned_start / 8 + i] = ALL_ZEROES;
	}
	if (tab) {
		/* Make sure to clear any trailing aligned bits. */
		bitmap->map[aligned_start / 8 + i] &= ~((1 << tab) - 1);
	}

}

/** Copy portion of one bitmap into another bitmap.
 *
 * @param dst		Destination bitmap.
 * @param src		Source bitmap.
 * @param bits		Number of bits to copy.
 */
void bitmap_copy(bitmap_t *dst, bitmap_t *src, size_t bits)
{
	size_t i;
	
	ASSERT(bits <= dst->bits);
	ASSERT(bits <= src->bits);
	
	for (i = 0; i < bits / 8; i++)
		dst->map[i] = src->map[i];
	
	if (bits % 8) {
		bitmap_clear_range(dst, i * 8, bits % 8);
		dst->map[i] |= src->map[i] & ((1 << (bits % 8)) - 1);
	}
}

/** @}
 */
