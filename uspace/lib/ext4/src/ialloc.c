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
 * @file  ialloc.c
 * @brief I-node (de)allocation operations.
 */

#include <errno.h>
#include <stdbool.h>
#include "ext4/bitmap.h"
#include "ext4/block_group.h"
#include "ext4/filesystem.h"
#include "ext4/ialloc.h"
#include "ext4/superblock.h"


/** Convert i-node number to relative index in block group.
 *
 * @param sb    Superblock
 * @param inode I-node number to be converted
 *
 * @return Index of the i-node in the block group
 *
 */
static uint32_t ext4_ialloc_inode2index_in_group(ext4_superblock_t *sb,
    uint32_t inode)
{
	uint32_t inodes_per_group = ext4_superblock_get_inodes_per_group(sb);
	return (inode - 1) % inodes_per_group;
}

/** Convert relative index of i-node to absolute i-node number.
 *
 * @param sb    Superblock
 * @param inode Index to be converted
 *
 * @return Absolute number of the i-node
 *
 */
static uint32_t ext4_ialloc_index_in_group2inode(ext4_superblock_t *sb,
    uint32_t index, uint32_t bgid)
{
	uint32_t inodes_per_group = ext4_superblock_get_inodes_per_group(sb);
	return bgid * inodes_per_group + (index + 1);
}

/** Compute block group number from the i-node number.
 *
 * @param sb    Superblock
 * @param inode I-node number to be found the block group for
 *
 * @return Block group number computed from i-node number
 *
 */
static uint32_t ext4_ialloc_get_bgid_of_inode(ext4_superblock_t *sb,
    uint32_t inode)
{
	uint32_t inodes_per_group = ext4_superblock_get_inodes_per_group(sb);
	return (inode - 1) / inodes_per_group;
}


/** Free i-node number and modify filesystem data structers.
 *
 * @param fs     Filesystem, where the i-node is located
 * @param index  Index of i-node to be release
 * @param is_dir Flag us for information whether i-node is directory or not
 *
 */
errno_t ext4_ialloc_free_inode(ext4_filesystem_t *fs, uint32_t index, bool is_dir)
{
	ext4_superblock_t *sb = fs->superblock;

	/* Compute index of block group and load it */
	uint32_t block_group = ext4_ialloc_get_bgid_of_inode(sb, index);

	ext4_block_group_ref_t *bg_ref;
	errno_t rc = ext4_filesystem_get_block_group_ref(fs, block_group, &bg_ref);
	if (rc != EOK)
		return rc;

	/* Load i-node bitmap */
	uint32_t bitmap_block_addr = ext4_block_group_get_inode_bitmap(
	    bg_ref->block_group, sb);
	block_t *bitmap_block;
	rc = block_get(&bitmap_block, fs->device, bitmap_block_addr,
	    BLOCK_FLAGS_NONE);
	if (rc != EOK)
		return rc;

	/* Free i-node in the bitmap */
	uint32_t index_in_group = ext4_ialloc_inode2index_in_group(sb, index);
	ext4_bitmap_free_bit(bitmap_block->data, index_in_group);
	bitmap_block->dirty = true;

	/* Put back the block with bitmap */
	rc = block_put(bitmap_block);
	if (rc != EOK) {
		/* Error in saving bitmap */
		ext4_filesystem_put_block_group_ref(bg_ref);
		return rc;
	}

	/* If released i-node is a directory, decrement used directories count */
	if (is_dir) {
		uint32_t bg_used_dirs = ext4_block_group_get_used_dirs_count(
		    bg_ref->block_group, sb);
		bg_used_dirs--;
		ext4_block_group_set_used_dirs_count(bg_ref->block_group, sb,
		    bg_used_dirs);
	}

	/* Update block group free inodes count */
	uint32_t free_inodes = ext4_block_group_get_free_inodes_count(
	    bg_ref->block_group, sb);
	free_inodes++;
	ext4_block_group_set_free_inodes_count(bg_ref->block_group, sb,
	    free_inodes);

	bg_ref->dirty = true;

	/* Put back the modified block group */
	rc = ext4_filesystem_put_block_group_ref(bg_ref);
	if (rc != EOK)
		return rc;

	/* Update superblock free inodes count */
	uint32_t sb_free_inodes =
	    ext4_superblock_get_free_inodes_count(sb);
	sb_free_inodes++;
	ext4_superblock_set_free_inodes_count(sb, sb_free_inodes);

	return EOK;
}

/** I-node allocation algorithm.
 *
 * This is more simple algorithm, than Orlov allocator used
 * in the Linux kernel.
 *
 * @param fs     Filesystem to allocate i-node on
 * @param index  Output value - allocated i-node number
 * @param is_dir Flag if allocated i-node will be file or directory
 *
 * @return Error code
 *
 */
errno_t ext4_ialloc_alloc_inode(ext4_filesystem_t *fs, uint32_t *index, bool is_dir)
{
	ext4_superblock_t *sb = fs->superblock;

	uint32_t bgid = 0;
	uint32_t bg_count = ext4_superblock_get_block_group_count(sb);
	uint32_t sb_free_inodes = ext4_superblock_get_free_inodes_count(sb);
	uint32_t avg_free_inodes = sb_free_inodes / bg_count;

	/* Try to find free i-node in all block groups */
	while (bgid < bg_count) {
		/* Load block group to check */
		ext4_block_group_ref_t *bg_ref;
		errno_t rc = ext4_filesystem_get_block_group_ref(fs, bgid, &bg_ref);
		if (rc != EOK)
			return rc;

		ext4_block_group_t *bg = bg_ref->block_group;

		/* Read necessary values for algorithm */
		uint32_t free_blocks = ext4_block_group_get_free_blocks_count(bg, sb);
		uint32_t free_inodes = ext4_block_group_get_free_inodes_count(bg, sb);
		uint32_t used_dirs = ext4_block_group_get_used_dirs_count(bg, sb);

		/*
		 * Check if this block group is a good candidate
		 * for allocation.
		 *
		 * The criterion is based on the average number
		 * of free inodes, unless we examine the last block
		 * group. In that case the last block group might
		 * have less than the average number of free inodes,
		 * but it still needs to be taken as a candidate
		 * because the previous block groups have zero free
		 * blocks.
		 */
		if (((free_inodes >= avg_free_inodes) || (bgid == bg_count - 1)) &&
		    (free_blocks > 0)) {
			/* Load block with bitmap */
			uint32_t bitmap_block_addr = ext4_block_group_get_inode_bitmap(
			    bg_ref->block_group, sb);

			block_t *bitmap_block;
			rc = block_get(&bitmap_block, fs->device, bitmap_block_addr,
			    BLOCK_FLAGS_NONE);
			if (rc != EOK) {
				ext4_filesystem_put_block_group_ref(bg_ref);
				return rc;
			}

			/* Try to allocate i-node in the bitmap */
			uint32_t inodes_in_group = ext4_superblock_get_inodes_in_group(sb, bgid);
			uint32_t index_in_group;
			rc = ext4_bitmap_find_free_bit_and_set(bitmap_block->data,
			    0, &index_in_group, inodes_in_group);

			/* Block group has not any free i-node */
			if (rc == ENOSPC) {
				rc = block_put(bitmap_block);
				if (rc != EOK) {
					ext4_filesystem_put_block_group_ref(bg_ref);
					return rc;
				}

				rc = ext4_filesystem_put_block_group_ref(bg_ref);
				if (rc != EOK)
					return rc;

				bgid++;
				continue;
			}

			/* Free i-node found, save the bitmap */
			bitmap_block->dirty = true;

			rc = block_put(bitmap_block);
			if (rc != EOK) {
				ext4_filesystem_put_block_group_ref(bg_ref);
				return rc;
			}

			/* Modify filesystem counters */
			free_inodes--;
			ext4_block_group_set_free_inodes_count(bg, sb, free_inodes);

			/* Increment used directories counter */
			if (is_dir) {
				used_dirs++;
				ext4_block_group_set_used_dirs_count(bg, sb, used_dirs);
			}

			/* Decrease unused inodes count */
			if (ext4_block_group_has_flag(bg,
			    EXT4_BLOCK_GROUP_ITABLE_ZEROED)) {
				uint32_t unused =
				    ext4_block_group_get_itable_unused(bg, sb);

				uint32_t inodes_in_group =
				    ext4_superblock_get_inodes_in_group(sb, bgid);

				uint32_t free = inodes_in_group - unused;

				if (index_in_group >= free) {
					unused = inodes_in_group - (index_in_group + 1);
					ext4_block_group_set_itable_unused(bg, sb, unused);
				}
			}

			/* Save modified block group */
			bg_ref->dirty = true;

			rc = ext4_filesystem_put_block_group_ref(bg_ref);
			if (rc != EOK)
				return rc;

			/* Update superblock */
			sb_free_inodes--;
			ext4_superblock_set_free_inodes_count(sb, sb_free_inodes);

			/* Compute the absolute i-nodex number */
			*index = ext4_ialloc_index_in_group2inode(sb, index_in_group, bgid);

			return EOK;
		}

		/* Block group not modified, put it and jump to the next block group */
		rc = ext4_filesystem_put_block_group_ref(bg_ref);
		if (rc != EOK)
			return rc;

		++bgid;
	}

	return ENOSPC;
}

/**
 * @}
 */
