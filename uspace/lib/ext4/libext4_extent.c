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
 * @file	libext4_extent.c
 * @brief	Ext4 extent structures operations.
 */

#include <byteorder.h>
#include <errno.h>
#include <malloc.h>
#include "libext4.h"

/** Get logical number of the block covered by extent.
 *
 * @param extent	extent to load number from
 * @return			logical number of the first block covered by extent
 */
uint32_t ext4_extent_get_first_block(ext4_extent_t *extent)
{
	return uint32_t_le2host(extent->first_block);
}

/** Set logical number of the first block covered by extent.
 *
 * @param extent	extent to set number to
 * @param iblock	logical number of the first block covered by extent
 */
void ext4_extent_set_first_block(ext4_extent_t *extent, uint32_t iblock)
{
	extent->first_block = host2uint32_t_le(iblock);
}

/** Get number of blocks covered by extent.
 *
 * @param extent	extent to load count from
 * @return			number of blocks covered by extent
 */
uint16_t ext4_extent_get_block_count(ext4_extent_t *extent)
{
	return uint16_t_le2host(extent->block_count);
}

/** Set number of blocks covered by extent.
 *
 * @param extent	extent to load count from
 * @param count 	number of blocks covered by extent
 */
void ext4_extent_set_block_count(ext4_extent_t *extent, uint16_t count)
{
	extent->block_count = host2uint16_t_le(count);
}

/** Get physical number of the first block covered by extent.
 *
 * @param extent 	extent to load number
 * @return			physical number of the first block covered by extent
 */
uint64_t ext4_extent_get_start(ext4_extent_t *extent)
{
	return ((uint64_t)uint16_t_le2host(extent->start_hi)) << 32 |
			((uint64_t)uint32_t_le2host(extent->start_lo));
}

/** Set physical number of the first block covered by extent.
 *
 * @param extent 	extent to load number
 * @param fblock	physical number of the first block covered by extent
 */
void ext4_extent_set_start(ext4_extent_t *extent, uint64_t fblock)
{
	extent->start_lo = host2uint32_t_le((fblock << 32) >> 32);
	extent->start_hi = host2uint16_t_le((uint16_t)(fblock >> 32));
}

/** Get logical number of the block covered by extent index.
 *
 * @param index 	extent index to load number from
 * @return			logical number of the first block covered by extent index
 */
uint32_t ext4_extent_index_get_first_block(ext4_extent_index_t *index)
{
	return uint32_t_le2host(index->first_block);
}

/** Set logical number of the block covered by extent index.
 *
 * @param index 	extent index to set number to
 * @param iblock	logical number of the first block covered by extent index
 */
void ext4_extent_index_set_first_block(ext4_extent_index_t *index,
		uint32_t iblock)
{
	index->first_block = host2uint32_t_le(iblock);
}

/** Get physical number of block where the child node is located.
 *
 * @param index		extent index to load number from
 * @return			physical number of the block with child node
 */
uint64_t ext4_extent_index_get_leaf(ext4_extent_index_t *index)
{
	return ((uint64_t)uint16_t_le2host(index->leaf_hi)) << 32 |
		((uint64_t)uint32_t_le2host(index->leaf_lo));
}

/** Set physical number of block where the child node is located.
 *
 * @param index		extent index to set number to
 * @param fblock	physical number of the block with child node
 */
void ext4_extent_index_set_leaf(ext4_extent_index_t *index, uint64_t fblock)
{
	index->leaf_lo = host2uint32_t_le((fblock << 32) >> 32);
	index->leaf_hi = host2uint16_t_le((uint16_t)(fblock >> 32));
}

/** Get magic value from extent header.
 *
 * @param header	extent header to load value from
 * @return			magic value of extent header
 */
uint16_t ext4_extent_header_get_magic(ext4_extent_header_t *header)
{
	return uint16_t_le2host(header->magic);
}

/** Set magic value to extent header.
 *
 * @param header	extent header to set value to
 * @param magic		magic value of extent header
 */
void ext4_extent_header_set_magic(ext4_extent_header_t *header, uint16_t magic)
{
	header->magic = host2uint16_t_le(magic);
}

/** Get number of entries from extent header
 *
 * @param header	extent header to get value from
 * @return			number of entries covered by extent header
 */
uint16_t ext4_extent_header_get_entries_count(ext4_extent_header_t *header)
{
	return uint16_t_le2host(header->entries_count);
}

/** Set number of entries to extent header
 *
 * @param header	extent header to set value to
 * @param count		number of entries covered by extent header
 */
void ext4_extent_header_set_entries_count(ext4_extent_header_t *header,
		uint16_t count)
{
	header->entries_count = host2uint16_t_le(count);
}

/** Get maximum number of entries from extent header
 *
 * @param header	extent header to get value from
 * @return			maximum number of entries covered by extent header
 */
uint16_t ext4_extent_header_get_max_entries_count(ext4_extent_header_t *header)
{
	return uint16_t_le2host(header->max_entries_count);
}

/** Set maximum number of entries to extent header
 *
 * @param header	extent header to set value to
 * @param max_count maximum number of entries covered by extent header
 */
void ext4_extent_header_set_max_entries_count(ext4_extent_header_t *header,
		uint16_t max_count)
{
	header->max_entries_count = host2uint16_t_le(max_count);
}

/** Get depth of extent subtree.
 *
 * @param header	extent header to get value from
 * @return			depth of extent subtree
 */
uint16_t ext4_extent_header_get_depth(ext4_extent_header_t *header)
{
	return uint16_t_le2host(header->depth);
}

/** Set depth of extent subtree.
 *
 * @param header	extent header to set value to
 * @param depth 	depth of extent subtree
 */
void ext4_extent_header_set_depth(ext4_extent_header_t *header, uint16_t depth)
{
	header->depth = host2uint16_t_le(depth);
}

/** Get generation from extent header
 *
 * @param header 	extent header to get value from
 * @return 			generation
 */
uint32_t ext4_extent_header_get_generation(ext4_extent_header_t *header)
{
	return uint32_t_le2host(header->generation);
}

/** Set generation to extent header
 *
 * @param header 		extent header to set value to
 * @param generation	generation
 */
void ext4_extent_header_set_generation(ext4_extent_header_t *header,
		uint32_t generation)
{
	header->generation = host2uint32_t_le(generation);
}

/** Binary search in extent index node.
 *
 * @param header	extent header of index node
 * @param index		output value - found index will be set here
 * @param iblock	logical block number to find in index node
 */
static void ext4_extent_binsearch_idx(ext4_extent_header_t *header,
	ext4_extent_index_t **index, uint32_t iblock)
{
	ext4_extent_index_t *r, *l, *m;

	uint16_t entries_count = ext4_extent_header_get_entries_count(header);

	// Initialize bounds
	l = EXT4_EXTENT_FIRST_INDEX(header) + 1;
	r = EXT4_EXTENT_FIRST_INDEX(header) + entries_count - 1;

	// Do binary search
	while (l <= r) {
		m = l + (r - l) / 2;
		uint32_t first_block = ext4_extent_index_get_first_block(m);
		if (iblock < first_block) {
				r = m - 1;
		} else {
				l = m + 1;
		}
	}

	// Set output value
	*index = l - 1;
}

/** Binary search in extent leaf node.
 * @param header	extent header of leaf node
 * @param extent	output value - found extent will be set here,
 * 					or NULL if node is empty
 * @param iblock	logical block number to find in leaf node
 *
 */
static void ext4_extent_binsearch(ext4_extent_header_t *header,
		ext4_extent_t **extent, uint32_t iblock)
{
	ext4_extent_t *r, *l, *m;

	uint16_t entries_count = ext4_extent_header_get_entries_count(header);

	if (entries_count == 0) {
		// this leaf is empty
		*extent = NULL;
		return;
	}

	// Initialize bounds
	l = EXT4_EXTENT_FIRST(header) + 1;
	r = EXT4_EXTENT_FIRST(header) + entries_count - 1;

	// Do binary search
	while (l <= r) {
		m = l + (r - l) / 2;
		uint32_t first_block = ext4_extent_get_first_block(m);
		if (iblock < first_block) {
				r = m - 1;
		} else {
				l = m + 1;
		}
	}

	// Set output value
	*extent = l - 1;
}

/** Find physical block in the extent tree by logical block number.
 *
 * There is no need to save path in the tree during this algorithm.
 *
 * @param inode_ref		i-node to load block from
 * @param iblock		logical block number to find
 * @param fblock		output value for physical block number
 * @return				error code
 */
int ext4_extent_find_block(ext4_inode_ref_t *inode_ref,
		uint32_t iblock, uint32_t *fblock)
{
	int rc;

	// Compute bound defined by i-node size
	uint64_t inode_size = ext4_inode_get_size(
			inode_ref->fs->superblock, inode_ref->inode);

	uint32_t block_size = ext4_superblock_get_block_size(
			inode_ref->fs->superblock);

	uint32_t last_idx = (inode_size - 1) / block_size;

	// Check if requested iblock is not over size of i-node
	if (iblock > last_idx) {
		*fblock = 0;
		return EOK;
	}

	block_t* block = NULL;

	// Walk through extent tree
	ext4_extent_header_t *header = ext4_inode_get_extent_header(inode_ref->inode);

//	EXT4FS_DBG("inode = \%u", inode_ref->index);
//	EXT4FS_DBG("count = \%u", ext4_extent_header_get_entries_count(header));

	while (ext4_extent_header_get_depth(header) != 0) {

		// Search index in node
		ext4_extent_index_t *index;
		ext4_extent_binsearch_idx(header, &index, iblock);

		// Load child node and set values for the next iteration
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

	// Search extent in the leaf block
	ext4_extent_t* extent = NULL;
	ext4_extent_binsearch(header, &extent, iblock);

	// Prevent empty leaf
	if (extent == NULL) {
		*fblock = 0;
	} else {

//		EXT4FS_DBG("required = \%u, first = \%u, start = \%u, count = \%u", iblock, ext4_extent_get_first_block(extent), (uint32_t)ext4_extent_get_start(extent), ext4_extent_get_block_count(extent));

		// Compute requested physical block address
		uint32_t phys_block;
		uint32_t first = ext4_extent_get_first_block(extent);
		phys_block = ext4_extent_get_start(extent) + iblock - first;

//		phys_block = ext4_extent_get_start(extent) + iblock;
//		phys_block -= ext4_extent_get_first_block(extent);

		*fblock = phys_block;
	}

	// Cleanup
	if (block != NULL) {
		block_put(block);
	}

	return EOK;
}


/** Find extent for specified iblock.
 *
 * This function is used for finding block in the extent tree with
 * saving the path through the tree for possible future modifications.
 *
 * @param inode_ref		i-node to read extent tree from
 * @param iblock		iblock to find extent for
 * @param ret_path		output value for loaded path from extent tree
 * @return				error code
 */
static int ext4_extent_find_extent(ext4_inode_ref_t *inode_ref,
		uint32_t iblock, ext4_extent_path_t **ret_path)
{
	int rc;

	ext4_extent_header_t *eh =
			ext4_inode_get_extent_header(inode_ref->inode);

	uint16_t depth = ext4_extent_header_get_depth(eh);

	ext4_extent_path_t *tmp_path;

	// Added 2 for possible tree growing
	tmp_path = malloc(sizeof(ext4_extent_path_t) * (depth + 2));
	if (tmp_path == NULL) {
		return ENOMEM;
	}

	// Initialize structure for algorithm start
	tmp_path[0].block = inode_ref->block;
	tmp_path[0].header = eh;

	// Walk through the extent tree
	uint16_t pos = 0;
	while (ext4_extent_header_get_depth(eh) != 0) {

		// Search index in index node by iblock
		ext4_extent_binsearch_idx(tmp_path[pos].header, &tmp_path[pos].index, iblock);

		tmp_path[pos].depth = depth;
		tmp_path[pos].extent = NULL;

		assert(tmp_path[pos].index != NULL);

		// Load information for the next iteration
		uint64_t fblock = ext4_extent_index_get_leaf(tmp_path[pos].index);

		block_t *block;
		rc = block_get(&block, inode_ref->fs->device, fblock, BLOCK_FLAGS_NONE);
		if (rc != EOK) {
			goto cleanup;
		}

		pos++;

		eh = (ext4_extent_header_t *)block->data;
		tmp_path[pos].block = block;
		tmp_path[pos].header = eh;

	}

	tmp_path[pos].depth = 0;
	tmp_path[pos].extent = NULL;
	tmp_path[pos].index = NULL;

    // Find extent in the leaf node
	ext4_extent_binsearch(tmp_path[pos].header, &tmp_path[pos].extent, iblock);

	*ret_path = tmp_path;

	return EOK;

cleanup:
	// Put loaded blocks
	// From 1 -> 0 is a block with inode data
	for (uint16_t i = 1; i < tmp_path->depth; ++i) {
		if (tmp_path[i].block) {
			block_put(tmp_path[i].block);
		}
	}

	// Destroy temporary data structure
	free(tmp_path);

	return rc;
}

/** Release extent and all data blocks covered by the extent.
 *
 * @param inode_ref		i-node to release extent and block from
 * @param extent		extent to release
 * @return				error code
 */
static int ext4_extent_release(
		ext4_inode_ref_t *inode_ref, ext4_extent_t *extent)
{
	int rc;

	// Compute number of the first physical block to release
	uint64_t start = ext4_extent_get_start(extent);
	uint16_t block_count = ext4_extent_get_block_count(extent);

	rc = ext4_balloc_free_blocks(inode_ref, start, block_count);
	if (rc != EOK) {
		EXT4FS_DBG("Error in releasing data blocks");
		return rc;
	}

	return EOK;
}

/** Recursively release the whole branch of the extent tree.
 *
 * For each entry of the node release the subbranch and finally release
 * the node. In the leaf node all extents will be released.
 *
 * @param inode_ref		i-node where the branch is released
 * @param index			index in the non-leaf node to be released
 * 						with the whole subtree
 * @return				error code
 */
static int ext4_extent_release_branch(ext4_inode_ref_t *inode_ref,
		ext4_extent_index_t *index)
{
	int rc;

	block_t* block;

	uint32_t fblock = ext4_extent_index_get_leaf(index);

	rc = block_get(&block, inode_ref->fs->device, fblock, BLOCK_FLAGS_NOREAD);
	if (rc != EOK) {
		EXT4FS_DBG("ERROR get_block");
		return rc;
	}

	ext4_extent_header_t *header = block->data;

	if (ext4_extent_header_get_depth(header)) {

		// The node is non-leaf, do recursion

		ext4_extent_index_t *idx = EXT4_EXTENT_FIRST_INDEX(header);

		// Release all subbranches
		for (uint32_t i = 0; i < ext4_extent_header_get_entries_count(header); ++i, ++idx) {
			rc = ext4_extent_release_branch(inode_ref, idx);
			if (rc != EOK) {
				EXT4FS_DBG("error recursion");
				return rc;
			}
		}
	} else {

		// Leaf node reached
		ext4_extent_t *ext = EXT4_EXTENT_FIRST(header);

		// Release all extents and stop recursion

		for (uint32_t i = 0; i < ext4_extent_header_get_entries_count(header); ++i, ++ext) {
			rc = ext4_extent_release(inode_ref, ext);
			if (rc != EOK) {
				EXT4FS_DBG("error recursion");
				return rc;
			}
		}
	}

	// Release data block where the node was stored

	rc = block_put(block);
	if (rc != EOK) {
		EXT4FS_DBG("ERROR put_block");
		return rc;
	}

	ext4_balloc_free_block(inode_ref, fblock);

	return EOK;
}

/** Release all data blocks starting from specified logical block.
 *
 * @param inode_ref		i-node to release blocks from
 * @param iblock_from	first logical block to release
 */
int ext4_extent_release_blocks_from(ext4_inode_ref_t *inode_ref,
		uint32_t iblock_from)
{
	int rc = EOK;

	// Find the first extent to modify
	ext4_extent_path_t *path;
	rc = ext4_extent_find_extent(inode_ref, iblock_from, &path);
	if (rc != EOK) {
		return rc;
	}

	// Jump to last item of the path (extent)
	ext4_extent_path_t *path_ptr = path;
	while (path_ptr->depth != 0) {
		path_ptr++;
	}

	assert(path_ptr->extent != NULL);

	// First extent maybe released partially
	uint32_t first_fblock;
	first_fblock = ext4_extent_get_start(path_ptr->extent) + iblock_from;
	first_fblock -= ext4_extent_get_first_block(path_ptr->extent);

	uint16_t block_count = ext4_extent_get_block_count(path_ptr->extent);

	uint16_t delete_count = block_count - first_fblock +
			ext4_extent_get_start(path_ptr->extent);

	// Release all blocks
	rc = ext4_balloc_free_blocks(inode_ref, first_fblock, delete_count);
	if (rc != EOK) {
		goto cleanup;
	}

	// Correct counter
	block_count -= delete_count;
	ext4_extent_set_block_count(path_ptr->extent, block_count);

	// Initialize the following loop
	uint16_t entries = ext4_extent_header_get_entries_count(path_ptr->header);
	ext4_extent_t *tmp_ext = path_ptr->extent + 1;
	ext4_extent_t *stop_ext = EXT4_EXTENT_FIRST(path_ptr->header) + entries;

	// If first extent empty, release it
	if (block_count == 0) {
		entries--;
		ext4_extent_header_set_entries_count(path_ptr->header, entries);
	}

	// Release all successors of the first extent in the same node
	while (tmp_ext < stop_ext) {
		first_fblock = ext4_extent_get_start(tmp_ext);
		delete_count = ext4_extent_get_block_count(tmp_ext);

		rc = ext4_balloc_free_blocks(inode_ref, first_fblock, delete_count);
		if (rc != EOK) {
			goto cleanup;
		}

		entries--;
		ext4_extent_header_set_entries_count(path_ptr->header, entries);

		tmp_ext++;
	}

	// If leaf node is empty, the whole tree must be checked and the node will be released
	bool remove_parent_record = false;

	// Don't release root block (including inode data) !!!
	if ((path_ptr != path) && (entries == 0)) {
		rc = ext4_balloc_free_block(inode_ref, path_ptr->block->lba);
		if (rc != EOK) {
			goto cleanup;
		}
		remove_parent_record = true;
	}

	// Jump to the parent
	--path_ptr;

	// release all successors in all tree levels
	while (path_ptr >= path) {
		entries = ext4_extent_header_get_entries_count(path_ptr->header);
		ext4_extent_index_t *index = path_ptr->index + 1;
		ext4_extent_index_t *stop =
				EXT4_EXTENT_FIRST_INDEX(path_ptr->header) + entries;

		// Correct entry because of changes in the previous iteration
		if (remove_parent_record) {
			entries--;
		}

		// Iterate over all entries and release the whole subtrees
		while (index < stop) {
			rc = ext4_extent_release_branch(inode_ref, index);
			if (rc != EOK) {
				goto cleanup;
			}
			++index;
			--entries;
		}

		ext4_extent_header_set_entries_count(path_ptr->header, entries);

		path_ptr->block->dirty = true;

		// Free the node if it is empty
		if ((entries == 0) && (path_ptr != path)) {
			rc = ext4_balloc_free_block(inode_ref, path_ptr->block->lba);
			if (rc != EOK) {
				goto cleanup;
			}

			// Mark parent to be checked
			remove_parent_record = true;
		} else {
			remove_parent_record = false;
		}

		--path_ptr;
	}


cleanup:
	// Put loaded blocks
	// From 1 -> 0 is a block with inode data
	for (uint16_t i = 1; i <= path->depth; ++i) {
		if (path[i].block) {
			block_put(path[i].block);
		}
	}

	// Destroy temporary data structure
	free(path);

	return rc;
}


/** Append new extent to the i-node and do some splitting if necessary.
 *
 * @param inode_ref			i-node to append extent to
 * @param path				path in the extent tree for possible splitting
 * @param last_path_item	input/output parameter for pointer to the last
 * 							valid item in the extent tree path
 * @param iblock			logical index of block to append extent for
 * @return					error code
 */
static int ext4_extent_append_extent(ext4_inode_ref_t *inode_ref,
		ext4_extent_path_t *path, ext4_extent_path_t **last_path_item,
		uint32_t iblock)
{
	EXT4FS_DBG("");
	int rc;

	ext4_extent_path_t *path_ptr = *last_path_item;

	uint16_t entries = ext4_extent_header_get_entries_count(path_ptr->header);
	uint16_t limit = ext4_extent_header_get_max_entries_count(path_ptr->header);

	// Trivial way - no splitting
	if (entries < limit) {
		EXT4FS_DBG("adding extent entry");

		ext4_extent_header_set_entries_count(path_ptr->header, entries + 1);
		path_ptr->extent = EXT4_EXTENT_FIRST(path_ptr->header) + entries;
		ext4_extent_set_block_count(path_ptr->extent, 0);
		ext4_extent_set_first_block(path_ptr->extent, iblock);
		ext4_extent_set_start(path_ptr->extent, 0);
		path_ptr->block->dirty = true;

		return EOK;
	}

	uint32_t block_size =
			ext4_superblock_get_block_size(inode_ref->fs->superblock);

	// Trivial tree - grow (extents were in root node)
	if (path_ptr == path) {

		uint32_t new_fblock;
		rc = ext4_balloc_alloc_block(inode_ref, &new_fblock);
		if (rc != EOK) {
			EXT4FS_DBG("error in block allocation");
			return rc;
		}

		block_t *block;
		rc = block_get(&block, inode_ref->fs->device,
				new_fblock, BLOCK_FLAGS_NOREAD);
		if (rc != EOK) {
			EXT4FS_DBG("error in block_get");
			return rc;
		}

		memset(block->data, 0, block_size);

		// Move data from root to the new block
		memcpy(block->data, inode_ref->inode->blocks,
				EXT4_INODE_BLOCKS * sizeof(uint32_t));

		path_ptr++;
		path_ptr->block = block;
		path_ptr->header = (ext4_extent_header_t *)block->data;
		path_ptr->depth = ext4_extent_header_get_depth(path_ptr->header);
		path_ptr->index = NULL;

		uint16_t entries = ext4_extent_header_get_entries_count(path_ptr->header);
		path_ptr->extent = EXT4_EXTENT_FIRST(path_ptr->header) + entries;
		ext4_extent_header_set_entries_count(path_ptr->header, entries + 1);
		uint16_t limit = (block_size - sizeof(ext4_extent_header_t)) /
				sizeof(ext4_extent_t);
		ext4_extent_header_set_max_entries_count(path_ptr->header, limit);

		// Modify root (in inode)
		path->depth = 1;
		path->extent = NULL;
		path->index = EXT4_EXTENT_FIRST_INDEX(path->header);

		ext4_extent_header_set_depth(path->header, path_ptr->depth + 1);
		ext4_extent_header_set_entries_count(path->header, 1);

		ext4_extent_index_set_first_block(path->index, 0);
		ext4_extent_index_set_leaf(path->index, new_fblock);

		path_ptr->block->dirty = true;
		path->block->dirty = true;

		*last_path_item = path_ptr;

		return EOK;
	}

	assert(false);

	// start splitting
	uint32_t fblock = 0;
	while (path_ptr > path) {

		// TODO

		rc = ext4_balloc_alloc_block(inode_ref, &fblock);
		if (rc != EOK) {
			return rc;
		}

		block_t *block;
		rc = block_get(&block, inode_ref->fs->device, fblock, BLOCK_FLAGS_NOREAD);
		if (rc != EOK) {
			ext4_balloc_free_block(inode_ref, fblock);
			return rc;
		}

		// Init block
		memset(block->data, 0, block_size);

		// Not modified
		block_put(path_ptr->block);
		path_ptr->block = block;

		path_ptr->header = block->data;

		if (path_ptr->depth) {
			path_ptr->index = EXT4_EXTENT_FIRST_INDEX(path_ptr->header);
		} else {
			path_ptr->extent = EXT4_EXTENT_FIRST(path_ptr->header);
		}

		path_ptr--;
	}

	// If splitting reached root node
	if (path_ptr == path) {

		uint32_t new_fblock;
		rc = ext4_balloc_alloc_block(inode_ref, &new_fblock);
		if (rc != EOK) {
			EXT4FS_DBG("error in block allocation");
			return rc;
		}

		block_t *block;
		rc = block_get(&block, inode_ref->fs->device,
				new_fblock, BLOCK_FLAGS_NOREAD);
		if (rc != EOK) {
			EXT4FS_DBG("error in block_get");
			return rc;
		}

		memset(block->data, 0, block_size);

		// Move data from root to the new block
		memcpy(block->data, inode_ref->inode->blocks,
				EXT4_INODE_BLOCKS * sizeof(uint32_t));

		path_ptr++;
		path_ptr->block = block;
		path_ptr->header = (ext4_extent_header_t *)block->data;
		path_ptr->depth = ext4_extent_header_get_depth(path_ptr->header);
		path_ptr->index = NULL;

		uint16_t entries = ext4_extent_header_get_entries_count(path_ptr->header);
		path_ptr->extent = EXT4_EXTENT_FIRST(path_ptr->header) + entries;
		ext4_extent_header_set_entries_count(path_ptr->header, entries + 1);
		uint16_t limit = (block_size - sizeof(ext4_extent_header_t)) /
				sizeof(ext4_extent_t);
		ext4_extent_header_set_max_entries_count(path_ptr->header, limit);

		// Modify root (in inode)
		path->depth = 1;
		path->extent = NULL;
		path->index = EXT4_EXTENT_FIRST_INDEX(path->header);

		ext4_extent_header_set_depth(path->header, path_ptr->depth + 1);
		ext4_extent_header_set_entries_count(path->header, 1);

		ext4_extent_index_set_first_block(path->index, 0);
		ext4_extent_index_set_leaf(path->index, new_fblock);

		path_ptr->block->dirty = true;
		path->block->dirty = true;

		*last_path_item = path_ptr;

		return EOK;
	}

	return EOK;
}

/** Append data block to the i-node.
 *
 * This function allocates data block, tries to append it
 * to some existing extent or creates new extents.
 * It includes possible extent tree modifications (splitting).
 *
 * @param inode_ref			i-node to append block to
 * @param iblock			output logical number of newly allocated block
 * @param fblock			output physical block address of newly allocated block
 * @return					error code
 */
int ext4_extent_append_block(ext4_inode_ref_t *inode_ref,
		uint32_t *iblock, uint32_t *fblock)
{
	EXT4FS_DBG("");
	int rc = EOK;

	ext4_superblock_t *sb = inode_ref->fs->superblock;
	uint64_t inode_size = ext4_inode_get_size(sb, inode_ref->inode);
	uint32_t block_size = ext4_superblock_get_block_size(sb);

	// Calculate number of new logical block
	uint32_t new_block_idx = 0;
	if (inode_size > 0) {
		if ((inode_size % block_size) != 0) {
			inode_size += block_size - (inode_size % block_size);
		}
		new_block_idx = inode_size / block_size;
	}

	// Load the nearest leaf (with extent)
	ext4_extent_path_t *path;
	rc = ext4_extent_find_extent(inode_ref, new_block_idx, &path);
	if (rc != EOK) {
		return rc;
	}

	// Jump to last item of the path (extent)
	ext4_extent_path_t *path_ptr = path;
	while (path_ptr->depth != 0) {
		path_ptr++;
	}

	// Add new extent to the node if not present
	if (path_ptr->extent == NULL) {
		goto append_extent;
	}

	uint16_t block_count = ext4_extent_get_block_count(path_ptr->extent);
	uint16_t block_limit = (1 << 15);

	uint32_t phys_block = 0;
	if (block_count < block_limit) {

		// There is space for new block in the extent

		if (block_count == 0) {

			// Existing extent is empty

			rc = ext4_balloc_alloc_block(inode_ref, &phys_block);
			if (rc != EOK) {
				goto finish;
			}

			// Initialize extent
			ext4_extent_set_first_block(path_ptr->extent, new_block_idx);
			ext4_extent_set_start(path_ptr->extent, phys_block);
			ext4_extent_set_block_count(path_ptr->extent, 1);

			// Update i-node
			ext4_inode_set_size(inode_ref->inode, inode_size + block_size);
			inode_ref->dirty = true;

			path_ptr->block->dirty = true;

			goto finish;
		} else {

			// Existing extent contains some blocks

			phys_block = ext4_extent_get_start(path_ptr->extent);
			phys_block += ext4_extent_get_block_count(path_ptr->extent);

			// Check if the following block is free for allocation
			bool free;
			rc = ext4_balloc_try_alloc_block(inode_ref, phys_block, &free);
			if (rc != EOK) {
				goto finish;
			}

			if (! free) {
				// target is not free, new block must be appended to new extent
				goto append_extent;
			}


			// Update extent
			ext4_extent_set_block_count(path_ptr->extent, block_count + 1);

			// Update i-node
			ext4_inode_set_size(inode_ref->inode, inode_size + block_size);
			inode_ref->dirty = true;

			path_ptr->block->dirty = true;

			goto finish;
		}
	}

// Append new extent to the tree
append_extent:

	phys_block = 0;

	// Allocate new data block
	rc = ext4_balloc_alloc_block(inode_ref, &phys_block);
	if (rc != EOK) {
		EXT4FS_DBG("error in block allocation, rc = \%d", rc);
		goto finish;
	}

	// Append extent for new block (includes tree splitting if needed)
	rc = ext4_extent_append_extent(inode_ref, path, &path_ptr, new_block_idx);
	if (rc != EOK) {
		ext4_balloc_free_block(inode_ref, phys_block);
		goto finish;
	}

	// Initialize newly created extent
	ext4_extent_set_block_count(path_ptr->extent, 1);
	ext4_extent_set_first_block(path_ptr->extent, new_block_idx);
	ext4_extent_set_start(path_ptr->extent, phys_block);

	// Update i-node
	ext4_inode_set_size(inode_ref->inode, inode_size + block_size);
	inode_ref->dirty = true;

	path_ptr->block->dirty = true;


finish:
	// Set return values
	*iblock = new_block_idx;
	*fblock = phys_block;

	// Put loaded blocks
	// From 1 -> 0 is a block with inode data
	for (uint16_t i = 1; i <= path->depth; ++i) {
		if (path[i].block) {
			block_put(path[i].block);
		}
	}

	// Destroy temporary data structure
	free(path);

	return rc;
}

/**
 * @}
 */ 
