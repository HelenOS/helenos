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
#include "libext2_directory.h"
#include <byteorder.h>
#include <errno.h>
#include <assert.h>

/**
 * Get inode number for the directory entry
 * 
 * @param de pointer to linked list directory entry
 */
uint32_t ext2_directory_entry_ll_get_inode(ext2_directory_entry_ll_t *de)
{
	return uint32_t_le2host(de->inode);
}

/**
 * Get length of the directory entry
 * 
 * @param de pointer to linked list directory entry
 */
uint16_t ext2_directory_entry_ll_get_entry_length(
    ext2_directory_entry_ll_t *de)
{
	return uint16_t_le2host(de->entry_length);
}

/**
 * Get length of the name stored in the directory entry
 * 
 * @param de pointer to linked list directory entry
 */
uint16_t ext2_directory_entry_ll_get_name_length(
    ext2_superblock_t *sb, ext2_directory_entry_ll_t *de)
{
	if (ext2_superblock_get_rev_major(sb) == 0 &&
	    ext2_superblock_get_rev_minor(sb) < 5) {
		return ((uint16_t)de->name_length_high) << 8 | 
		    ((uint16_t)de->name_length);
	}
	return de->name_length;
}

/**
 * Initialize a directory iterator
 * 
 * @param it pointer to iterator to initialize
 * @param fs pointer to filesystem structure
 * @param inode pointer to inode reference structure
 * @return EOK on success or negative error code on failure
 */
int ext2_directory_iterator_init(ext2_directory_iterator_t *it,
    ext2_filesystem_t *fs, ext2_inode_ref_t *inode_ref)
{
	int rc;
	uint32_t block_id;
	it->inode_ref = inode_ref;
	it->fs = fs;
	
	// Get the first data block, so we can get first entry
	rc = ext2_filesystem_get_inode_data_block_index(fs, inode_ref->inode, 0, 
	    &block_id);
	if (rc != EOK) {
		return rc;
	}
	
	rc = block_get(&it->current_block, fs->device, block_id, 0);
	if (rc != EOK) {
		return rc;
	}
	
	it->current = it->current_block->data;
	it->current_offset = 0;
	
	return EOK;
}

/**
 * Advance the directory iterator to the next entry
 * 
 * @param it pointer to iterator to initialize
 * @return EOK on success or negative error code on failure
 */
int ext2_directory_iterator_next(ext2_directory_iterator_t *it)
{
	int rc;
	uint16_t skip;
	uint64_t size;
	aoff64_t current_block_idx;
	aoff64_t next_block_idx;
	uint32_t next_block_phys_idx;
	uint32_t block_size;
	uint32_t offset_in_block;
	
	assert(it->current != NULL);
	
	skip = ext2_directory_entry_ll_get_entry_length(it->current);
	size = ext2_inode_get_size(it->fs->superblock, it->inode_ref->inode);
	
	// Are we at the end?
	if (it->current_offset + skip >= size) {
		rc = block_put(it->current_block);
		it->current_block = NULL;
		it->current = NULL;
		if (rc != EOK) {
			return rc;
		}
		
		it->current_offset += skip;
		return EOK;
	}
	
	block_size = ext2_superblock_get_block_size(it->fs->superblock);
	current_block_idx = it->current_offset / block_size;
	next_block_idx = (it->current_offset + skip) / block_size;
	
	// If we are moving accross block boundary,
	// we need to get another block
	if (current_block_idx != next_block_idx) {
		rc = block_put(it->current_block);
		it->current_block = NULL;
		it->current = NULL;
		if (rc != EOK) {
			return rc;
		}
		
		rc = ext2_filesystem_get_inode_data_block_index(it->fs,
		    it->inode_ref->inode, next_block_idx, &next_block_phys_idx);
		if (rc != EOK) {
			return rc;
		}
		
		rc = block_get(&it->current_block, it->fs->device, next_block_phys_idx,
		    BLOCK_FLAGS_NONE);
		if (rc != EOK) {
			it->current_block = NULL;
			return rc;
		}
	}
	
	offset_in_block = (it->current_offset + skip) % block_size;
	
	// Ensure proper alignment
	if ((offset_in_block % 4) != 0) {
		it->current = NULL;
		return EIO;
	}
	
	// Ensure that the core of the entry does not overflow the block
	if (offset_in_block > block_size - 8) {
		it->current = NULL;
		return EIO;
	}
		
	it->current = it->current_block->data + offset_in_block;
	it->current_offset += skip;
	
	// Ensure that the whole entry does not overflow the block
	skip = ext2_directory_entry_ll_get_entry_length(it->current);
	if (offset_in_block + skip > block_size) {
		it->current = NULL;
		return EIO;
	}
	
	// Ensure the name length is not too large
	if (ext2_directory_entry_ll_get_name_length(it->fs->superblock, 
	    it->current) > skip-8) {
		it->current = NULL;
		return EIO;
	}
	
	return EOK;
}

/**
 * Release all resources asociated with the directory iterator
 * 
 * @param it pointer to iterator to initialize
 * @return EOK on success or negative error code on failure
 */
int ext2_directory_iterator_fini(ext2_directory_iterator_t *it)
{
	int rc;
	
	it->fs = NULL;
	it->inode_ref = NULL;
	it->current = NULL;
	
	if (it->current_block) {
		rc = block_put(it->current_block);
		if (rc != EOK) {
			return rc;
		}
	}
	
	return EOK;
}

/** @}
 */
