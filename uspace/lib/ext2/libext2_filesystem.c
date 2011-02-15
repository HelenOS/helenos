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
#include <errno.h>
#include <libblock.h>
#include <malloc.h>

/**
 * Initialize an instance of filesystem on the device.
 * This function reads superblock from the device and
 * initializes libblock cache with appropriate logical block size.
 * 
 * @param fs			Pointer to ext2_filesystem_t to initialize
 * @param devmap_handle	Device handle of the block device
 * 
 * @return 		EOK on success or negative error code on failure
 */
int ext2_filesystem_init(ext2_filesystem_t *fs, devmap_handle_t devmap_handle)
{
	int rc;
	ext2_superblock_t *temp_superblock;
	size_t block_size;
	
	fs->device = devmap_handle;
	
	rc = block_init(fs->device, 2048);
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
	
	rc = block_cache_init(devmap_handle, block_size, 0, CACHE_MODE_WT);
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
	
	// Block group descriptor table starts at the next block after superblock
	block_id = ext2_superblock_get_first_block(fs->superblock) + 1;
	
	// Find the block containing the descriptor we are looking for
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
