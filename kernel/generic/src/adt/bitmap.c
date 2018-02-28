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
 * setting and clearing ranges of bits and for finding ranges
 * of unset bits.
 */

#include <adt/bitmap.h>
#include <align.h>
#include <assert.h>
#include <macros.h>
#include <typedefs.h>

#define ALL_ONES    0xff
#define ALL_ZEROES  0x00

/** Unchecked version of bitmap_get()
 *
 * This version of bitmap_get() does not do any boundary checks.
 *
 * @param bitmap  Bitmap to access.
 * @param element Element to access.
 *
 * @return Bit value of the element in the bitmap.
 *
 */
static unsigned int bitmap_get_fast(bitmap_t *bitmap, size_t element)
{
	size_t byte = element / BITMAP_ELEMENT;
	uint8_t mask = 1 << (element & BITMAP_REMAINER);

	return !!((bitmap->bits)[byte] & mask);
}

/** Get bitmap size
 *
 * Return the size (in bytes) required for the bitmap.
 *
 * @param elements   Number bits stored in bitmap.
 *
 * @return Size (in bytes) required for the bitmap.
 *
 */
size_t bitmap_size(size_t elements)
{
	size_t size = elements / BITMAP_ELEMENT;

	if ((elements % BITMAP_ELEMENT) != 0)
		size++;

	return size;
}

/** Initialize bitmap.
 *
 * No portion of the bitmap is set or cleared by this function.
 *
 * @param bitmap     Bitmap structure.
 * @param elements   Number of bits stored in bitmap.
 * @param data       Address of the memory used to hold the map.
 *                   The optional 2nd level bitmap follows the 1st
 *                   level bitmap.
 *
 */
void bitmap_initialize(bitmap_t *bitmap, size_t elements, void *data)
{
	bitmap->elements = elements;
	bitmap->bits = (uint8_t *) data;
	bitmap->next_fit = 0;
}

/** Set range of bits.
 *
 * @param bitmap Bitmap structure.
 * @param start  Starting bit.
 * @param count  Number of bits to set.
 *
 */
void bitmap_set_range(bitmap_t *bitmap, size_t start, size_t count)
{
	assert(start + count <= bitmap->elements);

	if (count == 0)
		return;

	size_t start_byte = start / BITMAP_ELEMENT;
	size_t aligned_start = ALIGN_UP(start, BITMAP_ELEMENT);

	/* Leading unaligned bits */
	size_t lub = min(aligned_start - start, count);

	/* Aligned middle bits */
	size_t amb = (count > lub) ? (count - lub) : 0;

	/* Trailing aligned bits */
	size_t tab = amb % BITMAP_ELEMENT;

	if (start + count < aligned_start) {
		/* Set bits in the middle of byte. */
		bitmap->bits[start_byte] |=
		    ((1 << lub) - 1) << (start & BITMAP_REMAINER);
		return;
	}

	if (lub) {
		/* Make sure to set any leading unaligned bits. */
		bitmap->bits[start_byte] |=
		    ~((1 << (BITMAP_ELEMENT - lub)) - 1);
	}

	size_t i;

	for (i = 0; i < amb / BITMAP_ELEMENT; i++) {
		/* The middle bits can be set byte by byte. */
		bitmap->bits[aligned_start / BITMAP_ELEMENT + i] =
		    ALL_ONES;
	}

	if (tab) {
		/* Make sure to set any trailing aligned bits. */
		bitmap->bits[aligned_start / BITMAP_ELEMENT + i] |=
		    (1 << tab) - 1;
	}
}

/** Clear range of bits.
 *
 * @param bitmap Bitmap structure.
 * @param start  Starting bit.
 * @param count  Number of bits to clear.
 *
 */
void bitmap_clear_range(bitmap_t *bitmap, size_t start, size_t count)
{
	assert(start + count <= bitmap->elements);

	if (count == 0)
		return;

	size_t start_byte = start / BITMAP_ELEMENT;
	size_t aligned_start = ALIGN_UP(start, BITMAP_ELEMENT);

	/* Leading unaligned bits */
	size_t lub = min(aligned_start - start, count);

	/* Aligned middle bits */
	size_t amb = (count > lub) ? (count - lub) : 0;

	/* Trailing aligned bits */
	size_t tab = amb % BITMAP_ELEMENT;

	if (start + count < aligned_start) {
		/* Set bits in the middle of byte */
		bitmap->bits[start_byte] &=
		    ~(((1 << lub) - 1) << (start & BITMAP_REMAINER));
		return;
	}

	if (lub) {
		/* Make sure to clear any leading unaligned bits. */
		bitmap->bits[start_byte] &=
		    (1 << (BITMAP_ELEMENT - lub)) - 1;
	}

	size_t i;

	for (i = 0; i < amb / BITMAP_ELEMENT; i++) {
		/* The middle bits can be cleared byte by byte. */
		bitmap->bits[aligned_start / BITMAP_ELEMENT + i] =
		    ALL_ZEROES;
	}

	if (tab) {
		/* Make sure to clear any trailing aligned bits. */
		bitmap->bits[aligned_start / BITMAP_ELEMENT + i] &=
		    ~((1 << tab) - 1);
	}

	bitmap->next_fit = start_byte;
}

/** Copy portion of one bitmap into another bitmap.
 *
 * @param dst   Destination bitmap.
 * @param src   Source bitmap.
 * @param count Number of bits to copy.
 *
 */
void bitmap_copy(bitmap_t *dst, bitmap_t *src, size_t count)
{
	assert(count <= dst->elements);
	assert(count <= src->elements);

	size_t i;

	for (i = 0; i < count / BITMAP_ELEMENT; i++)
		dst->bits[i] = src->bits[i];

	if (count % BITMAP_ELEMENT) {
		bitmap_clear_range(dst, i * BITMAP_ELEMENT,
		    count % BITMAP_ELEMENT);
		dst->bits[i] |= src->bits[i] &
		    ((1 << (count % BITMAP_ELEMENT)) - 1);
	}
}

static int constraint_satisfy(size_t index, size_t base, size_t constraint)
{
	return (((base + index) & constraint) == 0);
}

/** Find a continuous zero bit range
 *
 * Find a continuous zero bit range in the bitmap. The address
 * computed as the sum of the index of the first zero bit and
 * the base argument needs to be compliant with the constraint
 * (those bits that are set in the constraint cannot be set in
 * the address).
 *
 * If the index argument is non-NULL, the continuous zero range
 * is set and the index of the first bit is stored to index.
 * Otherwise the bitmap stays untouched.
 *
 * @param bitmap     Bitmap structure.
 * @param count      Number of continuous zero bits to find.
 * @param base       Address of the first bit in the bitmap.
 * @param prefered   Prefered address to start searching from.
 * @param constraint Constraint for the address of the first zero bit.
 * @param index      Place to store the index of the first zero
 *                   bit. Can be NULL (in which case the bitmap
 *                   is not modified).
 *
 * @return Non-zero if a continuous range of zero bits satisfying
 *         the constraint has been found.
 * @return Zero otherwise.
 *
 */
bool bitmap_allocate_range(bitmap_t *bitmap, size_t count, size_t base,
    size_t prefered, size_t constraint, size_t *index)
{
	if (count == 0)
		return false;

	size_t size = bitmap_size(bitmap->elements);
	size_t next_fit = bitmap->next_fit;

	/*
	 * Adjust the next-fit value according to the address
	 * the caller prefers to start the search at.
	 */
	if ((prefered > base) && (prefered < base + bitmap->elements)) {
		size_t prefered_fit = (prefered - base) / BITMAP_ELEMENT;

		if (prefered_fit > next_fit)
			next_fit = prefered_fit;
	}

	for (size_t pos = 0; pos < size; pos++) {
		size_t byte = (next_fit + pos) % size;

		/* Skip if the current byte has all bits set */
		if (bitmap->bits[byte] == ALL_ONES)
			continue;

		size_t byte_bit = byte * BITMAP_ELEMENT;

		for (size_t bit = 0; bit < BITMAP_ELEMENT; bit++) {
			size_t i = byte_bit + bit;

			if (i >= bitmap->elements)
				break;

			if (!constraint_satisfy(i, base, constraint))
				continue;

			if (!bitmap_get_fast(bitmap, i)) {
				size_t continuous = 1;

				for (size_t j = 1; j < count; j++) {
					if ((i + j < bitmap->elements) &&
					    (!bitmap_get_fast(bitmap, i + j)))
						continuous++;
					else
						break;
				}

				if (continuous == count) {
					if (index != NULL) {
						bitmap_set_range(bitmap, i, count);
						bitmap->next_fit = i / BITMAP_ELEMENT;
						*index = i;
					}

					return true;
				} else
					i += continuous;
			}
		}
	}

	return false;
}

/** @}
 */
