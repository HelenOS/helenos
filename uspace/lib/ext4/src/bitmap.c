/*
 * Copyright (c) 2012 Frantisek Princ
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

/** @addtogroup libext4
 * @{
 */
/**
 * @file  bitmap.c
 * @brief Ext4 bitmap operations.
 */

#include <errno.h>
#include <block.h>
#include <stdint.h>
#include "ext4/bitmap.h"

/** Set bit in bitmap to 0 (free).
 *
 * Index must be checked by caller, if it's not out of bounds.
 *
 * @param bitmap Pointer to bitmap
 * @param index  Index of bit in bitmap
 *
 */
void ext4_bitmap_free_bit(uint8_t *bitmap, uint32_t index)
{
	uint32_t byte_index = index / 8;
	uint32_t bit_index = index % 8;
	
	uint8_t *target = bitmap + byte_index;
	
	*target &= ~ (1 << bit_index);
}

/** Free continous set of bits (set to 0).
 *
 * Index and count must be checked by caller, if they aren't out of bounds.
 *
 * @param bitmap Pointer to bitmap
 * @param index  Index of first bit to zeroed
 * @param count  Number of bits to be zeroed
 *
 */
void ext4_bitmap_free_bits(uint8_t *bitmap, uint32_t index, uint32_t count)
{
	uint8_t *target;
	uint32_t idx = index;
	uint32_t remaining = count;
	uint32_t byte_index;
	
	/* Align index to multiple of 8 */
	while (((idx % 8) != 0) && (remaining > 0)) {
		byte_index = idx / 8;
		uint32_t bit_index = idx % 8;
		
		target = bitmap + byte_index;
		*target &= ~ (1 << bit_index);
		
		idx++;
		remaining--;
	}
	
	/* For < 8 bits this check necessary */
	if (remaining == 0)
		return;
	
	assert((idx % 8) == 0);
	
	byte_index = idx / 8;
	target = bitmap + byte_index;
	
	/* Zero the whole bytes */
	while (remaining >= 8) {
		*target = 0;
		
		idx += 8;
		remaining -= 8;
		target++;
	}
	
	assert(remaining < 8);
	
	/* Zero remaining bytes */
	while (remaining != 0) {
		byte_index = idx / 8;
		uint32_t bit_index = idx % 8;
		
		target = bitmap + byte_index;
		*target &= ~ (1 << bit_index);
		
		idx++;
		remaining--;
	}
}

/** Set bit in bitmap to 1 (used).
 *
 * @param bitmap Pointer to bitmap
 * @param index  Index of bit to set
 *
 */
void ext4_bitmap_set_bit(uint8_t *bitmap, uint32_t index)
{
	uint32_t byte_index = index / 8;
	uint32_t bit_index = index % 8;
	
	uint8_t *target = bitmap + byte_index;
	
	*target |= 1 << bit_index;
}

/** Check if requested bit is free.
 *
 * @param bitmap Pointer to bitmap
 * @param index  Index of bit to be checked
 *
 * @return True if bit is free, else false
 *
 */
bool ext4_bitmap_is_free_bit(uint8_t *bitmap, uint32_t index)
{
	uint32_t byte_index = index / 8;
	uint32_t bit_index = index % 8;
	
	uint8_t *target = bitmap + byte_index;
	
	if (*target & (1 << bit_index))
		return false;
	else
		return true;
}

/** Try to find free byte and set the first bit as used.
 *
 * Walk through bitmap and try to find free byte (equal to 0).
 * If byte found, set the first bit as used.
 *
 * @param bitmap Pointer to bitmap
 * @param start  Index of bit, where the algorithm will begin
 * @param index  Output value - index of bit (if found free byte)
 * @param max    Maximum index of bit in bitmap
 *
 * @return Error code
 *
 */
errno_t ext4_bitmap_find_free_byte_and_set_bit(uint8_t *bitmap, uint32_t start,
    uint32_t *index, uint32_t max)
{
	uint32_t idx;
	
	/* Align idx */
	if (start % 8)
		idx = start + (8 - (start % 8));
	else
		idx = start;
	
	uint8_t *pos = bitmap + (idx / 8);
	
	/* Try to find free byte */
	while (idx < max) {
		if (*pos == 0) {
			*pos |= 1;
			
			*index = idx;
			return EOK;
		}
		
		idx += 8;
		++pos;
	}
	
	/* Free byte not found */
	return ENOSPC;
}

/** Try to find free bit and set it as used (1).
 *
 * Walk through bitmap and try to find any free bit.
 *
 * @param bitmap    Pointer to bitmap
 * @param start_idx Index of bit, where algorithm will begin
 * @param index     Output value - index of set bit (if found)
 * @param max       Maximum index of bit in bitmap
 *
 * @return Error code
 *
 */
errno_t ext4_bitmap_find_free_bit_and_set(uint8_t *bitmap, uint32_t start_idx,
    uint32_t *index, uint32_t max)
{
	uint8_t *pos = bitmap + (start_idx / 8);
	uint32_t idx = start_idx;
	bool byte_part = false;
	
	/* Check the rest of first byte */
	while ((idx % 8) != 0) {
		byte_part = true;
		
		if ((*pos & (1 << (idx % 8))) == 0) {
			*pos |= (1 << (idx % 8));
			*index = idx;
			return EOK;
		}
		
		++idx;
	}
	
	if (byte_part)
		++pos;
	
	/* Check the whole bytes (255 = 11111111 binary) */
	while (idx < max) {
		if ((*pos & 255) != 255) {
			/* Free bit found */
			break;
		}
		
		idx += 8;
		++pos;
	}
	
	/* If idx < max, some free bit found */
	if (idx < max) {
		/* Check which bit from byte is free */
		for (uint8_t i = 0; i < 8; ++i) {
			if ((*pos & (1 << i)) == 0) {
				/* Free bit found */
				*pos |= (1 << i);
				
				*index = idx;
				return EOK;
			}
			
			idx++;
		}
	}
	
	/* Free bit not found */
	return ENOSPC;
}

/**
 * @}
 */
