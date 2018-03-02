/*
 * Copyright (c) 2011 Martin Sucha
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
 * @file  block_group.c
 * @brief Ext4 block group structure operations.
 */

#include <byteorder.h>
#include "ext4/block_group.h"
#include "ext4/superblock.h"

/** Get address of block with data block bitmap.
 *
 * @param bg Pointer to block group
 * @param sb Pointer to superblock
 *
 * @return Address of block with block bitmap
 *
 */
uint64_t ext4_block_group_get_block_bitmap(ext4_block_group_t *bg,
    ext4_superblock_t *sb)
{
	if (ext4_superblock_get_desc_size(sb) >
	    EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		return ((uint64_t) uint32_t_le2host(bg->block_bitmap_hi) << 32) |
		    uint32_t_le2host(bg->block_bitmap_lo);
	else
		return uint32_t_le2host(bg->block_bitmap_lo);
}

/** Set address of block with data block bitmap.
 *
 * @param bg           Pointer to block group
 * @param sb           Pointer to superblock
 * @param block_bitmap Address of block with block bitmap
 *
 */
void ext4_block_group_set_block_bitmap(ext4_block_group_t *bg,
    ext4_superblock_t *sb, uint64_t block_bitmap)
{
	bg->block_bitmap_lo = host2uint32_t_le((block_bitmap << 32) >> 32);

	if (ext4_superblock_get_desc_size(sb) >
	    EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		bg->block_bitmap_hi = host2uint32_t_le(block_bitmap >> 32);
}

/** Get address of block with i-node bitmap.
 *
 * @param bg Pointer to block group
 * @param sb Pointer to superblock
 *
 * @return Address of block with i-node bitmap
 *
 */
uint64_t ext4_block_group_get_inode_bitmap(ext4_block_group_t *bg,
    ext4_superblock_t *sb)
{
	if (ext4_superblock_get_desc_size(sb) >
	    EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		return ((uint64_t) uint32_t_le2host(bg->inode_bitmap_hi) << 32) |
		    uint32_t_le2host(bg->inode_bitmap_lo);
	else
		return uint32_t_le2host(bg->inode_bitmap_lo);
}

/** Set address of block with i-node bitmap.
 *
 * @param bg           Pointer to block group
 * @param sb           Pointer to superblock
 * @param inode_bitmap Address of block with i-node bitmap
 *
 */
void ext4_block_group_set_inode_bitmap(ext4_block_group_t *bg,
    ext4_superblock_t *sb, uint64_t inode_bitmap)
{
	bg->inode_bitmap_lo = host2uint32_t_le((inode_bitmap << 32) >> 32);

	if (ext4_superblock_get_desc_size(sb) >
	    EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		bg->inode_bitmap_hi = host2uint32_t_le(inode_bitmap >> 32);
}

/** Get address of the first block of the i-node table.
 *
 * @param bg Pointer to block group
 * @param sb Pointer to superblock
 *
 * @return Address of first block of i-node table
 *
 */
uint64_t ext4_block_group_get_inode_table_first_block(ext4_block_group_t *bg,
    ext4_superblock_t *sb)
{
	if (ext4_superblock_get_desc_size(sb) >
	    EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		return ((uint64_t)
		    uint32_t_le2host(bg->inode_table_first_block_hi) << 32) |
		    uint32_t_le2host(bg->inode_table_first_block_lo);
	else
		return uint32_t_le2host(bg->inode_table_first_block_lo);
}

/** Set address of the first block of the i-node table.
 *
 * @param bg                Pointer to block group
 * @param sb                Pointer to superblock
 * @param inode_table_first Address of first block of i-node table
 *
 */
void ext4_block_group_set_inode_table_first_block(ext4_block_group_t *bg,
    ext4_superblock_t *sb, uint64_t inode_table_first)
{
	bg->inode_table_first_block_lo =
	    host2uint32_t_le((inode_table_first << 32) >> 32);

	if (ext4_superblock_get_desc_size(sb) >
	    EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		bg->inode_table_first_block_hi =
		    host2uint32_t_le(inode_table_first >> 32);
}

/** Get number of free blocks in block group.
 *
 * @param bg Pointer to block group
 * @param sb Pointer to superblock
 *
 * @return Number of free blocks in block group
 *
 */
uint32_t ext4_block_group_get_free_blocks_count(ext4_block_group_t *bg,
    ext4_superblock_t *sb)
{
	if (ext4_superblock_get_desc_size(sb) >
	    EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		return ((uint32_t)
		    uint16_t_le2host(bg->free_blocks_count_hi) << 16) |
		    uint16_t_le2host(bg->free_blocks_count_lo);
	else
		return uint16_t_le2host(bg->free_blocks_count_lo);
}

/** Set number of free blocks in block group.
 *
 * @param bg    Pointer to block group
 * @param sb    Pointer to superblock
 * @param value Number of free blocks in block group
 *
 */
void ext4_block_group_set_free_blocks_count(ext4_block_group_t *bg,
    ext4_superblock_t *sb, uint32_t value)
{
	bg->free_blocks_count_lo = host2uint16_t_le((value << 16) >> 16);
	if (ext4_superblock_get_desc_size(sb) >
	    EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		bg->free_blocks_count_hi = host2uint16_t_le(value >> 16);
}

/** Get number of free i-nodes in block group.
 *
 * @param bg Pointer to block group
 * @param sb Pointer to superblock
 *
 * @return Number of free i-nodes in block group
 *
 */
uint32_t ext4_block_group_get_free_inodes_count(ext4_block_group_t *bg,
    ext4_superblock_t *sb)
{
	if (ext4_superblock_get_desc_size(sb) >
	    EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		return ((uint32_t)
		    uint16_t_le2host(bg->free_inodes_count_hi) << 16) |
		    uint16_t_le2host(bg->free_inodes_count_lo);
	else
		return uint16_t_le2host(bg->free_inodes_count_lo);
}

/** Set number of free i-nodes in block group.
 *
 * @param bg    Pointer to block group
 * @param sb    Pointer to superblock
 * @param value Number of free i-nodes in block group
 *
 */
void ext4_block_group_set_free_inodes_count(ext4_block_group_t *bg,
    ext4_superblock_t *sb, uint32_t value)
{
	bg->free_inodes_count_lo = host2uint16_t_le((value << 16) >> 16);
	if (ext4_superblock_get_desc_size(sb) >
	    EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		bg->free_inodes_count_hi = host2uint16_t_le(value >> 16);
}

/** Get number of used directories in block group.
 *
 * @param bg Pointer to block group
 * @param sb Pointer to superblock
 *
 * @return Number of used directories in block group
 *
 */
uint32_t ext4_block_group_get_used_dirs_count(ext4_block_group_t *bg,
    ext4_superblock_t *sb)
{
	if (ext4_superblock_get_desc_size(sb) >
	    EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		return ((uint32_t)
		    uint16_t_le2host(bg->used_dirs_count_hi) << 16) |
		    uint16_t_le2host(bg->used_dirs_count_lo);
	else
		return uint16_t_le2host(bg->used_dirs_count_lo);
}

/** Set number of used directories in block group.
 *
 * @param bg    Pointer to block group
 * @param sb    Pointer to superblock
 * @param value Number of used directories in block group
 *
 */
void ext4_block_group_set_used_dirs_count(ext4_block_group_t *bg,
    ext4_superblock_t *sb, uint32_t count)
{
	bg->used_dirs_count_lo = host2uint16_t_le((count << 16) >> 16);
	if (ext4_superblock_get_desc_size(sb) >
	    EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		bg->used_dirs_count_hi = host2uint16_t_le(count >> 16);
}

/** Get flags of block group.
 *
 * @param bg Pointer to block group
 *
 * @return Flags of block group
 *
 */
uint16_t ext4_block_group_get_flags(ext4_block_group_t *bg)
{
	return uint16_t_le2host(bg->flags);
}

/** Set flags for block group.
 *
 * @param bg    Pointer to block group
 * @param flags Flags for block group
 *
 */
void ext4_block_group_set_flags(ext4_block_group_t *bg, uint16_t flags)
{
	bg->flags = host2uint16_t_le(flags);
}

/** Get number of unused i-nodes.
 *
 * @param bg Pointer to block group
 * @param sb Pointer to superblock
 *
 * @return Number of unused i-nodes
 *
 */
uint32_t ext4_block_group_get_itable_unused(ext4_block_group_t *bg,
    ext4_superblock_t *sb)
{
	if (ext4_superblock_get_desc_size(sb) >
	    EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		return ((uint32_t)
		    uint16_t_le2host(bg->itable_unused_hi) << 16) |
		    uint16_t_le2host(bg->itable_unused_lo);
	else
		return uint16_t_le2host(bg->itable_unused_lo);
}

/** Set number of unused i-nodes.
 *
 * @param bg    Pointer to block group
 * @param sb    Pointer to superblock
 * @param value Number of unused i-nodes
 *
 */
void ext4_block_group_set_itable_unused(ext4_block_group_t *bg,
    ext4_superblock_t *sb, uint32_t value)
{
	bg->itable_unused_lo = host2uint16_t_le((value << 16) >> 16);
	if (ext4_superblock_get_desc_size(sb) >
	    EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		bg->itable_unused_hi = host2uint16_t_le(value >> 16);
}

/** Get checksum of block group.
 *
 * @param bg Pointer to block group
 *
 * @return checksum of block group
 *
 */
uint16_t ext4_block_group_get_checksum(ext4_block_group_t *bg)
{
	return uint16_t_le2host(bg->checksum);
}

/** Set checksum of block group.
 *
 * @param bg       Pointer to block group
 * @param checksum Cheksum of block group
 *
 */
void ext4_block_group_set_checksum(ext4_block_group_t *bg, uint16_t checksum)
{
	bg->checksum = host2uint16_t_le(checksum);
}

/** Check if block group has a flag.
 *
 * @param bg   Pointer to block group
 * @param flag Flag to be checked
 *
 * @return True if flag is set to 1
 *
 */
bool ext4_block_group_has_flag(ext4_block_group_t *bg, uint32_t flag)
{
	if (ext4_block_group_get_flags(bg) & flag)
		return true;

	return false;
}

/** Set (add) flag of block group.
 *
 * @param bg   Pointer to block group
 * @param flag Flag to be set
 *
 */
void ext4_block_group_set_flag(ext4_block_group_t *bg, uint32_t set_flag)
{
	uint32_t flags = ext4_block_group_get_flags(bg);
	flags = flags | set_flag;
	ext4_block_group_set_flags(bg, flags);
}

/** Clear (remove) flag of block group.
 *
 * @param bg   Pointer to block group
 * @param flag Flag to be cleared
 *
 */
void ext4_block_group_clear_flag(ext4_block_group_t *bg, uint32_t clear_flag)
{
	uint32_t flags = ext4_block_group_get_flags(bg);
	flags = flags & (~clear_flag);
	ext4_block_group_set_flags(bg, flags);
}

/**
 * @}
 */
