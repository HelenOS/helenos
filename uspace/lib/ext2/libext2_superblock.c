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
#include <errno.h>
#include <malloc.h>
#include <libblock.h>
#include <byteorder.h>

/**
 * Return a magic number from ext2 superblock, this should be equal to
 * EXT_SUPERBLOCK_MAGIC for valid ext2 superblock
 * 
 * @param sb pointer to superblock
 */
uint16_t ext2_superblock_get_magic(ext2_superblock_t *sb)
{
	return uint16_t_le2host(sb->magic);
}

/**
 * Get the position of first ext2 data block (i.e. the block number
 * containing main superblock)
 * 
 * @param sb pointer to superblock
 */
uint32_t ext2_superblock_get_first_block(ext2_superblock_t *sb)
{
	return uint32_t_le2host(sb->first_block);
}

/**
 * Get the number of bits to shift a value of 1024 to the left necessary
 * to get the size of a block
 * 
 * @param sb pointer to superblock
 */
uint32_t ext2_superblock_get_block_size_log2(ext2_superblock_t *sb)
{
	return uint32_t_le2host(sb->block_size_log2);
}

/**
 * Get the size of a block, in bytes 
 * 
 * @param sb pointer to superblock
 */
uint32_t ext2_superblock_get_block_size(ext2_superblock_t *sb)
{
	return 1024 << ext2_superblock_get_block_size_log2(sb);
}

/**
 * Get the number of bits to shift a value of 1024 to the left necessary
 * to get the size of a fragment (note that this is a signed integer and
 * if negative, the value should be shifted to the right instead)
 * 
 * @param sb pointer to superblock
 */
int32_t ext2_superblock_get_fragment_size_log2(ext2_superblock_t *sb)
{
	return uint32_t_le2host(sb->fragment_size_log2);
}

/**
 * Get the size of a fragment, in bytes 
 * 
 * @param sb pointer to superblock
 */
uint32_t ext2_superblock_get_fragment_size(ext2_superblock_t *sb)
{
	int32_t log = ext2_superblock_get_fragment_size_log2(sb);
	if (log >= 0) {
		return 1024 << log;
	}
	else {
		return 1024 >> -log;
	}
}

/**
 * Get number of blocks per block group
 * 
 * @param sb pointer to superblock
 */
uint32_t ext2_superblock_get_blocks_per_group(ext2_superblock_t *sb)
{
	return uint32_t_le2host(sb->blocks_per_group);
}

/**
 * Get number of fragments per block group
 * 
 * @param sb pointer to superblock
 */
uint32_t ext2_superblock_get_fragments_per_group(ext2_superblock_t *sb)
{
	return uint32_t_le2host(sb->fragments_per_group);
}

/**
 * Get filesystem state
 * 
 * @param sb pointer to superblock
 */
uint16_t ext2_superblock_get_state(ext2_superblock_t *sb)
{
	return uint16_t_le2host(sb->state);
}

/**
 * Get minor revision number
 * 
 * @param sb pointer to superblock
 */
uint16_t ext2_superblock_get_rev_minor(ext2_superblock_t *sb)
{
	return uint16_t_le2host(sb->rev_minor);
}

/**
 * Get major revision number
 * 
 * @param sb pointer to superblock
 */
uint32_t ext2_superblock_get_rev_major(ext2_superblock_t *sb)
{
	return uint32_t_le2host(sb->rev_major);
}

/**
 * Get index of first regular inode
 * 
 * @param sb pointer to superblock
 */
uint32_t ext2_superblock_get_first_inode(ext2_superblock_t *sb)
{
	if (ext2_superblock_get_rev_major(sb) == 0) {
		return EXT2_REV0_FIRST_INODE;
	}
	return uint32_t_le2host(sb->first_inode);
}

/**
 * Get size of inode
 * 
 * @param sb pointer to superblock
 */
uint16_t ext2_superblock_get_inode_size(ext2_superblock_t *sb)
{
	if (ext2_superblock_get_rev_major(sb) == 0) {
		return EXT2_REV0_INODE_SIZE;
	}
	return uint16_t_le2host(sb->inode_size);
}

/**
 * Get total inode count
 * 
 * @param sb pointer to superblock
 */
uint32_t ext2_superblock_get_total_inode_count(ext2_superblock_t *sb)
{
	return uint32_t_le2host(sb->total_inode_count);
}

/**
 * Get total block count
 * 
 * @param sb pointer to superblock
 */
uint32_t ext2_superblock_get_total_block_count(ext2_superblock_t *sb)
{
	return uint32_t_le2host(sb->total_block_count);
}

/**
 * Get amount of blocks reserved for the superuser
 * 
 * @param sb pointer to superblock
 */
uint32_t ext2_superblock_get_reserved_block_count(ext2_superblock_t *sb)
{
	return uint32_t_le2host(sb->reserved_block_count);
}

/**
 * Get amount of free blocks
 * 
 * @param sb pointer to superblock
 */
uint32_t ext2_superblock_get_free_block_count(ext2_superblock_t *sb)
{
	return uint32_t_le2host(sb->free_block_count);
}

/**
 * Get amount of free inodes
 * 
 * @param sb pointer to superblock
 */
uint32_t ext2_superblock_get_free_inode_count(ext2_superblock_t *sb)
{
	return uint32_t_le2host(sb->free_inode_count);
}

/**
 * Get id of operating system that created the filesystem
 * 
 * @param sb pointer to superblock
 */
uint32_t ext2_superblock_get_os(ext2_superblock_t *sb)
{
	return uint32_t_le2host(sb->os);
}

/**
 * Get count of inodes per block group
 * 
 * @param sb pointer to superblock
 */
uint32_t ext2_superblock_get_inodes_per_group(ext2_superblock_t *sb)
{
	return uint32_t_le2host(sb->inodes_per_group);
}

/**
 * Get compatible features flags
 * 
 * @param sb pointer to superblock
 */
uint32_t ext2_superblock_get_features_compatible(ext2_superblock_t *sb)
{
	return uint32_t_le2host(sb->features_compatible);
}

/**
 * Get incompatible features flags
 * 
 * @param sb pointer to superblock
 */
uint32_t ext2_superblock_get_features_incompatible(ext2_superblock_t *sb)
{
	return uint32_t_le2host(sb->features_incompatible);
}

/**
 * Get read-only compatible features flags
 * 
 * @param sb pointer to superblock
 */
uint32_t ext2_superblock_get_features_read_only(ext2_superblock_t *sb)
{
	return uint32_t_le2host(sb->features_read_only);
}

/**
 * Compute count of block groups present in the filesystem
 * 
 * Note: This function works only for correct filesystem,
 *       i.e. it assumes that total block count > 0 and
 *       blocks per group > 0
 * 
 * Example:
 *   If there are 3 blocks per group, the result should be as follows:
 *   Total blocks	Result
 *   1				1
 *   2				1
 *   3				1
 *   4				2
 * 
 * 
 * @param sb pointer to superblock
 */
uint32_t ext2_superblock_get_block_group_count(ext2_superblock_t *sb)
{
	/* We add one to the result because e.g. 2/3 = 0, while to store
	 *  2 blocks in 3-block group we need one (1) block group
	 * 
	 * We subtract one first because of special case that to store e.g.
	 *  3 blocks in a 3-block group we need only one group
	 *  (and 3/3 yields one - this is one more that we want as we
	 *   already add one at the end)
	 */ 
	return ((ext2_superblock_get_total_block_count(sb)-1) / 
	    ext2_superblock_get_blocks_per_group(sb))+1;
}

/** Read a superblock directly from device (i.e. no libblock cache)
 * 
 * @param service_id	Service ID of the block device.
 * @param superblock	Pointer where to store pointer to new superblock
 * 
 * @return		EOK on success or negative error code on failure.
 */
int ext2_superblock_read_direct(service_id_t service_id,
    ext2_superblock_t **superblock)
{
	void *data;
	int rc;
	
	data = malloc(EXT2_SUPERBLOCK_SIZE);
	if (data == NULL) {
		return ENOMEM;
	}
	
	rc = block_read_bytes_direct(service_id, EXT2_SUPERBLOCK_OFFSET,
	    EXT2_SUPERBLOCK_SIZE, data);
	if (rc != EOK) {
		free(data);
		return rc;
	}
	
	(*superblock) = data;
	return EOK;
}

/** Check a superblock for sanity
 * 
 * @param sb	Pointer to superblock
 * 
 * @return		EOK on success or negative error code on failure.
 */
int ext2_superblock_check_sanity(ext2_superblock_t *sb)
{
	if (ext2_superblock_get_magic(sb) != EXT2_SUPERBLOCK_MAGIC) {
		return ENOTSUP;
	}
	
	if (ext2_superblock_get_rev_major(sb) > 1) {
		return ENOTSUP;
	}
	
	if (ext2_superblock_get_total_inode_count(sb) == 0) {
		return ENOTSUP;
	}
	
	if (ext2_superblock_get_total_block_count(sb) == 0) {
		return ENOTSUP;
	}
	
	if (ext2_superblock_get_blocks_per_group(sb) == 0) {
		return ENOTSUP;
	}
	
	if (ext2_superblock_get_fragments_per_group(sb) == 0) {
		return ENOTSUP;
	}
	
	/* We don't support fragments smaller than block */
	if (ext2_superblock_get_block_size(sb) != 
		    ext2_superblock_get_fragment_size(sb)) {
		return ENOTSUP;
	}
	if (ext2_superblock_get_blocks_per_group(sb) !=
		    ext2_superblock_get_fragments_per_group(sb)) {
		return ENOTSUP;
	}
	
	if (ext2_superblock_get_inodes_per_group(sb) == 0) {
		return ENOTSUP;
	}
	
	if (ext2_superblock_get_inode_size(sb) < 128) {
		return ENOTSUP;
	}
	
	if (ext2_superblock_get_first_inode(sb) < 11) {
		return ENOTSUP;
	}
	
	return EOK;
}


/** @}
 */
