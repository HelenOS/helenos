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
 * @file  inode.c
 * @brief Ext4 i-node structure operations.
 */

#include <block.h>
#include <byteorder.h>
#include <errno.h>
#include "ext4/inode.h"
#include "ext4/superblock.h"

/** Compute number of bits for block count.
 *
 *  @param block_size Filesystem block_size
 *
 *  @return Number of bits
 *
 */
static uint32_t ext4_inode_block_bits_count(uint32_t block_size)
{
	uint32_t bits = 8;
	uint32_t size = block_size;

	do {
		bits++;
		size = size >> 1;
	} while (size > 256);

	return bits;
}

/** Get mode of the i-node.
 *
 * @param sb    Superblock
 * @param inode I-node to load mode from
 *
 * @return Mode of the i-node
 *
 */
uint32_t ext4_inode_get_mode(ext4_superblock_t *sb, ext4_inode_t *inode)
{
	if (ext4_superblock_get_creator_os(sb) == EXT4_SUPERBLOCK_OS_HURD) {
		return ((uint32_t) uint16_t_le2host(inode->osd2.hurd2.mode_high)) << 16 |
		    ((uint32_t) uint16_t_le2host(inode->mode));
	}

	return uint16_t_le2host(inode->mode);
}

/** Set mode of the i-node.
 *
 * @param sb    Superblock
 * @param inode I-node to set mode to
 * @param mode  Mode to set to i-node
 *
 */
void ext4_inode_set_mode(ext4_superblock_t *sb, ext4_inode_t *inode, uint32_t mode)
{
	inode->mode = host2uint16_t_le((mode << 16) >> 16);

	if (ext4_superblock_get_creator_os(sb) == EXT4_SUPERBLOCK_OS_HURD)
		inode->osd2.hurd2.mode_high = host2uint16_t_le(mode >> 16);
}

/** Get ID of the i-node owner (user id).
 *
 * @param inode I-node to load uid from
 *
 * @return User ID of the i-node owner
 *
 */
uint32_t ext4_inode_get_uid(ext4_inode_t *inode)
{
	return uint32_t_le2host(inode->uid);
}

/** Set ID of the i-node owner.
 *
 * @param inode I-node to set uid to
 * @param uid   ID of the i-node owner
 *
 */
void ext4_inode_set_uid(ext4_inode_t *inode, uint32_t uid)
{
	inode->uid = host2uint32_t_le(uid);
}

/** Get real i-node size.
 *
 * @param sb    Superblock
 * @param inode I-node to load size from
 *
 * @return Real size of i-node
 *
 */
uint64_t ext4_inode_get_size(ext4_superblock_t *sb, ext4_inode_t *inode)
{
	uint32_t major_rev = ext4_superblock_get_rev_level(sb);

	if ((major_rev > 0) &&
	    (ext4_inode_is_type(sb, inode, EXT4_INODE_MODE_FILE)))
		return ((uint64_t)uint32_t_le2host(inode->size_hi)) << 32 |
		    ((uint64_t)uint32_t_le2host(inode->size_lo));

	return uint32_t_le2host(inode->size_lo);
}

/** Set real i-node size.
 *
 * @param inode I-node to set size to
 * @param size  Size of the i-node
 *
 */
void ext4_inode_set_size(ext4_inode_t *inode, uint64_t size)
{
	inode->size_lo = host2uint32_t_le((size << 32) >> 32);
	inode->size_hi = host2uint32_t_le(size >> 32);
}

/** Get time, when i-node was last accessed.
 *
 * @param inode I-node
 *
 * @return Time of the last access (POSIX)
 *
 */
uint32_t ext4_inode_get_access_time(ext4_inode_t *inode)
{
	return uint32_t_le2host(inode->access_time);
}

/** Set time, when i-node was last accessed.
 *
 * @param inode I-node
 * @param time  Time of the last access (POSIX)
 *
 */
void ext4_inode_set_access_time(ext4_inode_t *inode, uint32_t time)
{
	inode->access_time = host2uint32_t_le(time);
}

/** Get time, when i-node was last changed.
 *
 * @param inode I-node
 *
 * @return Time of the last change (POSIX)
 *
 */
uint32_t ext4_inode_get_change_inode_time(ext4_inode_t *inode)
{
	return uint32_t_le2host(inode->change_inode_time);
}

/** Set time, when i-node was last changed.
 *
 * @param inode I-node
 * @param time  Time of the last change (POSIX)
 *
 */
void ext4_inode_set_change_inode_time(ext4_inode_t *inode, uint32_t time)
{
	inode->change_inode_time = host2uint32_t_le(time);
}

/** Get time, when i-node content was last modified.
 *
 * @param inode I-node
 *
 * @return Time of the last content modification (POSIX)
 *
 */
uint32_t ext4_inode_get_modification_time(ext4_inode_t *inode)
{
	return uint32_t_le2host(inode->modification_time);
}

/** Set time, when i-node content was last modified.
 *
 * @param inode I-node
 * @param time  Time of the last content modification (POSIX)
 *
 */
void ext4_inode_set_modification_time(ext4_inode_t *inode, uint32_t time)
{
	inode->modification_time = host2uint32_t_le(time);
}

/** Get time, when i-node was deleted.
 *
 * @param inode I-node
 *
 * @return Time of the delete action (POSIX)
 *
 */
uint32_t ext4_inode_get_deletion_time(ext4_inode_t *inode)
{
	return uint32_t_le2host(inode->deletion_time);
}

/** Set time, when i-node was deleted.
 *
 * @param inode I-node
 * @param time  Time of the delete action (POSIX)
 *
 */
void ext4_inode_set_deletion_time(ext4_inode_t *inode, uint32_t time)
{
	inode->deletion_time = host2uint32_t_le(time);
}

/** Get ID of the i-node owner's group.
 *
 * @param inode I-node to load gid from
 *
 * @return Group ID of the i-node owner
 *
 */
uint32_t ext4_inode_get_gid(ext4_inode_t *inode)
{
	return uint32_t_le2host(inode->gid);
}

/** Set ID ot the i-node owner's group.
 *
 * @param inode I-node to set gid to
 * @param gid   Group ID of the i-node owner
 *
 */
void ext4_inode_set_gid(ext4_inode_t *inode, uint32_t gid)
{
	inode->gid = host2uint32_t_le(gid);
}

/** Get number of links to i-node.
 *
 * @param inode I-node to load number of links from
 *
 * @return Number of links to i-node
 *
 */
uint16_t ext4_inode_get_links_count(ext4_inode_t *inode)
{
	return uint16_t_le2host(inode->links_count);
}

/** Set number of links to i-node.
 *
 * @param inode I-node to set number of links to
 * @param count Number of links to i-node
 *
 */
void ext4_inode_set_links_count(ext4_inode_t *inode, uint16_t count)
{
	inode->links_count = host2uint16_t_le(count);
}

/** Get number of 512-bytes blocks used for i-node.
 *
 * @param sb    Superblock
 * @param inode I-node
 *
 * @return Number of 512-bytes blocks
 *
 */
uint64_t ext4_inode_get_blocks_count(ext4_superblock_t *sb, ext4_inode_t *inode)
{
	if (ext4_superblock_has_feature_read_only(sb,
	    EXT4_FEATURE_RO_COMPAT_HUGE_FILE)) {
		/* 48-bit field */
		uint64_t count = ((uint64_t)
		    uint16_t_le2host(inode->osd2.linux2.blocks_high)) << 32 |
		    uint32_t_le2host(inode->blocks_count_lo);

		if (ext4_inode_has_flag(inode, EXT4_INODE_FLAG_HUGE_FILE)) {
			uint32_t block_size = ext4_superblock_get_block_size(sb);
			uint32_t block_bits = ext4_inode_block_bits_count(block_size);
			return count << (block_bits - 9);
		} else
			return count;
	} else
		return uint32_t_le2host(inode->blocks_count_lo);
}

/** Set number of 512-bytes blocks used for i-node.
 *
 * @param sb    Superblock
 * @param inode I-node
 * @param count Number of 512-bytes blocks
 *
 * @return Error code
 *
 */
errno_t ext4_inode_set_blocks_count(ext4_superblock_t *sb, ext4_inode_t *inode,
    uint64_t count)
{
	/* 32-bit maximum */
	uint64_t max = 0;
	max = ~max >> 32;

	if (count <= max) {
		inode->blocks_count_lo = host2uint32_t_le(count);
		inode->osd2.linux2.blocks_high = 0;
		ext4_inode_clear_flag(inode, EXT4_INODE_FLAG_HUGE_FILE);

		return EOK;
	}

	/* Check if there can be used huge files (many blocks) */
	if (!ext4_superblock_has_feature_read_only(sb,
	    EXT4_FEATURE_RO_COMPAT_HUGE_FILE))
		return EINVAL;

	/* 48-bit maximum */
	max = 0;
	max = ~max >> 16;

	if (count <= max) {
		inode->blocks_count_lo = host2uint32_t_le(count);
		inode->osd2.linux2.blocks_high = host2uint16_t_le(count >> 32);
		ext4_inode_clear_flag(inode, EXT4_INODE_FLAG_HUGE_FILE);
	} else {
		uint32_t block_size = ext4_superblock_get_block_size(sb);
		uint32_t block_bits = ext4_inode_block_bits_count(block_size);
		ext4_inode_set_flag(inode, EXT4_INODE_FLAG_HUGE_FILE);
		count = count >> (block_bits - 9);
		inode->blocks_count_lo = host2uint32_t_le(count);
		inode->osd2.linux2.blocks_high = host2uint16_t_le(count >> 32);
	}

	return EOK;
}

/** Get flags (features) of i-node.
 *
 * @param inode I-node to get flags from
 *
 * @return Flags (bitmap)
 *
 */
uint32_t ext4_inode_get_flags(ext4_inode_t *inode)
{
	return uint32_t_le2host(inode->flags);
}

/** Set flags (features) of i-node.
 *
 * @param inode I-node to set flags to
 * @param flags Flags to set to i-node
 *
 */
void ext4_inode_set_flags(ext4_inode_t *inode, uint32_t flags)
{
	inode->flags = host2uint32_t_le(flags);
}

/** Get file generation (used by NFS).
 *
 * @param inode I-node
 *
 * @return File generation
 *
 */
uint32_t ext4_inode_get_generation(ext4_inode_t *inode)
{
	return uint32_t_le2host(inode->generation);
}

/** Set file generation (used by NFS).
 *
 * @param inode      I-node
 * @param generation File generation
 *
 */
void ext4_inode_set_generation(ext4_inode_t *inode, uint32_t generation)
{
	inode->generation = host2uint32_t_le(generation);
}

/** Get address of block, where are extended attributes located.
 *
 * @param inode I-node
 * @param sb    Superblock
 *
 * @return Block address
 *
 */
uint64_t ext4_inode_get_file_acl(ext4_inode_t *inode, ext4_superblock_t *sb)
{
	if (ext4_superblock_get_creator_os(sb) == EXT4_SUPERBLOCK_OS_LINUX)
		return ((uint32_t)
		    uint16_t_le2host(inode->osd2.linux2.file_acl_high)) << 16 |
		    (uint32_t_le2host(inode->file_acl_lo));

	return uint32_t_le2host(inode->file_acl_lo);
}

/** Set address of block, where are extended attributes located.
 *
 * @param inode    I-node
 * @param sb       Superblock
 * @param file_acl Block address
 *
 */
void ext4_inode_set_file_acl(ext4_inode_t *inode, ext4_superblock_t *sb,
    uint64_t file_acl)
{
	inode->file_acl_lo = host2uint32_t_le((file_acl << 32) >> 32);

	if (ext4_superblock_get_creator_os(sb) == EXT4_SUPERBLOCK_OS_LINUX)
		inode->osd2.linux2.file_acl_high = host2uint16_t_le(file_acl >> 32);
}

/** Get block address of specified direct block.
 *
 * @param inode I-node to load block from
 * @param idx   Index of logical block
 *
 * @return Physical block address
 *
 */
uint32_t ext4_inode_get_direct_block(ext4_inode_t *inode, uint32_t idx)
{
	assert(idx < EXT4_INODE_DIRECT_BLOCK_COUNT);

	return uint32_t_le2host(inode->blocks[idx]);
}

/** Set block address of specified direct block.
 *
 * @param inode  I-node to set block address to
 * @param idx    Index of logical block
 * @param fblock Physical block address
 *
 */
void ext4_inode_set_direct_block(ext4_inode_t *inode, uint32_t idx, uint32_t fblock)
{
	assert(idx < EXT4_INODE_DIRECT_BLOCK_COUNT);

	inode->blocks[idx] = host2uint32_t_le(fblock);
}

/** Get block address of specified indirect block.
 *
 * @param inode I-node to get block address from
 * @param idx   Index of indirect block
 *
 * @return Physical block address
 *
 */
uint32_t ext4_inode_get_indirect_block(ext4_inode_t *inode, uint32_t idx)
{
	return uint32_t_le2host(inode->blocks[idx + EXT4_INODE_INDIRECT_BLOCK]);
}

/** Set block address of specified indirect block.
 *
 * @param inode  I-node to set block address to
 * @param idx    Index of indirect block
 * @param fblock Physical block address
 *
 */
void ext4_inode_set_indirect_block(ext4_inode_t *inode, uint32_t idx,
    uint32_t fblock)
{
	inode->blocks[idx + EXT4_INODE_INDIRECT_BLOCK] =
	    host2uint32_t_le(fblock);
}

/** Check if i-node has specified type.
 *
 * @param sb    Superblock
 * @param inode I-node to check type of
 * @param type  Type to check
 *
 * @return Result of check operation
 *
 */
bool ext4_inode_is_type(ext4_superblock_t *sb, ext4_inode_t *inode,
    uint32_t type)
{
	uint32_t mode = ext4_inode_get_mode(sb, inode);
	return (mode & EXT4_INODE_MODE_TYPE_MASK) == type;
}

/** Get extent header from the root of the extent tree.
 *
 * @param inode I-node to get extent header from
 *
 * @return Pointer to extent header of the root node
 *
 */
ext4_extent_header_t * ext4_inode_get_extent_header(ext4_inode_t *inode)
{
	return (ext4_extent_header_t *) inode->blocks;
}

/** Check if i-node has specified flag.
 *
 * @param inode I-node to check flags of
 * @param flag  Flag to check
 *
 * @return Result of check operation
 *
 */
bool ext4_inode_has_flag(ext4_inode_t *inode, uint32_t flag)
{
	if (ext4_inode_get_flags(inode) & flag)
		return true;

	return false;
}

/** Remove specified flag from i-node.
 *
 * @param inode      I-node to clear flag on
 * @param clear_flag Flag to be cleared
 *
 */
void ext4_inode_clear_flag(ext4_inode_t *inode, uint32_t clear_flag)
{
	uint32_t flags = ext4_inode_get_flags(inode);
	flags = flags & (~clear_flag);
	ext4_inode_set_flags(inode, flags);
}

/** Set specified flag to i-node.
 *
 * @param inode    I-node to set flag on
 * @param set_flag Flag to be set
 *
 */
void ext4_inode_set_flag(ext4_inode_t *inode, uint32_t set_flag)
{
	uint32_t flags = ext4_inode_get_flags(inode);
	flags = flags | set_flag;
	ext4_inode_set_flags(inode, flags);
}

/** Check if i-node can be truncated.
 *
 * @param sb    Superblock
 * @param inode I-node to check
 *
 * @return Result of the check operation
 *
 */
bool ext4_inode_can_truncate(ext4_superblock_t *sb, ext4_inode_t *inode)
{
	if ((ext4_inode_has_flag(inode, EXT4_INODE_FLAG_APPEND)) ||
	    (ext4_inode_has_flag(inode, EXT4_INODE_FLAG_IMMUTABLE)))
		return false;

	if ((ext4_inode_is_type(sb, inode, EXT4_INODE_MODE_FILE)) ||
	    (ext4_inode_is_type(sb, inode, EXT4_INODE_MODE_DIRECTORY)))
		return true;

	return false;
}

/**
 * @}
 */
