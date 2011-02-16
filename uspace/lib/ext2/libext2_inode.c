/*
 * Copyright (c) 2011 Martin Sucha
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

/** @addtogroup libext2
 * @{
 */
/**
 * @file
 */

#include "libext2.h"
#include "libext2_inode.h"
#include "libext2_superblock.h"
#include <byteorder.h>

/**
 * Get mode stored in the inode
 * 
 * @param inode pointer to inode
 */
inline uint32_t ext2_inode_get_mode(ext2_superblock_t *sb, ext2_inode_t *inode)
{
	if (ext2_superblock_get_os(sb) == EXT2_SUPERBLOCK_OS_HURD) {
		return ((uint32_t)uint16_t_le2host(inode->mode_high)) << 16 |
		    ((uint32_t)uint16_t_le2host(inode->mode));
	}
	return uint16_t_le2host(inode->mode);
}

/**
 * Get uid this inode is belonging to
 * 
 * @param inode pointer to inode
 */
inline uint32_t ext2_inode_get_user_id(ext2_superblock_t *sb, ext2_inode_t *inode)
{
	uint32_t os = ext2_superblock_get_os(sb);
	if (os == EXT2_SUPERBLOCK_OS_LINUX || os == EXT2_SUPERBLOCK_OS_HURD) {
		return ((uint32_t)uint16_t_le2host(inode->user_id_high)) << 16 |
		    ((uint32_t)uint16_t_le2host(inode->user_id));
	}
	return uint16_t_le2host(inode->user_id);
}

/**
 * Get size of file
 * 
 * For regular files in revision 1 and later, the high 32 bits of
 * file size are stored in inode->size_high and are 0 otherwise
 * 
 * @param inode pointer to inode
 */
inline uint64_t ext2_inode_get_size(ext2_superblock_t *sb, ext2_inode_t *inode)
{
	uint32_t major_rev = ext2_superblock_get_rev_major(sb);
	uint32_t mode = ext2_inode_get_mode(sb, inode);
	
	if (major_rev > 0 && mode & EXT2_INODE_MODE_FILE) {
		return ((uint64_t)uint32_t_le2host(inode->size_high)) << 32 |
		    ((uint64_t)uint32_t_le2host(inode->size));
	}
	return uint32_t_le2host(inode->size);
}

/**
 * Get gid this inode belongs to
 * 
 * For Linux and Hurd, the high 16 bits are stored in OS dependent part
 * of inode structure
 * 
 * @param inode pointer to inode
 */
inline uint32_t ext2_inode_get_group_id(ext2_superblock_t *sb, ext2_inode_t *inode)
{
	uint32_t os = ext2_superblock_get_os(sb);
	if (os == EXT2_SUPERBLOCK_OS_LINUX || os == EXT2_SUPERBLOCK_OS_HURD) {
		return ((uint32_t)uint16_t_le2host(inode->group_id_high)) << 16 |
		    ((uint32_t)uint16_t_le2host(inode->group_id));
	}
	return uint16_t_le2host(inode->group_id);
}

/**
 * Get usage count (i.e. hard link count)
 * A value of 1 is common, while 0 means that the inode should be freed
 * 
 * @param inode pointer to inode
 */
inline uint16_t ext2_inode_get_usage_count(ext2_inode_t *inode)
{
	return uint16_t_le2host(inode->usage_count);
}

/**
 * Get number of 512-byte data blocks allocated for contents of the file
 * represented by this inode.
 * This should be multiple of block size unless fragments are used.
 * 
 * @param inode pointer to inode
 */
inline uint32_t ext2_inode_get_reserved_512_blocks(ext2_inode_t *inode)
{
	return uint32_t_le2host(inode->reserved_512_blocks);
}

/**
 * Get inode flags
 * 
 * @param inode pointer to inode
 */
inline uint32_t ext2_inode_get_flags(ext2_inode_t *inode) {
	return uint32_t_le2host(inode->flags);
}

/**
 * Get direct block ID
 * 
 * @param inode pointer to inode
 * @param idx Index to block. Valid values are 0 <= idx < 12
 */
inline uint32_t ext2_inode_get_direct_block(ext2_inode_t *inode, uint8_t idx)
{
	return uint32_t_le2host(inode->direct_blocks[idx]);
}

/**
 * Get indirect block ID
 * 
 * @param inode pointer to inode
 */
inline uint32_t ext2_inode_get_single_indirect_block(ext2_inode_t *inode)
{
	return uint32_t_le2host(inode->single_indirect_block);
}

/**
 * Get double indirect block ID
 * 
 * @param inode pointer to inode
 */
inline uint32_t ext2_inode_get_double_indirect_block(ext2_inode_t *inode)
{
	return uint32_t_le2host(inode->double_indirect_block);
}

/**
 * Get triple indirect block ID
 * 
 * @param inode pointer to inode
 */
inline uint32_t ext2_inode_get_triple_indirect_block(ext2_inode_t *inode)
{
	return uint32_t_le2host(inode->triple_indirect_block);
}



/** @}
 */
