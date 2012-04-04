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
 * @file	libext4_ialloc.c
 * @brief	Inode (de)allocation operations.
 */

#include <errno.h>
#include <time.h>
#include "libext4.h"

static uint32_t ext4_ialloc_inode2index_in_group(ext4_superblock_t *sb,
		uint32_t inode)
{
	uint32_t inodes_per_group = ext4_superblock_get_inodes_per_group(sb);
	return (inode - 1) % inodes_per_group;
}

static uint32_t ext4_ialloc_index_in_group2inode(ext4_superblock_t *sb,
		uint32_t index, uint32_t bgid)
{
	uint32_t inodes_per_group = ext4_superblock_get_inodes_per_group(sb);
	return bgid * inodes_per_group + (index + 1);
}

static uint32_t ext4_ialloc_get_bgid_of_inode(ext4_superblock_t *sb,
		uint32_t inode)
{
	uint32_t inodes_per_group = ext4_superblock_get_inodes_per_group(sb);
	return (inode - 1) / inodes_per_group;

}


int ext4_ialloc_free_inode(ext4_filesystem_t *fs, uint32_t index, bool is_dir)
{
	int rc;

	ext4_superblock_t *sb = fs->superblock;

	uint32_t block_group = ext4_ialloc_get_bgid_of_inode(sb, index);

	ext4_block_group_ref_t *bg_ref;
	rc = ext4_filesystem_get_block_group_ref(fs, block_group, &bg_ref);
	if (rc != EOK) {
		return rc;
	}

	uint32_t bitmap_block_addr = ext4_block_group_get_inode_bitmap(
			bg_ref->block_group, sb);
	block_t *bitmap_block;
	rc = block_get(&bitmap_block, fs->device, bitmap_block_addr, BLOCK_FLAGS_NONE);
	if (rc != EOK) {
		return rc;
	}

	uint32_t index_in_group = ext4_ialloc_inode2index_in_group(sb, index);
	ext4_bitmap_free_bit(bitmap_block->data, index_in_group);
	bitmap_block->dirty = true;

	rc = block_put(bitmap_block);
	if (rc != EOK) {
		// Error in saving bitmap
		ext4_filesystem_put_block_group_ref(bg_ref);
		return rc;
	}

	// if inode is directory, decrement used directories count
	if (is_dir) {
		uint32_t bg_used_dirs = ext4_block_group_get_used_dirs_count(
			bg_ref->block_group, sb);
		bg_used_dirs--;
		ext4_block_group_set_used_dirs_count(
				bg_ref->block_group, sb, bg_used_dirs);
	}

	// Update block group free inodes count
	uint32_t free_inodes = ext4_block_group_get_free_inodes_count(
			bg_ref->block_group, sb);
	free_inodes++;
	ext4_block_group_set_free_inodes_count(bg_ref->block_group,
			sb, free_inodes);

	if (ext4_block_group_has_flag(bg_ref->block_group, EXT4_BLOCK_GROUP_INODE_UNINIT)) {
		uint32_t unused_inodes = ext4_block_group_get_itable_unused(
				bg_ref->block_group, sb);
		unused_inodes++;
		ext4_block_group_set_itable_unused(bg_ref->block_group, sb, unused_inodes);
	}

	bg_ref->dirty = true;

	rc = ext4_filesystem_put_block_group_ref(bg_ref);
	if (rc != EOK) {
		return rc;
	}

	// Update superblock free inodes count
	uint32_t sb_free_inodes = ext4_superblock_get_free_inodes_count(sb);
	sb_free_inodes++;
	ext4_superblock_set_free_inodes_count(sb, sb_free_inodes);

	return EOK;
}

int ext4_ialloc_alloc_inode(ext4_filesystem_t *fs, uint32_t *index, bool is_dir)
{
	int rc;

	ext4_superblock_t *sb = fs->superblock;

	uint32_t bgid = 0;
	uint32_t bg_count = ext4_superblock_get_block_group_count(sb);
	uint32_t sb_free_inodes = ext4_superblock_get_free_inodes_count(sb);
	uint32_t avg_free_inodes = sb_free_inodes / bg_count;

	while (bgid < bg_count) {

		ext4_block_group_ref_t *bg_ref;
		rc = ext4_filesystem_get_block_group_ref(fs, bgid, &bg_ref);
		if (rc != EOK) {
			return rc;
		}
		ext4_block_group_t *bg = bg_ref->block_group;

		uint32_t free_blocks = ext4_block_group_get_free_blocks_count(bg, sb);
		uint32_t free_inodes = ext4_block_group_get_free_inodes_count(bg, sb);
		uint32_t used_dirs = ext4_block_group_get_used_dirs_count(bg, sb);

		if ((free_inodes >= avg_free_inodes) && (free_blocks > 0)) {

			uint32_t bitmap_block_addr =  ext4_block_group_get_inode_bitmap(
					bg_ref->block_group, sb);

			block_t *bitmap_block;
			rc = block_get(&bitmap_block, fs->device,
					bitmap_block_addr, BLOCK_FLAGS_NONE);
			if (rc != EOK) {
				return rc;
			}

			// Alloc bit
			uint32_t inodes_in_group = ext4_superblock_get_inodes_in_group(sb, bgid);
			uint32_t index_in_group;
			rc = ext4_bitmap_find_free_bit_and_set(
					bitmap_block->data, 0, &index_in_group, inodes_in_group);

			// TODO check
			if (rc == ENOSPC) {
				block_put(bitmap_block);
				ext4_filesystem_put_block_group_ref(bg_ref);
				continue;
			}

			bitmap_block->dirty = true;

			rc = block_put(bitmap_block);
			if (rc != EOK) {
				return rc;
			}

			// Modify filesystem counters
			free_inodes--;
			ext4_block_group_set_free_inodes_count(bg, sb, free_inodes);

			if (ext4_block_group_has_flag(bg, EXT4_BLOCK_GROUP_INODE_UNINIT)) {
				uint16_t unused_inodes = ext4_block_group_get_itable_unused(bg, sb);
				unused_inodes--;
				ext4_block_group_set_itable_unused(bg, sb, unused_inodes);
			}

			if (is_dir) {
				used_dirs++;
				ext4_block_group_set_used_dirs_count(bg, sb, used_dirs);
			}

			bg_ref->dirty = true;

			rc = ext4_filesystem_put_block_group_ref(bg_ref);
			if (rc != EOK) {
				// TODO
				EXT4FS_DBG("ERRRRR");
			}

			sb_free_inodes--;
			ext4_superblock_set_free_inodes_count(sb, sb_free_inodes);

			*index = ext4_ialloc_index_in_group2inode(sb, index_in_group, bgid);

			return EOK;

		}

		// Not modified
		ext4_filesystem_put_block_group_ref(bg_ref);
		++bgid;
	}

	return ENOSPC;
}

/**
 * @}
 */ 
