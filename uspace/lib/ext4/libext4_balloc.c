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

#include <errno.h>
#include <sys/types.h>
#include "libext4.h"

int ext4_bitmap_free_block(ext4_filesystem_t *fs, ext4_inode_ref_t *inode_ref, uint32_t block_index)
{
	int rc;
	uint32_t blocks_per_group;
	uint32_t block_group;
	uint32_t index_in_group;
	uint32_t bitmap_block;
	uint32_t block_size;
	ext4_block_group_ref_t *bg_ref;
	block_t *block;
	uint32_t first_block;

	block_size = ext4_superblock_get_block_size(fs->superblock);
	blocks_per_group = ext4_superblock_get_blocks_per_group(fs->superblock);
	first_block = ext4_superblock_get_first_data_block(fs->superblock);

	// First block == 0 or 1
	if (first_block == 0) {
		block_group = block_index / blocks_per_group;
		index_in_group = block_index % blocks_per_group;
	} else {
		block_group = (block_index - 1) / blocks_per_group;
		index_in_group = (block_index - 1) % blocks_per_group;
	}

	rc = ext4_filesystem_get_block_group_ref(fs, block_group, &bg_ref);
	if (rc != EOK) {
		return rc;
	}

	bitmap_block = ext4_block_group_get_block_bitmap(bg_ref->block_group);

	rc = block_get(&block, fs->device, bitmap_block, 0);
	if (rc != EOK) {
		return rc;
	}

	ext4_bitmap_free_bit(block->data, index_in_group);

	block->dirty = true;
	rc = block_put(block);
	if (rc != EOK) {
		return rc;
	}

	uint64_t ino_blocks = ext4_inode_get_blocks_count(fs->superblock, inode_ref->inode);
	ino_blocks -= block_size / EXT4_INODE_BLOCK_SIZE;
	ext4_inode_set_blocks_count(fs->superblock, inode_ref->inode, ino_blocks);
	inode_ref->dirty = true;

	uint32_t free_blocks = ext4_block_group_get_free_blocks_count(bg_ref->block_group);
	free_blocks++;
	ext4_block_group_set_free_blocks_count(bg_ref->block_group, free_blocks);
	bg_ref->dirty = true;

	// TODO change free blocks count in superblock

	rc = ext4_filesystem_put_block_group_ref(bg_ref);
	if (rc != EOK) {
		EXT4FS_DBG("error in saving bg_ref \%d", rc);
		// TODO error
		return rc;
	}

	return EOK;
}


static uint32_t ext4_bitmap_find_goal(ext4_filesystem_t *fs, ext4_inode_ref_t *inode_ref)
{
	uint32_t goal = 0;

	int rc;
	uint64_t inode_size = ext4_inode_get_size(fs->superblock, inode_ref->inode);
	uint32_t block_size = ext4_superblock_get_block_size(fs->superblock);
	uint32_t inode_block_count = inode_size / block_size;


	if (inode_size % block_size != 0) {
		inode_block_count++;
	}

	if (inode_block_count > 0) {
		// TODO check retval
//		EXT4FS_DBG("has blocks");
		ext4_filesystem_get_inode_data_block_index(fs, inode_ref->inode, inode_block_count - 1, &goal);

		// TODO
		// If goal == 0 -> SPARSE file !!!
		goal++;
		return goal;
	}

//	uint32_t blocks_per_group = ext4_superblock_get_blocks_per_group(fs->superblock);

	// Identify block group of inode
	uint32_t inodes_per_group = ext4_superblock_get_inodes_per_group(fs->superblock);
	uint32_t block_group = (inode_ref->index - 1) / inodes_per_group;
	block_size = ext4_superblock_get_block_size(fs->superblock);

	ext4_block_group_ref_t *bg_ref;
	rc = ext4_filesystem_get_block_group_ref(fs, block_group, &bg_ref);
	if (rc != EOK) {
		return 0;
	}

	uint32_t inode_table_first_block = ext4_block_group_get_inode_table_first_block(bg_ref->block_group);
	uint16_t inode_table_item_size = ext4_superblock_get_inode_size(fs->superblock);
	uint32_t inode_table_bytes = inodes_per_group * inode_table_item_size;
	uint32_t inode_table_blocks = inode_table_bytes / block_size;

	if (inode_table_bytes % block_size) {
		inode_table_blocks++;
	}

	goal = inode_table_first_block + inode_table_blocks;

	ext4_filesystem_put_block_group_ref(bg_ref);

	return goal;
}

int ext4_bitmap_alloc_block(ext4_filesystem_t *fs, ext4_inode_ref_t *inode_ref, uint32_t *fblock)
{
	int rc;
	ext4_block_group_ref_t *bg_ref;
	uint32_t bitmap_block;
	block_t *block;
	uint32_t rel_block_idx = 0;
	uint32_t index_in_group;
	uint32_t tmp;

	uint32_t allocated_block = 0;

	// Determine GOAL
	uint32_t goal = ext4_bitmap_find_goal(fs, inode_ref);

	uint32_t block_size = ext4_superblock_get_block_size(fs->superblock);

	//if (goal == 0) - unable to determine goal
	if (goal == 0) {
		// TODO
		EXT4FS_DBG("ERRORR (goal == 0)");
	}

//	EXT4FS_DBG("goal = \%u", goal);

	uint32_t blocks_per_group = ext4_superblock_get_blocks_per_group(fs->superblock);
	uint32_t first_block = ext4_superblock_get_first_data_block(fs->superblock);

	uint32_t block_group;

	// First block == 0 or 1
	if (first_block == 0) {
		block_group = goal / blocks_per_group;
		index_in_group = goal % blocks_per_group;
	} else {
		block_group = (goal - 1) / blocks_per_group;
		index_in_group = (goal - 1) % blocks_per_group;
	}


//	EXT4FS_DBG("block_group = \%u, index_in_group = \%u", block_group, index_in_group);

	rc = ext4_filesystem_get_block_group_ref(fs, block_group, &bg_ref);
	if (rc != EOK) {
		return rc;
	}

	// Load bitmap
	bitmap_block = ext4_block_group_get_block_bitmap(bg_ref->block_group);

	rc = block_get(&block, fs->device, bitmap_block, 0);
	if (rc != EOK) {
		ext4_filesystem_put_block_group_ref(bg_ref);
		return rc;
	}

//	EXT4FS_DBG("bitmap loaded");

	if (ext4_bitmap_is_free_bit(block->data, index_in_group)) {

//		EXT4FS_DBG("goal is free");

		ext4_bitmap_set_bit(block->data, index_in_group);
		block->dirty = true;
		rc = block_put(block);
		if (rc != EOK) {
			// TODO error
			EXT4FS_DBG("error in saving bitmap \%d", rc);
		}

		allocated_block = goal;
		goto end;

	}

//	EXT4FS_DBG("try 63 blocks after goal");
	// Try to find free block near to goal
	for (tmp = index_in_group + 1; (tmp < blocks_per_group) && (tmp < ((index_in_group + 63) & ~63)); ++tmp) {

//		EXT4FS_DBG("trying \%u", tmp);

		if (ext4_bitmap_is_free_bit(block->data, tmp)) {

//			EXT4FS_DBG("block \%u is free -> allocate it", tmp);

			ext4_bitmap_set_bit(block->data, tmp);
			block->dirty = true;
			rc = block_put(block);
			if (rc != EOK) {
				// TODO error
				EXT4FS_DBG("error in saving bitmap \%d", rc);
			}

			if (first_block == 0) {
				allocated_block = blocks_per_group * block_group + tmp;
			} else {
				allocated_block = blocks_per_group * block_group + tmp + 1;
			}

			goto end;
		}
	}

//	EXT4FS_DBG("try find free byte");

	// Find free BYTE in bitmap
	rc = ext4_bitmap_find_free_byte_and_set_bit(block->data, index_in_group, &rel_block_idx, block_size);
	if (rc == EOK) {
		block->dirty = true;
		rc = block_put(block);
		if (rc != EOK) {
			// TODO error
			EXT4FS_DBG("error in saving bitmap \%d", rc);
		}

		if (first_block == 0) {
			allocated_block = blocks_per_group * block_group + rel_block_idx;
		} else {
			allocated_block = blocks_per_group * block_group + rel_block_idx + 1;
		}

		goto end;
	}

//	EXT4FS_DBG("try find free bit");

	// Find free bit in bitmap
	rc = ext4_bitmap_find_free_bit_and_set(block->data, index_in_group, &rel_block_idx, block_size);
	if (rc == EOK) {
		block->dirty = true;
		rc = block_put(block);
		if (rc != EOK) {
			// TODO error
			EXT4FS_DBG("error in saving bitmap \%d", rc);
		}

		if (first_block == 0) {
			allocated_block = blocks_per_group * block_group + rel_block_idx;
		} else {
			allocated_block = blocks_per_group * block_group + rel_block_idx + 1;
		}

		goto end;
	}


	// TODO Try other block groups
	EXT4FS_DBG("try other block group");
	return ENOSPC;

end:

	;
//	EXT4FS_DBG("returning block \%u", allocated_block);

	// TODO decrement superblock free blocks count
	//uint32_t sb_free_blocks = ext4_superblock_get_free_blocks_count(sb);
	//sb_free_blocks--;
	//ext4_superblock_set_free_blocks_count(sb, sb_free_blocks);

	uint64_t ino_blocks = ext4_inode_get_blocks_count(fs->superblock, inode_ref->inode);
	ino_blocks += block_size / EXT4_INODE_BLOCK_SIZE;
	ext4_inode_set_blocks_count(fs->superblock, inode_ref->inode, ino_blocks);
	inode_ref->dirty = true;

	uint32_t bg_free_blocks = ext4_block_group_get_free_blocks_count(bg_ref->block_group);
	bg_free_blocks--;
	ext4_block_group_set_free_blocks_count(bg_ref->block_group, bg_free_blocks);
	bg_ref->dirty = true;

	ext4_filesystem_put_block_group_ref(bg_ref);

//	EXT4FS_DBG("block \%u allocated", blocks_per_group * block_group + rel_block_idx + 1);



	*fblock = allocated_block;
	return EOK;
}


/**
 * @file	libext4_balloc.c
 * @brief	TODO
 */



/**
 * @}
 */ 
