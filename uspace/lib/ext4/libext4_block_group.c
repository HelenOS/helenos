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
 * @file	libext4_block_group.c
 * @brief	Ext4 block group structure operations
 */

#include <byteorder.h>
#include "libext4.h"

uint64_t ext4_block_group_get_block_bitmap(ext4_block_group_t *bg,
		ext4_superblock_t *sb)
{
	if (ext4_superblock_get_desc_size(sb) > EXT4_BLOCK_MIN_GROUP_DESCRIPTOR_SIZE) {
		return ((uint64_t)uint32_t_le2host(bg->block_bitmap_hi) << 32) |
			uint32_t_le2host(bg->block_bitmap_lo);
	} else {
		return uint32_t_le2host(bg->block_bitmap_lo);
	}
}

void ext4_block_group_set_block_bitmap(ext4_block_group_t *bg,
		ext4_superblock_t *sb, uint64_t block_bitmap)
{
	bg->block_bitmap_lo = host2uint32_t_le((block_bitmap << 32) >> 32);

	if (ext4_superblock_get_desc_size(sb) > EXT4_BLOCK_MIN_GROUP_DESCRIPTOR_SIZE) {
		bg->block_bitmap_hi = host2uint32_t_le(block_bitmap >> 32);
	}
}

uint64_t ext4_block_group_get_inode_bitmap(ext4_block_group_t *bg,
		ext4_superblock_t *sb)
{
	if (ext4_superblock_get_desc_size(sb) > EXT4_BLOCK_MIN_GROUP_DESCRIPTOR_SIZE) {
		return ((uint64_t)uint32_t_le2host(bg->inode_bitmap_hi) << 32) |
			uint32_t_le2host(bg->inode_bitmap_lo);
	} else {
		return uint32_t_le2host(bg->inode_bitmap_lo);
	}

}

void ext4_block_group_set_inode_bitmap(ext4_block_group_t *bg,
		ext4_superblock_t *sb, uint64_t inode_bitmap)
{
	bg->inode_bitmap_lo = host2uint32_t_le((inode_bitmap << 32) >> 32);

	if (ext4_superblock_get_desc_size(sb) > EXT4_BLOCK_MIN_GROUP_DESCRIPTOR_SIZE) {
		bg->inode_bitmap_hi = host2uint32_t_le(inode_bitmap >> 32);
	}
}

uint64_t ext4_block_group_get_inode_table_first_block(ext4_block_group_t *bg,
		ext4_superblock_t *sb)
{
	if (ext4_superblock_get_desc_size(sb) > EXT4_BLOCK_MIN_GROUP_DESCRIPTOR_SIZE) {
		return ((uint64_t)uint32_t_le2host(bg->inode_table_first_block_hi) << 32) |
			uint32_t_le2host(bg->inode_table_first_block_lo);
	} else {
		return uint32_t_le2host(bg->inode_table_first_block_lo);
	}
}

void ext4_block_group_set_inode_table_first_block(ext4_block_group_t *bg,
		ext4_superblock_t *sb, uint64_t ino_tbl_first)
{
	bg->inode_table_first_block_lo = host2uint32_t_le((ino_tbl_first << 32) >> 32);

	if (ext4_superblock_get_desc_size(sb) > EXT4_BLOCK_MIN_GROUP_DESCRIPTOR_SIZE) {
		bg->inode_table_first_block_hi = host2uint32_t_le(ino_tbl_first >> 32);
	}
}

uint32_t ext4_block_group_get_free_blocks_count(ext4_block_group_t *bg,
		ext4_superblock_t *sb)
{
	if (ext4_superblock_get_desc_size(sb) > EXT4_BLOCK_MIN_GROUP_DESCRIPTOR_SIZE) {
		return ((uint32_t)uint16_t_le2host(bg->free_blocks_count_hi) << 16) |
			uint16_t_le2host(bg->free_blocks_count_lo);
	} else {
		return uint16_t_le2host(bg->free_blocks_count_lo);
	}
}

void ext4_block_group_set_free_blocks_count(ext4_block_group_t *bg,
		ext4_superblock_t *sb, uint32_t value)
{
	bg->free_blocks_count_lo = host2uint16_t_le((value << 16) >> 16);
	if (ext4_superblock_get_desc_size(sb) > EXT4_BLOCK_MIN_GROUP_DESCRIPTOR_SIZE) {
		bg->free_blocks_count_hi = host2uint16_t_le(value >> 16);
	}
}

uint32_t ext4_block_group_get_free_inodes_count(ext4_block_group_t *bg,
		ext4_superblock_t *sb)
{
	if (ext4_superblock_get_desc_size(sb) > EXT4_BLOCK_MIN_GROUP_DESCRIPTOR_SIZE) {
		return ((uint32_t)uint16_t_le2host(bg->free_inodes_count_hi) << 16) |
			uint16_t_le2host(bg->free_inodes_count_lo);
	} else {
		return uint16_t_le2host(bg->free_inodes_count_lo);
	}
}

void ext4_block_group_set_free_inodes_count(ext4_block_group_t *bg,
		ext4_superblock_t *sb, uint32_t value)
{
	bg->free_inodes_count_lo = host2uint16_t_le((value << 16) >> 16);
	if (ext4_superblock_get_desc_size(sb) > EXT4_BLOCK_MIN_GROUP_DESCRIPTOR_SIZE) {
		bg->free_inodes_count_hi = host2uint16_t_le(value >> 16);
	}
}


uint32_t ext4_block_group_get_used_dirs_count(ext4_block_group_t *bg,
		ext4_superblock_t *sb)
{
	if (ext4_superblock_get_desc_size(sb) > EXT4_BLOCK_MIN_GROUP_DESCRIPTOR_SIZE) {
		return ((uint32_t)uint16_t_le2host(bg->used_dirs_count_hi) << 16) |
			uint16_t_le2host(bg->used_dirs_count_lo);
	} else {
		return uint16_t_le2host(bg->used_dirs_count_lo);
	}
}

void ext4_block_group_set_used_dirs_count(ext4_block_group_t *bg,
		ext4_superblock_t *sb, uint32_t count)
{
	bg->used_dirs_count_lo = host2uint16_t_le((count << 16) >> 16);
	if (ext4_superblock_get_desc_size(sb) > EXT4_BLOCK_MIN_GROUP_DESCRIPTOR_SIZE) {
		bg->used_dirs_count_hi = host2uint16_t_le(count >> 16);
	}
}

uint16_t ext4_block_group_get_flags(ext4_block_group_t *bg)
{
	return uint16_t_le2host(bg->flags);
}

uint32_t ext4_block_group_get_itable_unused(ext4_block_group_t *bg,
		ext4_superblock_t *sb)
{
	if (ext4_superblock_get_desc_size(sb) > EXT4_BLOCK_MIN_GROUP_DESCRIPTOR_SIZE) {
		return ((uint32_t)uint16_t_le2host(bg->itable_unused_hi) << 16) |
			uint16_t_le2host(bg->itable_unused_lo);
	} else {
		return uint16_t_le2host(bg->itable_unused_lo);
	}
}

void ext4_block_group_set_itable_unused(ext4_block_group_t *bg,
		ext4_superblock_t *sb, uint32_t value)
{
	bg->itable_unused_lo = host2uint16_t_le((value << 16) >> 16);
	if (ext4_superblock_get_desc_size(sb) > EXT4_BLOCK_MIN_GROUP_DESCRIPTOR_SIZE) {
		bg->itable_unused_hi = host2uint16_t_le(value >> 16);
	}

}

uint16_t ext4_block_group_get_checksum(ext4_block_group_t *bg)
{
	return uint16_t_le2host(bg->checksum);
}

void ext4_block_group_set_checksum(ext4_block_group_t *bg, uint16_t checksum)
{
	bg->checksum = host2uint16_t_le(checksum);
}

/**
 * @}
 */ 
