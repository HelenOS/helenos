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

#include "libext2_filesystem.h"
#include "libext2_superblock.h"
#include "libext2_block_group.h"
#include "libext2_inode.h"
#include <errno.h>
#include <libblock.h>
#include <malloc.h>
#include <assert.h>
#include <byteorder.h>

/**
 * Initialize an instance of filesystem on the device.
 * This function reads superblock from the device and
 * initializes libblock cache with appropriate logical block size.
 * 
 * @param fs			Pointer to ext2_filesystem_t to initialize
 * @param service_id	Service ID of the block device
 * 
 * @return 		EOK on success or negative error code on failure
 */
int ext2_filesystem_init(ext2_filesystem_t *fs, service_id_t service_id)
{
	int rc;
	ext2_superblock_t *temp_superblock;
	size_t block_size;
	
	fs->device = service_id;
	
	rc = block_init(EXCHANGE_SERIALIZE, fs->device, 2048);
	if (rc != EOK) {
		return rc;
	}
	
	rc = ext2_superblock_read_direct(fs->device, &temp_superblock);
	if (rc != EOK) {
		block_fini(fs->device);
		return rc;
	}
	
	block_size = ext2_superblock_get_block_size(temp_superblock);
	
	if (block_size > EXT2_MAX_BLOCK_SIZE) {
		block_fini(fs->device);
		return ENOTSUP;
	}
	
	rc = block_cache_init(service_id, block_size, 0, CACHE_MODE_WT);
	if (rc != EOK) {
		block_fini(fs->device);
		return rc;
	}
	
	fs->superblock = temp_superblock;
	
	return EOK; 
}

/**
 * Check filesystem for sanity
 * 
 * @param fs			Pointer to ext2_filesystem_t to check
 * @return 		EOK on success or negative error code on failure
 */
int ext2_filesystem_check_sanity(ext2_filesystem_t *fs)
{
	int rc;
	
	rc = ext2_superblock_check_sanity(fs->superblock);
	if (rc != EOK) {
		return rc;
	}
	
	return EOK;
}

/**
 * Check feature flags
 * 
 * @param fs Pointer to ext2_filesystem_t to check
 * @param read_only bool to set to true if the fs needs to be read-only
 * @return EOK on success or negative error code on failure
 */
int ext2_filesystem_check_flags(ext2_filesystem_t *fs, bool *o_read_only)
{
	/* feature flags are present in rev 1 and later */
	if (ext2_superblock_get_rev_major(fs->superblock) == 0) {
		*o_read_only = false;
		return EOK;
	}
	
	uint32_t incompatible;
	uint32_t read_only;
	
	incompatible = ext2_superblock_get_features_incompatible(fs->superblock);
	read_only = ext2_superblock_get_features_read_only(fs->superblock);
	
	/* check whether we support all features
	 * first unset any supported feature flags
	 * and see whether any unspported feature remains */
	incompatible &= ~EXT2_SUPPORTED_INCOMPATIBLE_FEATURES;
	read_only &= ~EXT2_SUPPORTED_READ_ONLY_FEATURES;
	
	if (incompatible > 0) {
		*o_read_only = true;
		return ENOTSUP;
	}
	
	if (read_only > 0) {
		*o_read_only = true;
	}
	
	return EOK;
}

/**
 * Get a reference to block descriptor
 * 
 * @param fs Pointer to filesystem information
 * @param bgid Index of block group to find
 * @param ref Pointer where to store pointer to block group reference
 * 
 * @return 		EOK on success or negative error code on failure
 */
int ext2_filesystem_get_block_group_ref(ext2_filesystem_t *fs, uint32_t bgid,
    ext2_block_group_ref_t **ref)
{
	int rc;
	aoff64_t block_id;
	uint32_t descriptors_per_block;
	size_t offset;
	ext2_block_group_ref_t *newref;
	
	newref = malloc(sizeof(ext2_block_group_ref_t));
	if (newref == NULL) {
		return ENOMEM;
	}
	
	descriptors_per_block = ext2_superblock_get_block_size(fs->superblock)
	    / EXT2_BLOCK_GROUP_DESCRIPTOR_SIZE;
	
	/* Block group descriptor table starts at the next block after superblock */
	block_id = ext2_superblock_get_first_block(fs->superblock) + 1;
	
	/* Find the block containing the descriptor we are looking for */
	block_id += bgid / descriptors_per_block;
	offset = (bgid % descriptors_per_block) * EXT2_BLOCK_GROUP_DESCRIPTOR_SIZE;
	
	rc = block_get(&newref->block, fs->device, block_id, 0);
	if (rc != EOK) {
		free(newref);
		return rc;
	}
	
	newref->block_group = newref->block->data + offset;
	
	*ref = newref;
	
	return EOK;
}

/**
 * Free a reference to block group
 * 
 * @param ref Pointer to block group reference to free
 * 
 * @return 		EOK on success or negative error code on failure
 */
int ext2_filesystem_put_block_group_ref(ext2_block_group_ref_t *ref)
{
	int rc;
	
	rc = block_put(ref->block);
	free(ref);
	
	return rc;
}

/**
 * Get a reference to inode
 * 
 * @param fs Pointer to filesystem information
 * @param index The index number of the inode
 * @param ref Pointer where to store pointer to inode reference
 * 
 * @return 		EOK on success or negative error code on failure
 */
int ext2_filesystem_get_inode_ref(ext2_filesystem_t *fs, uint32_t index,
    ext2_inode_ref_t **ref)
{
	int rc;
	aoff64_t block_id;
	uint32_t block_group;
	uint32_t offset_in_group;
	uint32_t byte_offset_in_group;
	size_t offset_in_block;
	uint32_t inodes_per_group;
	uint32_t inode_table_start;
	uint16_t inode_size;
	uint32_t block_size;
	ext2_block_group_ref_t *bg_ref;
	ext2_inode_ref_t *newref;
	
	newref = malloc(sizeof(ext2_inode_ref_t));
	if (newref == NULL) {
		return ENOMEM;
	}
	
	inodes_per_group = ext2_superblock_get_inodes_per_group(fs->superblock);
	
	/* inode numbers are 1-based, but it is simpler to work with 0-based
	 * when computing indices
	 */
	index -= 1;
	block_group = index / inodes_per_group;
	offset_in_group = index % inodes_per_group;
	
	rc = ext2_filesystem_get_block_group_ref(fs, block_group, &bg_ref);
	if (rc != EOK) {
		free(newref);
		return rc;
	}
	
	inode_table_start = ext2_block_group_get_inode_table_first_block(
	    bg_ref->block_group);
	
	rc = ext2_filesystem_put_block_group_ref(bg_ref);
	if (rc != EOK) {
		free(newref);
		return rc;
	}

	inode_size = ext2_superblock_get_inode_size(fs->superblock);
	block_size = ext2_superblock_get_block_size(fs->superblock);
	
	byte_offset_in_group = offset_in_group * inode_size;
	
	block_id = inode_table_start + (byte_offset_in_group / block_size);
	offset_in_block = byte_offset_in_group % block_size;
	
	rc = block_get(&newref->block, fs->device, block_id, 0);
	if (rc != EOK) {
		free(newref);
		return rc;
	}
	
	newref->inode = newref->block->data + offset_in_block;
	/* we decremented index above, but need to store the original value
	 * in the reference
	 */
	newref->index = index+1;
	
	*ref = newref;
	
	return EOK;
}

/**
 * Free a reference to inode
 * 
 * @param ref Pointer to inode reference to free
 * 
 * @return 		EOK on success or negative error code on failure
 */
int ext2_filesystem_put_inode_ref(ext2_inode_ref_t *ref)
{
	int rc;
	
	rc = block_put(ref->block);
	free(ref);
	
	return rc;
}

/**
 * Find a filesystem block number where iblock-th data block
 * of the given inode is located.
 * 
 * @param fblock the number of filesystem block, or 0 if no such block is allocated yet
 * 
 * @return 		EOK on success or negative error code on failure
 */
int ext2_filesystem_get_inode_data_block_index(ext2_filesystem_t *fs, ext2_inode_t* inode,
    aoff64_t iblock, uint32_t* fblock)
{
	int rc;
	aoff64_t limits[4];
	uint32_t block_ids_per_block;
	aoff64_t blocks_per_level[4];
	uint32_t offset_in_block;
	uint32_t current_block;
	aoff64_t block_offset_in_level;
	int i;
	int level;
	block_t *block;
	
	/* Handle simple case when we are dealing with direct reference */ 
	if (iblock < EXT2_INODE_DIRECT_BLOCKS) {
		current_block = ext2_inode_get_direct_block(inode, (uint32_t)iblock);
		*fblock = current_block;
		return EOK;
	}
	
	/* Compute limits for indirect block levels
	 * TODO: compute this once when loading filesystem and store in ext2_filesystem_t
	 */
	block_ids_per_block = ext2_superblock_get_block_size(fs->superblock) / sizeof(uint32_t);
	limits[0] = EXT2_INODE_DIRECT_BLOCKS;
	blocks_per_level[0] = 1;
	for (i = 1; i < 4; i++) {
		blocks_per_level[i]  = blocks_per_level[i-1] *
		    block_ids_per_block;
		limits[i] = limits[i-1] + blocks_per_level[i];
	}
	
	/* Determine the indirection level needed to get the desired block */
	level = -1;
	for (i = 1; i < 4; i++) {
		if (iblock < limits[i]) {
			level = i;
			break;
		}
	}
	
	if (level == -1) {
		return EIO;
	}
	
	/* Compute offsets for the topmost level */
	block_offset_in_level = iblock - limits[level-1];
	current_block = ext2_inode_get_indirect_block(inode, level-1);
	offset_in_block = block_offset_in_level / blocks_per_level[level-1];
	
	/* Navigate through other levels, until we find the block number
	 * or find null reference meaning we are dealing with sparse file
	 */
	while (level > 0) {
		rc = block_get(&block, fs->device, current_block, 0);
		if (rc != EOK) {
			return rc;
		}
		
		assert(offset_in_block < block_ids_per_block);
		current_block = uint32_t_le2host(((uint32_t*)block->data)[offset_in_block]);
		
		rc = block_put(block);
		if (rc != EOK) {
			return rc;
		}
		
		if (current_block == 0) {
			/* This is a sparse file */
			*fblock = 0;
			return EOK;
		}
		
		level -= 1;
		
		/* If we are on the last level, break here as
		 * there is no next level to visit
		 */
		if (level == 0) {
			break;
		}
		
		/* Visit the next level */
		block_offset_in_level %= blocks_per_level[level];
		offset_in_block = block_offset_in_level / blocks_per_level[level-1];
	}
	
	*fblock = current_block;
	
	return EOK;
}

/**
 * Allocate a given number of blocks and store their ids in blocks
 * 
 * @todo TODO: This function is not finished and really has never been
 *             used (and tested) yet
 * 
 * @param fs pointer to filesystem
 * @param blocks array of count uint32_t values where store block ids
 * @param count number of blocks to allocate and elements in blocks array
 * @param preferred_bg preferred block group number
 * 
 * @return 		EOK on success or negative error code on failure
 */
int ext2_filesystem_allocate_blocks(ext2_filesystem_t *fs, uint32_t *blocks,
    size_t count, uint32_t preferred_bg)
{
	uint32_t bg_count = ext2_superblock_get_block_group_count(fs->superblock);
	uint32_t bpg = ext2_superblock_get_blocks_per_group(fs->superblock);
	uint32_t block_size = ext2_superblock_get_block_size(fs->superblock);
	uint32_t block_group = preferred_bg;
	uint32_t free_blocks_sb;
	uint32_t block_groups_left;
	size_t idx;
	ext2_block_group_ref_t *bg;
	int rc;
	uint32_t bb_block;
	block_t *block;
	size_t bb_idx;
	size_t bb_bit;
	
	free_blocks_sb = ext2_superblock_get_free_block_count(fs->superblock);
	
	if (count > free_blocks_sb) {
		return EIO;
	}
	
	block_groups_left = bg_count;
	
	idx = 0;
	
	/* Read the block group descriptor */
	rc = ext2_filesystem_get_block_group_ref(fs, block_group, &bg);
	if (rc != EOK) {
		goto failed;
	}
	
	while (idx < count && block_groups_left > 0) {
		uint16_t fb = ext2_block_group_get_free_block_count(bg->block_group);
		if (fb == 0) {
			block_group = (block_group + 1) % bg_count;
			block_groups_left -= 1;
			
			rc = ext2_filesystem_put_block_group_ref(bg);
			if (rc != EOK) {
				goto failed;
			}
			
			rc = ext2_filesystem_get_block_group_ref(fs, block_group, &bg);
			if (rc != EOK) {
				goto failed;
			}
			continue;
		}
		
		/* We found a block group with free block, let's look at the block bitmap */
		bb_block = ext2_block_group_get_block_bitmap_block(bg->block_group);
		
		rc = block_get(&block, fs->device, bb_block, BLOCK_FLAGS_NONE);
		if (rc != EOK) {
			goto failed;
		}
		
		/* Use all blocks from this block group */
		for (bb_idx = 0; bb_idx < block_size && idx < count; bb_idx++) {
			uint8_t *data = (uint8_t *) block->data;
			if (data[bb_idx] == 0xff) {
				continue;
			}
			/* find an empty bit */
			uint8_t mask;
			for (mask = 1, bb_bit = 0;
				 bb_bit < 8 && idx < count; 
				 bb_bit++, mask = mask << 1) {
				if ((data[bb_idx] & mask) == 0) {
					// free block found
					blocks[idx] = block_group * bpg + bb_idx*8 + bb_bit;
					data[bb_idx] |= mask;
					idx += 1;
					fb -= 1;
					ext2_block_group_set_free_block_count(bg->block_group, fb);
				}
			}
		}
	}
	
	rc = ext2_filesystem_put_block_group_ref(bg);
	if (rc != EOK) {
		goto failed;
	}
	
	// TODO update superblock
	
	return EOK;
failed:
	// TODO deallocate already allocated blocks, if possible
	
	return rc;
}

/**
 * Finalize an instance of filesystem
 * 
 * @param fs Pointer to ext2_filesystem_t to finalize
 */
void ext2_filesystem_fini(ext2_filesystem_t *fs)
{
	free(fs->superblock);
	block_fini(fs->device);
}


/** @}
 */
