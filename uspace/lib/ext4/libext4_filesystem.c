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
 * @file	libext4_filesystem.c
 * @brief	TODO
 */

#include <byteorder.h>
#include <errno.h>
#include <malloc.h>
#include "libext4.h"

int ext4_filesystem_init(ext4_filesystem_t *fs, service_id_t service_id)
{

	int rc;
	ext4_superblock_t *temp_superblock;
	size_t block_size;

	fs->device = service_id;

	// TODO what does constant 2048 mean?
	rc = block_init(EXCHANGE_SERIALIZE, fs->device, 2048);
	if (rc != EOK) {
		return rc;
	}

	/* Read superblock from device */
	rc = ext4_superblock_read_direct(fs->device, &temp_superblock);
	if (rc != EOK) {
		block_fini(fs->device);
		return rc;
	}

	/* Read block size from superblock and check */
	block_size = ext4_superblock_get_block_size(temp_superblock);
	if (block_size > EXT4_MAX_BLOCK_SIZE) {
		block_fini(fs->device);
		return ENOTSUP;
	}

	/* Initialize block caching */
	rc = block_cache_init(service_id, block_size, 0, CACHE_MODE_WT);
	if (rc != EOK) {
		block_fini(fs->device);
		return rc;
	}

	/* Return loaded superblock */
	fs->superblock = temp_superblock;

	return EOK;
}

void ext4_filesystem_fini(ext4_filesystem_t *fs)
{
	free(fs->superblock);
	block_fini(fs->device);
}

int ext4_filesystem_check_sanity(ext4_filesystem_t *fs)
{
	int rc;

	rc = ext4_superblock_check_sanity(fs->superblock);
	if (rc != EOK) {
		return rc;
	}

	return EOK;
}

int ext4_filesystem_check_features(ext4_filesystem_t *fs, bool *o_read_only)
{
	/* Feature flags are present in rev 1 and later */
	if (ext4_superblock_get_rev_level(fs->superblock) == 0) {
		*o_read_only = false;
		return EOK;
	}

	uint32_t incompatible_features;
	incompatible_features = ext4_superblock_get_features_incompatible(fs->superblock);
	incompatible_features &= ~EXT4_FEATURE_INCOMPAT_SUPP;
	if (incompatible_features > 0) {
		*o_read_only = true;
		return ENOTSUP;
	}

	uint32_t compatible_read_only;
	compatible_read_only = ext4_superblock_get_features_read_only(fs->superblock);
	compatible_read_only &= ~EXT4_FEATURE_RO_COMPAT_SUPP;
	if (compatible_read_only > 0) {
		*o_read_only = true;
	}

	return EOK;
}

// Feature checkers
bool ext4_filesystem_has_feature_compatible(ext4_filesystem_t *fs, uint32_t feature)
{
	ext4_superblock_t *sb = fs->superblock;

	if (ext4_superblock_get_features_compatible(sb) & feature) {
		return true;
	}
	return false;
}

bool ext4_filesystem_has_feature_incompatible(ext4_filesystem_t *fs, uint32_t feature)
{
	ext4_superblock_t *sb = fs->superblock;

	if (ext4_superblock_get_features_incompatible(sb) & feature) {
		return true;
	}
	return false;
}

bool ext4_filesystem_has_feature_read_only(ext4_filesystem_t *fs, uint32_t feature)
{
	ext4_superblock_t *sb = fs->superblock;

	if (ext4_superblock_get_features_read_only(sb) & feature) {
		return true;
	}
	return false;
}


int ext4_filesystem_get_block_group_ref(ext4_filesystem_t *fs, uint32_t bgid,
    ext4_block_group_ref_t **ref)
{
	int rc;
	aoff64_t block_id;
	uint32_t descriptors_per_block;
	size_t offset;
	ext4_block_group_ref_t *newref;

	newref = malloc(sizeof(ext4_block_group_ref_t));
	if (newref == NULL) {
		return ENOMEM;
	}

	descriptors_per_block = ext4_superblock_get_block_size(fs->superblock)
	    / EXT4_BLOCK_GROUP_DESCRIPTOR_SIZE;

	/* Block group descriptor table starts at the next block after superblock */
	block_id = ext4_superblock_get_first_data_block(fs->superblock) + 1;

	/* Find the block containing the descriptor we are looking for */
	block_id += bgid / descriptors_per_block;
	offset = (bgid % descriptors_per_block) * EXT4_BLOCK_GROUP_DESCRIPTOR_SIZE;

	rc = block_get(&newref->block, fs->device, block_id, 0);
	if (rc != EOK) {
		free(newref);
		return rc;
	}

	newref->block_group = newref->block->data + offset;

	*ref = newref;

	return EOK;
}

int ext4_filesystem_put_block_group_ref(ext4_block_group_ref_t *ref)
{
	int rc;

	rc = block_put(ref->block);
	free(ref);

	return rc;
}

int ext4_filesystem_get_inode_ref(ext4_filesystem_t *fs, uint32_t index,
    ext4_inode_ref_t **ref)
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
	ext4_block_group_ref_t *bg_ref;
	ext4_inode_ref_t *newref;

	newref = malloc(sizeof(ext4_inode_ref_t));
	if (newref == NULL) {
		return ENOMEM;
	}

	inodes_per_group = ext4_superblock_get_inodes_per_group(fs->superblock);

	/* inode numbers are 1-based, but it is simpler to work with 0-based
	 * when computing indices
	 */
	index -= 1;
	block_group = index / inodes_per_group;
	offset_in_group = index % inodes_per_group;

	rc = ext4_filesystem_get_block_group_ref(fs, block_group, &bg_ref);
	if (rc != EOK) {
		free(newref);
		return rc;
	}

	inode_table_start = ext4_block_group_get_inode_table_first_block(
	    bg_ref->block_group);

	rc = ext4_filesystem_put_block_group_ref(bg_ref);
	if (rc != EOK) {
		free(newref);
		return rc;
	}

	inode_size = ext4_superblock_get_inode_size(fs->superblock);
	block_size = ext4_superblock_get_block_size(fs->superblock);

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


int ext4_filesystem_put_inode_ref(ext4_inode_ref_t *ref)
{
	int rc;

	rc = block_put(ref->block);
	free(ref);

	return rc;
}

int ext4_filesystem_get_inode_data_block_index(ext4_filesystem_t *fs, ext4_inode_t* inode,
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

	/* Handle inode using extents */
	// TODO check "extents" feature in superblock ???
	if (ext4_inode_has_flag(inode, EXT4_INODE_FLAG_EXTENTS)) {
		current_block = ext4_inode_get_extent_block(inode, iblock);
		*fblock = current_block;
		return EOK;

	}

	/* Handle simple case when we are dealing with direct reference */
	if (iblock < EXT4_INODE_DIRECT_BLOCK_COUNT) {
		current_block = ext4_inode_get_direct_block(inode, (uint32_t)iblock);
		*fblock = current_block;
		return EOK;
	}

	/* Compute limits for indirect block levels
	 * TODO: compute this once when loading filesystem and store in ext2_filesystem_t
	 */
	block_ids_per_block = ext4_superblock_get_block_size(fs->superblock) / sizeof(uint32_t);
	limits[0] = EXT4_INODE_DIRECT_BLOCK_COUNT;
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
	current_block = ext4_inode_get_indirect_block(inode, level-1);
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
 * @}
 */ 
