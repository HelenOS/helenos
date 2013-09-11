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
 *
 * The bitmap ADT can optionally implement a two-level hierarchy
 * for faster range searches. The second level bitmap (of blocks)
 * is not precise, but conservative. This means that if the block
 * bit is set, it guarantees that all bits in the block are set.
 * But if the block bit is unset, nothing can be said about the
 * bits in the block.
 *
 */

#include <adt/bitmap.h>
#include <typedefs.h>
#include <align.h>
#include <debug.h>
#include <macros.h>

#define ALL_ONES    0xff
#define ALL_ZEROES  0x00

/** Get bitmap size
 *
 * Return the size (in bytes) required for the bitmap.
 *
 * @param elements   Number bits stored in bitmap.
 * @param block_size Block size of the 2nd level bitmap.
 *                   If set to zero, no 2nd level is used.
 *
 * @return Size (in bytes) required for the bitmap.
 *
 */
size_t bitmap_size(size_t elements, size_t block_size)
{
	size_t size = elements / BITMAP_ELEMENT;
	
	if ((elements % BITMAP_ELEMENT) != 0)
		size++;
	
	if (block_size > 0) {
		size += elements / block_size;
		
		if ((elements % block_size) != 0)
			size++;
	}
	
	return size;
}

/** Initialize bitmap.
 *
 * No portion of the bitmap is set or cleared by this function.
 *
 * @param bitmap     Bitmap structure.
 * @param elements   Number of bits stored in bitmap.
 * @param block_size Block size of the 2nd level bitmap.
 *                   If set to zero, no 2nd level is used.
 * @param data       Address of the memory used to hold the map.
 *                   The optional 2nd level bitmap follows the 1st
 *                   level bitmap.
 *
 */
void bitmap_initialize(bitmap_t *bitmap, size_t elements, size_t block_size,
    void *data)
{
	bitmap->elements = elements;
	bitmap->bits = (uint8_t *) data;
	
	if (block_size > 0) {
		bitmap->block_size = block_size;
		bitmap->blocks = bitmap->bits +
		    bitmap_size(elements, 0);
	} else {
		bitmap->block_size = 0;
		bitmap->blocks = NULL;
	}
}

static void bitmap_set_range_internal(uint8_t *bits, size_t start, size_t count)
{
	if (count == 0)
		return;
	
	size_t aligned_start = ALIGN_UP(start, BITMAP_ELEMENT);
	
	/* Leading unaligned bits */
	size_t lub = min(aligned_start - start, count);
	
	/* Aligned middle bits */
	size_t amb = (count > lub) ? (count - lub) : 0;
	
	/* Trailing aligned bits */
	size_t tab = amb % BITMAP_ELEMENT;
	
	if (start + count < aligned_start) {
		/* Set bits in the middle of byte. */
		bits[start / BITMAP_ELEMENT] |=
		    ((1 << lub) - 1) << (start & BITMAP_REMAINER);
		return;
	}
	
	if (lub) {
		/* Make sure to set any leading unaligned bits. */
		bits[start / BITMAP_ELEMENT] |=
		    ~((1 << (BITMAP_ELEMENT - lub)) - 1);
	}
	
	size_t i;
	
	for (i = 0; i < amb / BITMAP_ELEMENT; i++) {
		/* The middle bits can be set byte by byte. */
		bits[aligned_start / BITMAP_ELEMENT + i] = ALL_ONES;
	}
	
	if (tab) {
		/* Make sure to set any trailing aligned bits. */
		bits[aligned_start / BITMAP_ELEMENT + i] |= (1 << tab) - 1;
	}
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
	ASSERT(start + count <= bitmap->elements);
	
	bitmap_set_range_internal(bitmap->bits, start, count);
	
	if (bitmap->block_size > 0) {
		size_t aligned_start = ALIGN_UP(start, bitmap->block_size);
		
		/* Leading unaligned bits */
		size_t lub = min(aligned_start - start, count);
		
		/* Aligned middle bits */
		size_t amb = (count > lub) ? (count - lub) : 0;
		
		size_t aligned_size = amb / bitmap->block_size;
		
		bitmap_set_range_internal(bitmap->blocks, aligned_start,
		    aligned_size);
	}
}

static void bitmap_clear_range_internal(uint8_t *bits, size_t start,
    size_t count)
{
	if (count == 0)
		return;
	
	size_t aligned_start = ALIGN_UP(start, BITMAP_ELEMENT);
	
	/* Leading unaligned bits */
	size_t lub = min(aligned_start - start, count);
	
	/* Aligned middle bits */
	size_t amb = (count > lub) ? (count - lub) : 0;
	
	/* Trailing aligned bits */
	size_t tab = amb % BITMAP_ELEMENT;
	
	if (start + count < aligned_start) {
		/* Set bits in the middle of byte */
		bits[start / BITMAP_ELEMENT] &=
		    ~(((1 << lub) - 1) << (start & BITMAP_REMAINER));
		return;
	}
	
	if (lub) {
		/* Make sure to clear any leading unaligned bits. */
		bits[start / BITMAP_ELEMENT] &=
		    (1 << (BITMAP_ELEMENT - lub)) - 1;
	}
	
	size_t i;
	
	for (i = 0; i < amb / BITMAP_ELEMENT; i++) {
		/* The middle bits can be cleared byte by byte. */
		bits[aligned_start / BITMAP_ELEMENT + i] = ALL_ZEROES;
	}
	
	if (tab) {
		/* Make sure to clear any trailing aligned bits. */
		bits[aligned_start / BITMAP_ELEMENT + i] &= ~((1 << tab) - 1);
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
	ASSERT(start + count <= bitmap->elements);
	
	bitmap_clear_range_internal(bitmap->bits, start, count);
	
	if (bitmap->block_size > 0) {
		size_t aligned_start = start / bitmap->block_size;
		
		size_t aligned_end = (start + count) / bitmap->block_size;
		
		if (((start + count) % bitmap->block_size) != 0)
			aligned_end++;
		
		size_t aligned_size = aligned_end - aligned_start;
		
		bitmap_clear_range_internal(bitmap->blocks, aligned_start,
		    aligned_size);
	}
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
	ASSERT(count <= dst->elements);
	ASSERT(count <= src->elements);
	
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
int bitmap_allocate_range(bitmap_t *bitmap, size_t count, size_t base,
    size_t constraint, size_t *index)
{
	if (count == 0)
		return false;
	
	/*
	 * This is a trivial implementation that should be
	 * optimized further.
	 */
	
	for (size_t i = 0; i < bitmap->elements; i++) {
		if (!constraint_satisfy(i, base, constraint))
			continue;
		
		if (!bitmap_get(bitmap, i)) {
			bool continuous = true;
			
			for (size_t j = 1; j < count; j++) {
				if ((i + j >= bitmap->elements) ||
				    (bitmap_get(bitmap, i + j))) {
					continuous = false;
					break;
				}
			}
			
			if (continuous) {
				if (index != NULL) {
					bitmap_set_range(bitmap, i, count);
					*index = i;
				}
				
				return true;
			}
		}
		
	}
	
	return false;
}

/** Clear range of set bits.
 *
 * This is essentially bitmap_clear_range(), but it also
 * checks whether all the cleared bits are actually set.
 *
 * @param bitmap Bitmap structure.
 * @param start  Starting bit.
 * @param count  Number of bits to clear.
 *
 */
void bitmap_free_range(bitmap_t *bitmap, size_t start, size_t count)
{
	/*
	 * This is a trivial implementation that should be
	 * optimized further.
	 */
	
	for (size_t i = 0; i < count; i++) {
		if (!bitmap_get(bitmap, start + i))
			panic("Freeing a bitmap range that is not set");
	}
	
	bitmap_clear_range(bitmap, start, count);
}

/** @}
 */
