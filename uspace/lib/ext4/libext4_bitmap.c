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
 * @file	libext4_bitmap.c
 * @brief	Ext4 bitmap (block & inode) operations.
 */

#include <errno.h>
#include <libblock.h>
#include <sys/types.h>
#include "libext4.h"

static void ext4_bitmap_free_bit(uint8_t *bitmap, uint32_t index)
{
	// Block numbers are 1-based
	uint32_t byte_index = index / 8;
	uint32_t bit_index = index % 8;

//	EXT4FS_DBG("freeing block \%u, byte \%u and bit \%u", index, byte_index, bit_index);

	uint8_t *target = bitmap + byte_index;
	uint8_t old_value = *target;

	uint8_t new_value = old_value & (~ (1 << bit_index));

	*target = new_value;
}

static int ext4_bitmap_find_free_bit_and_set(uint8_t *bitmap, uint32_t *index, uint32_t size)
{
	uint8_t *pos = bitmap;
	int i;
	uint32_t idx = 0;
	uint8_t value, new_value;

	while (pos < bitmap + size) {
		if ((*pos & 255) != 255) {
			// free bit found
			break;
		}

		++pos;
		idx += 8;
	}

	if (pos < bitmap + size) {

//		EXT4FS_DBG("byte found \%u", (uint32_t)(pos - bitmap));

		for(i = 0; i < 8; ++i) {
			value = *pos;

			if ((value & (1 << i)) == 0) {
				// free bit found
//				EXT4FS_DBG("bit found \%u", i);
				new_value = value | (1 << i);
				*pos = new_value;
				*index = idx + i;
				return EOK;
			}
		}
	}

	return ENOSPC;
}

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

	block_size = ext4_superblock_get_block_size(fs->superblock);

	blocks_per_group = ext4_superblock_get_blocks_per_group(fs->superblock);
	block_group = ((block_index - 1) / blocks_per_group);
	index_in_group = (block_index - 1) % blocks_per_group;

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

//	EXT4FS_DBG("block \%u released", block_index);

	return EOK;
}

int ext4_bitmap_alloc_block(ext4_filesystem_t *fs, ext4_inode_ref_t *inode_ref, uint32_t *fblock)
{
	int rc;
	uint32_t inodes_per_group, block_group, blocks_per_group;
	ext4_block_group_ref_t *bg_ref;
	uint32_t bitmap_block;
	block_t *block;
	uint32_t rel_block_idx = 0;
	uint32_t block_size;

	inodes_per_group = ext4_superblock_get_inodes_per_group(fs->superblock);
	block_group = (inode_ref->index - 1) / inodes_per_group;

	block_size = ext4_superblock_get_block_size(fs->superblock);

	rc = ext4_filesystem_get_block_group_ref(fs, block_group, &bg_ref);
	if (rc != EOK) {
		return rc;
	}

	bitmap_block = ext4_block_group_get_block_bitmap(bg_ref->block_group);

	rc = block_get(&block, fs->device, bitmap_block, 0);
	if (rc != EOK) {
		ext4_filesystem_put_block_group_ref(bg_ref);
		return rc;
	}

	rc = ext4_bitmap_find_free_bit_and_set(block->data, &rel_block_idx, block_size);
	if (rc != EOK) {
		EXT4FS_DBG("no free block found");
		// TODO if ENOSPC - try next block groups
	}

	block->dirty = true;

	rc = block_put(block);
	if (rc != EOK) {
		// TODO error
		EXT4FS_DBG("error in saving bitmap \%d", rc);
	}

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

	blocks_per_group = ext4_superblock_get_blocks_per_group(fs->superblock);

//	EXT4FS_DBG("block \%u allocated", blocks_per_group * block_group + rel_block_idx + 1);

	*fblock = blocks_per_group * block_group + rel_block_idx + 1;
	return EOK;

}

/**
 * @}
 */ 
