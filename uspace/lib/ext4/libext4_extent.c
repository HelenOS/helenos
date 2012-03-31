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
#include <errno.h>
#include <malloc.h>
#include "libext4.h"

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
 * Binary search in extent index node
 */
static void ext4_extent_binsearch_idx(ext4_extent_header_t *header,
	ext4_extent_index_t **index, uint32_t iblock)
{
	ext4_extent_index_t *r, *l, *m;

	uint16_t entries_count = ext4_extent_header_get_entries_count(header);

	if (entries_count == 1) {
		*index = EXT4_EXTENT_FIRST_INDEX(header);
		return;
	}

	l = EXT4_EXTENT_FIRST_INDEX(header) + 1;
	r = l + entries_count - 1;

	while (l <= r) {
		m = l + (r - l) / 2;
		uint32_t first_block = ext4_extent_index_get_first_block(m);
		if (iblock < first_block) {
				r = m - 1;
		} else {
				l = m + 1;
		}
	}

	*index = l - 1;
}

/**
 * Binary search in extent leaf node
 */
static void ext4_extent_binsearch(ext4_extent_header_t *header,
		ext4_extent_t **extent, uint32_t iblock)
{
	ext4_extent_t *r, *l, *m;

	uint16_t entries_count = ext4_extent_header_get_entries_count(header);

	if (entries_count == 0) {
		// this leaf is empty
		EXT4FS_DBG("EMPTY LEAF");
		*extent = NULL;
		return;
	}

	if (entries_count == 1) {
		*extent = EXT4_EXTENT_FIRST(header);
		return;
	}

	l = EXT4_EXTENT_FIRST(header) + 1;
	r = l + entries_count - 1;

	while (l <= r) {
		m = l + (r - l) / 2;
		uint32_t first_block = ext4_extent_get_first_block(m);
		if (iblock < first_block) {
				r = m - 1;
		} else {
				l = m + 1;
		}
	}

	*extent = l - 1;
}

// Reading routine without saving blocks to path - for saving memory during finding block
int ext4_extent_find_block(ext4_inode_ref_t *inode_ref, uint32_t iblock, uint32_t *fblock)
{
	int rc;

	block_t* block = NULL;

	ext4_extent_header_t *header = ext4_inode_get_extent_header(inode_ref->inode);
	while (ext4_extent_header_get_depth(header) != 0) {

		ext4_extent_index_t *index;
		ext4_extent_binsearch_idx(header, &index, iblock);

		uint64_t child = ext4_extent_index_get_leaf(index);

		if (block != NULL) {
			block_put(block);
		}

		rc = block_get(&block, inode_ref->fs->device, child, BLOCK_FLAGS_NONE);
		if (rc != EOK) {
			return rc;
		}

		header = (ext4_extent_header_t *)block->data;
	}


	ext4_extent_t* extent;
	ext4_extent_binsearch(header, &extent, iblock);


	uint32_t phys_block;
	phys_block = ext4_extent_get_start(extent) + iblock;
	phys_block -= ext4_extent_get_first_block(extent);

	*fblock = phys_block;

	if (block != NULL) {
		block_put(block);
	}


	return EOK;
}

//static int ext4_extent_find_extent(ext4_filesystem_t *fs,
//		ext4_inode_ref_t *inode_ref, uint32_t iblock, ext4_extent_path_t **ret_path)
//{
//	int rc;
//
//	ext4_extent_header_t *eh =
//			ext4_inode_get_extent_header(inode_ref->inode);
//
//	uint16_t depth = ext4_extent_header_get_depth(eh);
//
//	ext4_extent_path_t *tmp_path;
//
//	// Added 2 for possible tree growing
//	tmp_path = malloc(sizeof(ext4_extent_path_t) * (depth + 2));
//	if (tmp_path == NULL) {
//		return ENOMEM;
//	}
//
//	tmp_path[0].block = inode_ref->block;
//	tmp_path[0].header = eh;
//
//	uint16_t pos = 0;
//	while (ext4_extent_header_get_depth(eh) != 0) {
//
//		ext4_extent_binsearch_idx(tmp_path[pos].header, &tmp_path[pos].index, iblock);
//
//		tmp_path[pos].depth = depth;
//		tmp_path[pos].extent = NULL;
//
//		assert(tmp_path[pos].index != NULL);
//
//		uint64_t fblock = ext4_extent_index_get_leaf(tmp_path[pos].index);
//
//		block_t *block;
//		rc = block_get(&block, fs->device, fblock, BLOCK_FLAGS_NONE);
//		if (rc != EOK) {
//			// TODO cleanup
//			EXT4FS_DBG("ERRRR");
//			return rc;
//		}
//
//		pos++;
//
//		eh = (ext4_extent_header_t *)block->data;
//		tmp_path[pos].block = block;
//		tmp_path[pos].header = eh;
//
//	}
//
//	tmp_path[pos].depth = 0;
//	tmp_path[pos].extent = NULL;
//	tmp_path[pos].index = NULL;
//
//    /* find extent */
//	ext4_extent_binsearch(tmp_path[pos].header, &tmp_path[pos].extent, iblock);
//
//	*ret_path = tmp_path;
//
//	return EOK;
//}


//int ext4_extent_find_block(ext4_filesystem_t *fs,
//		ext4_inode_ref_t *inode_ref, uint32_t iblock, uint32_t *fblock)
//{
//	int rc;
//
//	ext4_extent_path_t *path;
//	rc = ext4_extent_find_extent(fs, inode_ref, iblock, &path);
//	if (rc != EOK) {
//		return rc;
//	}
//
//	uint16_t depth = path->depth;
//
//	ext4_extent_t *extent = path[depth].extent;
//
//	uint32_t phys_block;
//	phys_block = ext4_extent_get_start(extent) + iblock;
//	phys_block -= ext4_extent_get_first_block(extent);
//
//	*fblock = phys_block;
//
//	// Put loaded blocks
//	// From 1 -> 0 is a block with inode data
//	for (uint16_t i = 1; i < depth; ++i) {
//		if (path[i].block) {
//			block_put(path[i].block);
//		}
//	}
//
//	// Destroy temporary data structure
//	free(path);
//
//	return EOK;
//
//}

/**
 * @}
 */ 
