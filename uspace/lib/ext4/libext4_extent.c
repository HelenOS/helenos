/*
 * Copyright (c) 2011 Frantisek Princ
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
 * @file	libext4_extent.c
 * @brief	Ext4 extent structures operations.
 */

#include <byteorder.h>
#include "libext4_extent.h"

uint32_t ext4_extent_get_first_block(ext4_extent_t *extent)
{
	return uint32_t_le2host(extent->first_block);
}

void ext4_extent_set_first_block(ext4_extent_t *extent, uint32_t first_block)
{
	extent->first_block = host2uint32_t_le(first_block);
}

uint16_t ext4_extent_get_block_count(ext4_extent_t *extent)
{
	return uint16_t_le2host(extent->block_count);
}

void ext4_extent_set_block_count(ext4_extent_t *extent, uint16_t block_count)
{
	extent->block_count = host2uint16_t_le(block_count);
}

uint64_t ext4_extent_get_start(ext4_extent_t *extent)
{
	return ((uint64_t)uint16_t_le2host(extent->start_hi)) << 32 |
			((uint64_t)uint32_t_le2host(extent->start_lo));
}

void ext4_extent_set_start(ext4_extent_t *extent, uint64_t start)
{
	extent->start_lo = host2uint32_t_le((start << 32) >> 32);
	extent->start_hi = host2uint16_t_le((uint16_t)(start >> 32));
}

uint32_t ext4_extent_index_get_first_block(ext4_extent_index_t *index)
{
	return uint32_t_le2host(index->first_block);
}

void ext4_extent_index_set_first_block(ext4_extent_index_t *index,
		uint32_t first)
{
	index->first_block = host2uint32_t_le(first);
}

uint64_t ext4_extent_index_get_leaf(ext4_extent_index_t *index)
{
	return ((uint64_t)uint16_t_le2host(index->leaf_hi)) << 32 |
		((uint64_t)uint32_t_le2host(index->leaf_lo));
}

void ext4_extent_index_set_leaf(ext4_extent_index_t *index, uint64_t leaf)
{
	index->leaf_lo = host2uint32_t_le((leaf << 32) >> 32);
	index->leaf_hi = host2uint16_t_le((uint16_t)(leaf >> 32));
}

uint16_t ext4_extent_header_get_magic(ext4_extent_header_t *header)
{
	return uint16_t_le2host(header->magic);
}

void ext4_extent_header_set_magic(ext4_extent_header_t *header, uint16_t magic)
{
	header->magic = host2uint16_t_le(magic);
}

uint16_t ext4_extent_header_get_entries_count(ext4_extent_header_t *header)
{
	return uint16_t_le2host(header->entries_count);
}

void ext4_extent_header_set_entries_count(ext4_extent_header_t *header,
		uint16_t count)
{
	header->entries_count = host2uint16_t_le(count);
}

uint16_t ext4_extent_header_get_max_entries_count(ext4_extent_header_t *header)
{
	return uint16_t_le2host(header->max_entries_count);
}

void ext4_extent_header_set_max_entries_count(ext4_extent_header_t *header,
		uint16_t max_count)
{
	header->max_entries_count = host2uint16_t_le(max_count);
}

uint16_t ext4_extent_header_get_depth(ext4_extent_header_t *header)
{
	return uint16_t_le2host(header->depth);
}

void ext4_extent_header_set_depth(ext4_extent_header_t *header, uint16_t depth)
{
	header->depth = host2uint16_t_le(depth);
}

uint32_t ext4_extent_header_get_generation(ext4_extent_header_t *header)
{
	return uint32_t_le2host(header->generation);
}

void ext4_extent_header_set_generation(ext4_extent_header_t *header,
		uint32_t generation)
{
	header->generation = host2uint32_t_le(generation);
}

/**
 * @}
 */ 
