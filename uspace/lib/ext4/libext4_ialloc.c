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

static uint32_t ext4_ialloc_get_bgid_of_inode(ext4_superblock_t *sb,
		uint32_t inode)
{
	uint32_t inodes_per_group = ext4_superblock_get_inodes_per_group(sb);
	return (inode - 1) / inodes_per_group;

}


int ext4_ialloc_free_inode(ext4_filesystem_t *fs, ext4_inode_ref_t *inode_ref)
{
	int rc;

	uint32_t block_group = ext4_ialloc_get_bgid_of_inode(
			fs->superblock, inode_ref->index);

	ext4_block_group_ref_t *bg_ref;
	rc = ext4_filesystem_get_block_group_ref(fs, block_group, &bg_ref);
	if (rc != EOK) {
		EXT4FS_DBG("error in loading bg_ref \%d", rc);
		return rc;
	}

	uint32_t bitmap_block_addr = ext4_block_group_get_inode_bitmap(
			bg_ref->block_group, fs->superblock);
	block_t *bitmap_block;
	rc = block_get(&bitmap_block, fs->device, bitmap_block_addr, 0);
	if (rc != EOK) {
		EXT4FS_DBG("error in loading bitmap \%d", rc);
		return rc;
	}

	uint32_t index_in_group = ext4_ialloc_inode2index_in_group(
				fs->superblock, inode_ref->index);
	ext4_bitmap_free_bit(bitmap_block->data, index_in_group);
	bitmap_block->dirty = true;

	rc = block_put(bitmap_block);
	if (rc != EOK) {
		// Error in saving bitmap
		ext4_filesystem_put_block_group_ref(bg_ref);
		EXT4FS_DBG("error in saving bitmap \%d", rc);
		return rc;
	}

	// if inode is directory, decrement directories count
	if (ext4_inode_is_type(fs->superblock, inode_ref->inode, EXT4_INODE_MODE_DIRECTORY)) {
		uint32_t bg_used_dirs = ext4_block_group_get_used_dirs_count(
			bg_ref->block_group, fs->superblock);
		bg_used_dirs--;
		ext4_block_group_set_used_dirs_count(
				bg_ref->block_group, fs->superblock, bg_used_dirs);
	}

	// Update superblock free inodes count
	uint32_t sb_free_inodes = ext4_superblock_get_free_inodes_count(fs->superblock);
	sb_free_inodes++;
	ext4_superblock_set_free_inodes_count(fs->superblock, sb_free_inodes);

	// Update block group free inodes count
	uint32_t free_inodes = ext4_block_group_get_free_inodes_count(
			bg_ref->block_group, fs->superblock);
	free_inodes++;
	ext4_block_group_set_free_inodes_count(bg_ref->block_group,
			fs->superblock, free_inodes);
	bg_ref->dirty = true;

	rc = ext4_filesystem_put_block_group_ref(bg_ref);
	if (rc != EOK) {
		EXT4FS_DBG("error in saving bg_ref \%d", rc);
		// TODO error
		return rc;
	}

	return EOK;
}


/**
 * @}
 */ 
