/*
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
 * @file	libext4_filesystem.c
 * @brief	More complex filesystem operations.
 */

#include <byteorder.h>
#include <errno.h>
#include <malloc.h>
#include "libext4.h"

int ext4_filesystem_init(ext4_filesystem_t *fs, service_id_t service_id)
{
	int rc;

	fs->device = service_id;

	rc = block_init(EXCHANGE_SERIALIZE, fs->device, 4096);
	if (rc != EOK) {
		return rc;
	}

	/* Read superblock from device */
	ext4_superblock_t *temp_superblock;
	rc = ext4_superblock_read_direct(fs->device, &temp_superblock);
	if (rc != EOK) {
		block_fini(fs->device);
		return rc;
	}

	/* Read block size from superblock and check */
	uint32_t block_size = ext4_superblock_get_block_size(temp_superblock);
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

	uint32_t block_ids_per_block = block_size / sizeof(uint32_t);
	fs->inode_block_limits[0] = EXT4_INODE_DIRECT_BLOCK_COUNT;
	fs->inode_blocks_per_level[0] = 1;
	for (int i = 1; i < 4; i++) {
		fs->inode_blocks_per_level[i] = fs->inode_blocks_per_level[i-1] *
		    block_ids_per_block;
		fs->inode_block_limits[i] = fs->inode_block_limits[i-1] +
				fs->inode_blocks_per_level[i];
	}

	/* Return loaded superblock */
	fs->superblock = temp_superblock;

	return EOK;
}

int ext4_filesystem_fini(ext4_filesystem_t *fs, bool write_sb)
{
	int rc = EOK;

	if (write_sb) {
		rc = ext4_superblock_write_direct(fs->device, fs->superblock);
	}

	free(fs->superblock);
	block_fini(fs->device);

	return rc;
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

int ext4_filesystem_get_block_group_ref(ext4_filesystem_t *fs, uint32_t bgid,
    ext4_block_group_ref_t **ref)
{
	int rc;

	ext4_block_group_ref_t *newref = malloc(sizeof(ext4_block_group_ref_t));
	if (newref == NULL) {
		return ENOMEM;
	}

	uint32_t descriptors_per_block = ext4_superblock_get_block_size(fs->superblock)
	    / ext4_superblock_get_desc_size(fs->superblock);

	/* Block group descriptor table starts at the next block after superblock */
	aoff64_t block_id = ext4_superblock_get_first_data_block(fs->superblock) + 1;

	/* Find the block containing the descriptor we are looking for */
	block_id += bgid / descriptors_per_block;
	uint32_t offset = (bgid % descriptors_per_block) * ext4_superblock_get_desc_size(fs->superblock);

	rc = block_get(&newref->block, fs->device, block_id, 0);
	if (rc != EOK) {
		free(newref);
		return rc;
	}

	newref->block_group = newref->block->data + offset;
	newref->fs = fs;
	newref->index = bgid;
	newref->dirty = false;

	*ref = newref;

	return EOK;
}

static uint16_t ext4_filesystem_bg_checksum(ext4_superblock_t *sb, uint32_t bgid,
                            ext4_block_group_t *bg)
{
	uint16_t crc = 0;

	if (ext4_superblock_has_feature_read_only(sb, EXT4_FEATURE_RO_COMPAT_GDT_CSUM)) {

		void *base = bg;
		void *checksum = &bg->checksum;

		uint32_t offset = (uint32_t)(checksum - base);

		uint32_t le_group = host2uint32_t_le(bgid);

		crc = crc16(~0, sb->uuid, sizeof(sb->uuid));
		crc = crc16(crc, (uint8_t *)&le_group, sizeof(le_group));
		crc = crc16(crc, (uint8_t *)bg, offset);

		offset += sizeof(bg->checksum); /* skip checksum */

		/* for checksum of struct ext4_group_desc do the rest...*/
		if ((ext4_superblock_has_feature_incompatible(sb, EXT4_FEATURE_INCOMPAT_64BIT)) &&
			offset < ext4_superblock_get_desc_size(sb)) {

			crc = crc16(crc, ((uint8_t *)bg) + offset, ext4_superblock_get_desc_size(sb) - offset);
		}
	}

	return crc;

}


int ext4_filesystem_put_block_group_ref(ext4_block_group_ref_t *ref)
{
	int rc;

	if (ref->dirty) {
		uint16_t checksum = ext4_filesystem_bg_checksum(
				ref->fs->superblock, ref->index, ref->block_group);

		ext4_block_group_set_checksum(ref->block_group, checksum);

		ref->block->dirty = true;
	}

	rc = block_put(ref->block);
	free(ref);

	return rc;
}

int ext4_filesystem_get_inode_ref(ext4_filesystem_t *fs, uint32_t index,
    ext4_inode_ref_t **ref)
{
	int rc;

	ext4_inode_ref_t *newref = malloc(sizeof(ext4_inode_ref_t));
	if (newref == NULL) {
		return ENOMEM;
	}

	uint32_t inodes_per_group =
			ext4_superblock_get_inodes_per_group(fs->superblock);

	/* inode numbers are 1-based, but it is simpler to work with 0-based
	 * when computing indices
	 */
	index -= 1;
	uint32_t block_group = index / inodes_per_group;
	uint32_t offset_in_group = index % inodes_per_group;

	ext4_block_group_ref_t *bg_ref;
	rc = ext4_filesystem_get_block_group_ref(fs, block_group, &bg_ref);
	if (rc != EOK) {
		free(newref);
		return rc;
	}

	uint32_t inode_table_start = ext4_block_group_get_inode_table_first_block(
	    bg_ref->block_group, fs->superblock);

	rc = ext4_filesystem_put_block_group_ref(bg_ref);
	if (rc != EOK) {
		free(newref);
		return rc;
	}

	uint16_t inode_size = ext4_superblock_get_inode_size(fs->superblock);
	uint32_t block_size = ext4_superblock_get_block_size(fs->superblock);
	uint32_t byte_offset_in_group = offset_in_group * inode_size;

	aoff64_t block_id = inode_table_start + (byte_offset_in_group / block_size);
	rc = block_get(&newref->block, fs->device, block_id, 0);
	if (rc != EOK) {
		free(newref);
		return rc;
	}

	uint32_t offset_in_block = byte_offset_in_group % block_size;
	newref->inode = newref->block->data + offset_in_block;
	/* we decremented index above, but need to store the original value
	 * in the reference
	 */
	newref->index = index + 1;
	newref->fs = fs;
	newref->dirty = false;

	*ref = newref;

	return EOK;
}


int ext4_filesystem_put_inode_ref(ext4_inode_ref_t *ref)
{
	int rc;

	if (ref->dirty) {
		ref->block->dirty = true;
	}

	rc = block_put(ref->block);
	free(ref);

	return rc;
}

int ext4_filesystem_alloc_inode(ext4_filesystem_t *fs,
		ext4_inode_ref_t **inode_ref, int flags)
{
	int rc;

	bool is_dir = false;
	if (flags & L_DIRECTORY) {
		is_dir = true;
	}

	// allocate inode
	uint32_t index;
	rc = ext4_ialloc_alloc_inode(fs, &index, is_dir);
	if (rc != EOK) {
		return rc;
	}

	rc = ext4_filesystem_get_inode_ref(fs, index, inode_ref);
	if (rc != EOK) {
		ext4_ialloc_free_inode(fs, index, is_dir);
		return rc;
	}

	// init inode
	ext4_inode_t *inode = (*inode_ref)->inode;

	if (is_dir) {
		ext4_inode_set_mode(fs->superblock, inode, EXT4_INODE_MODE_DIRECTORY);
		ext4_inode_set_links_count(inode, 1); // '.' entry
	} else {
		ext4_inode_set_mode(fs->superblock, inode, EXT4_INODE_MODE_FILE);
		ext4_inode_set_links_count(inode, 0);
	}

	ext4_inode_set_uid(inode, 0);
	ext4_inode_set_gid(inode, 0);
	ext4_inode_set_size(inode, 0);
	ext4_inode_set_access_time(inode, 0);
	ext4_inode_set_change_inode_time(inode, 0);
	ext4_inode_set_modification_time(inode, 0);
	ext4_inode_set_deletion_time(inode, 0);
	ext4_inode_set_blocks_count(fs->superblock, inode, 0);
	ext4_inode_set_flags(inode, 0);
	ext4_inode_set_generation(inode, 0);

	// Reset blocks array
	for (uint32_t i = 0; i < EXT4_INODE_BLOCKS; i++) {
		inode->blocks[i] = 0;
	}

	if (ext4_superblock_has_feature_incompatible(
			fs->superblock, EXT4_FEATURE_INCOMPAT_EXTENTS)) {

		ext4_inode_set_flag(inode, EXT4_INODE_FLAG_EXTENTS);

		// Initialize extent root header
		ext4_extent_header_t *header = ext4_inode_get_extent_header(inode);
		ext4_extent_header_set_depth(header, 0);
		ext4_extent_header_set_entries_count(header, 0);
		ext4_extent_header_set_generation(header, 0);
		ext4_extent_header_set_magic(header, EXT4_EXTENT_MAGIC);

		uint16_t max_entries = (EXT4_INODE_BLOCKS * sizeof (uint32_t) - sizeof(ext4_extent_header_t))
				/ sizeof(ext4_extent_t);

		ext4_extent_header_set_max_entries_count(header, max_entries);
	}

	(*inode_ref)->dirty = true;

	return EOK;
}

int ext4_filesystem_free_inode(ext4_inode_ref_t *inode_ref)
{
	int rc;

	ext4_filesystem_t *fs = inode_ref->fs;

	if (ext4_superblock_has_feature_incompatible(
			fs->superblock, EXT4_FEATURE_INCOMPAT_EXTENTS) &&
				ext4_inode_has_flag(inode_ref->inode, EXT4_INODE_FLAG_EXTENTS)) {

		// Data structures are released during truncate operation...
		goto finish;
	}

	// release all indirect (no data) blocks

	// 1) Single indirect
	uint32_t fblock = ext4_inode_get_indirect_block(inode_ref->inode, 0);
	if (fblock != 0) {
		rc = ext4_balloc_free_block(inode_ref, fblock);
		if (rc != EOK) {
			return rc;
		}

		ext4_inode_set_indirect_block(inode_ref->inode, 0, 0);
	}

	block_t *block;
	uint32_t block_size = ext4_superblock_get_block_size(fs->superblock);
	uint32_t count = block_size / sizeof(uint32_t);

	// 2) Double indirect
	fblock = ext4_inode_get_indirect_block(inode_ref->inode, 1);
	if (fblock != 0) {
		rc = block_get(&block, fs->device, fblock, BLOCK_FLAGS_NONE);
		if (rc != EOK) {
			return rc;
		}

		uint32_t ind_block;
		for (uint32_t offset = 0; offset < count; ++offset) {
			ind_block = uint32_t_le2host(((uint32_t*)block->data)[offset]);

			if (ind_block != 0) {
				rc = ext4_balloc_free_block(inode_ref, ind_block);
				if (rc != EOK) {
					block_put(block);
					return rc;
				}
			}
		}

		block_put(block);
		rc = ext4_balloc_free_block(inode_ref, fblock);
		if (rc != EOK) {
			return rc;
		}

		ext4_inode_set_indirect_block(inode_ref->inode, 1, 0);
	}


	// 3) Tripple indirect
	block_t *subblock;
	fblock = ext4_inode_get_indirect_block(inode_ref->inode, 2);
	if (fblock != 0) {
		rc = block_get(&block, fs->device, fblock, BLOCK_FLAGS_NONE);
		if (rc != EOK) {
			return rc;
		}

		uint32_t ind_block;
		for (uint32_t offset = 0; offset < count; ++offset) {
			ind_block = uint32_t_le2host(((uint32_t*)block->data)[offset]);

			if (ind_block != 0) {
				rc = block_get(&subblock, fs->device, ind_block, BLOCK_FLAGS_NONE);
				if (rc != EOK) {
					block_put(block);
					return rc;
				}

				uint32_t ind_subblock;
				for (uint32_t suboffset = 0; suboffset < count; ++suboffset) {
					ind_subblock = uint32_t_le2host(((uint32_t*)subblock->data)[suboffset]);

					if (ind_subblock != 0) {
						rc = ext4_balloc_free_block(inode_ref, ind_subblock);
						if (rc != EOK) {
							block_put(subblock);
							block_put(block);
							return rc;
						}
					}

				}
				block_put(subblock);

			}

			rc = ext4_balloc_free_block(inode_ref, ind_block);
			if (rc != EOK) {
				block_put(block);
				return rc;
			}


		}

		block_put(block);
		rc = ext4_balloc_free_block(inode_ref, fblock);
		if (rc != EOK) {
			return rc;
		}

		ext4_inode_set_indirect_block(inode_ref->inode, 2, 0);
	}

finish:
	inode_ref->dirty = true;

	// Free inode
	if (ext4_inode_is_type(fs->superblock, inode_ref->inode,
			EXT4_INODE_MODE_DIRECTORY)) {
		rc = ext4_ialloc_free_inode(fs, inode_ref->index, true);
	} else {
		rc = ext4_ialloc_free_inode(fs, inode_ref->index, false);
	}
	if (rc != EOK) {
		return rc;
	}

	return EOK;
}

int ext4_filesystem_truncate_inode(
		ext4_inode_ref_t *inode_ref, aoff64_t new_size)
{
	int rc;

	ext4_superblock_t *sb = inode_ref->fs->superblock;

	if (! ext4_inode_can_truncate(sb, inode_ref->inode)) {
		// Unable to truncate
		return EINVAL;
	}

	aoff64_t old_size = ext4_inode_get_size(sb, inode_ref->inode);
	if (old_size == new_size) {
		// Nothing to do
		return EOK;
	}

	// It's not suppported to make the larger file
	if (old_size < new_size) {
		return EINVAL;
	}

	aoff64_t size_diff = old_size - new_size;
	uint32_t block_size  = ext4_superblock_get_block_size(sb);
	uint32_t diff_blocks_count = size_diff / block_size;
	if (size_diff % block_size != 0) {
		diff_blocks_count++;
	}

	uint32_t old_blocks_count = old_size / block_size;
	if (old_size % block_size != 0) {
		old_blocks_count++;
	}

	if (ext4_superblock_has_feature_incompatible(
			inode_ref->fs->superblock, EXT4_FEATURE_INCOMPAT_EXTENTS) &&
				ext4_inode_has_flag(inode_ref->inode, EXT4_INODE_FLAG_EXTENTS)) {

		rc = ext4_extent_release_blocks_from(inode_ref,
				old_blocks_count - diff_blocks_count);
		if (rc != EOK) {
			return rc;
		}
	} else {
		// starting from 1 because of logical blocks are numbered from 0
		for (uint32_t i = 1; i <= diff_blocks_count; ++i) {
			rc = ext4_filesystem_release_inode_block(inode_ref, old_blocks_count - i);
			if (rc != EOK) {
				return rc;
			}
		}
	}

	ext4_inode_set_size(inode_ref->inode, new_size);

	inode_ref->dirty = true;

	return EOK;
}

int ext4_filesystem_get_inode_data_block_index(ext4_inode_ref_t *inode_ref,
		aoff64_t iblock, uint32_t *fblock)
{
	int rc;

	ext4_filesystem_t *fs = inode_ref->fs;

	if (ext4_inode_get_size(fs->superblock, inode_ref->inode) == 0) {
		*fblock = 0;
		return EOK;
	}

	uint32_t current_block;

	/* Handle inode using extents */
	if (ext4_superblock_has_feature_incompatible(fs->superblock, EXT4_FEATURE_INCOMPAT_EXTENTS) &&
			ext4_inode_has_flag(inode_ref->inode, EXT4_INODE_FLAG_EXTENTS)) {
		rc = ext4_extent_find_block(inode_ref, iblock, &current_block);

		if (rc != EOK) {
			return rc;
		}

		*fblock = current_block;
		return EOK;

	}

	ext4_inode_t *inode = inode_ref->inode;

	/* Handle simple case when we are dealing with direct reference */
	if (iblock < EXT4_INODE_DIRECT_BLOCK_COUNT) {
		current_block = ext4_inode_get_direct_block(inode, (uint32_t)iblock);
		*fblock = current_block;
		return EOK;
	}

	/* Determine the indirection level needed to get the desired block */
	int level = -1;
	for (int i = 1; i < 4; i++) {
		if (iblock < fs->inode_block_limits[i]) {
			level = i;
			break;
		}
	}

	if (level == -1) {
		return EIO;
	}

	/* Compute offsets for the topmost level */
	aoff64_t block_offset_in_level = iblock - fs->inode_block_limits[level-1];
	current_block = ext4_inode_get_indirect_block(inode, level-1);
	uint32_t offset_in_block = block_offset_in_level / fs->inode_blocks_per_level[level-1];

	if (current_block == 0) {
		*fblock = 0;
		return EOK;
	}

	block_t *block;

	/* Navigate through other levels, until we find the block number
	 * or find null reference meaning we are dealing with sparse file
	 */
	while (level > 0) {
		rc = block_get(&block, fs->device, current_block, 0);
		if (rc != EOK) {
			return rc;
		}

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
		block_offset_in_level %= fs->inode_blocks_per_level[level];
		offset_in_block = block_offset_in_level / fs->inode_blocks_per_level[level-1];
	}

	*fblock = current_block;

	return EOK;
}


int ext4_filesystem_set_inode_data_block_index(ext4_inode_ref_t *inode_ref,
		aoff64_t iblock, uint32_t fblock)
{
	int rc;

	ext4_filesystem_t *fs = inode_ref->fs;

	/* Handle inode using extents */
	if (ext4_superblock_has_feature_compatible(fs->superblock, EXT4_FEATURE_INCOMPAT_EXTENTS) &&
			ext4_inode_has_flag(inode_ref->inode, EXT4_INODE_FLAG_EXTENTS)) {
		// not reachable !!!
		return ENOTSUP;
	}

	/* Handle simple case when we are dealing with direct reference */
	if (iblock < EXT4_INODE_DIRECT_BLOCK_COUNT) {
		ext4_inode_set_direct_block(inode_ref->inode, (uint32_t)iblock, fblock);
		inode_ref->dirty = true;
		return EOK;
	}

	/* Determine the indirection level needed to get the desired block */
	int level = -1;
	for (int i = 1; i < 4; i++) {
		if (iblock < fs->inode_block_limits[i]) {
			level = i;
			break;
		}
	}

	if (level == -1) {
		return EIO;
	}

	uint32_t block_size = ext4_superblock_get_block_size(fs->superblock);

	/* Compute offsets for the topmost level */
	aoff64_t block_offset_in_level = iblock - fs->inode_block_limits[level-1];
	uint32_t current_block = ext4_inode_get_indirect_block(inode_ref->inode, level-1);
	uint32_t offset_in_block = block_offset_in_level / fs->inode_blocks_per_level[level-1];

	uint32_t new_block_addr;
	block_t *block, *new_block;

	if (current_block == 0) {
		rc = ext4_balloc_alloc_block(inode_ref, &new_block_addr);
		if (rc != EOK) {
			return rc;
		}

		ext4_inode_set_indirect_block(inode_ref->inode, level - 1, new_block_addr);

		inode_ref->dirty = true;

		rc = block_get(&new_block, fs->device, new_block_addr, BLOCK_FLAGS_NOREAD);
		if (rc != EOK) {
			ext4_balloc_free_block(inode_ref, new_block_addr);
			return rc;
		}

		memset(new_block->data, 0, block_size);
		new_block->dirty = true;

		rc = block_put(new_block);
		if (rc != EOK) {
			return rc;
		}

		current_block = new_block_addr;
	}

	/* Navigate through other levels, until we find the block number
	 * or find null reference meaning we are dealing with sparse file
	 */
	while (level > 0) {

		rc = block_get(&block, fs->device, current_block, 0);
		if (rc != EOK) {
			return rc;
		}

		current_block = uint32_t_le2host(((uint32_t*)block->data)[offset_in_block]);

		if ((level > 1) && (current_block == 0)) {
			rc = ext4_balloc_alloc_block(inode_ref, &new_block_addr);
			if (rc != EOK) {
				block_put(block);
				return rc;
			}

			rc = block_get(&new_block, fs->device, new_block_addr, BLOCK_FLAGS_NOREAD);
			if (rc != EOK) {
				block_put(block);
				return rc;
			}

			memset(new_block->data, 0, block_size);
			new_block->dirty = true;

			rc = block_put(new_block);
			if (rc != EOK) {
				block_put(block);
				return rc;
			}

			((uint32_t*)block->data)[offset_in_block] = host2uint32_t_le(new_block_addr);
			block->dirty = true;
			current_block = new_block_addr;
		}

		if (level == 1) {
			((uint32_t*)block->data)[offset_in_block] = host2uint32_t_le(fblock);
			block->dirty = true;
		}

		rc = block_put(block);
		if (rc != EOK) {
			return rc;
		}

		level -= 1;

		/* If we are on the last level, break here as
		 * there is no next level to visit
		 */
		if (level == 0) {
			break;
		}

		/* Visit the next level */
		block_offset_in_level %= fs->inode_blocks_per_level[level];
		offset_in_block = block_offset_in_level / fs->inode_blocks_per_level[level-1];
	}

	return EOK;
}

int ext4_filesystem_release_inode_block(
		ext4_inode_ref_t *inode_ref, uint32_t iblock)
{
	int rc;

	uint32_t fblock;

	ext4_filesystem_t *fs = inode_ref->fs;

	// EXTENTS are handled otherwise
	assert(! (ext4_superblock_has_feature_incompatible(fs->superblock,
			EXT4_FEATURE_INCOMPAT_EXTENTS) &&
			ext4_inode_has_flag(inode_ref->inode, EXT4_INODE_FLAG_EXTENTS)));

	ext4_inode_t *inode = inode_ref->inode;

	/* Handle simple case when we are dealing with direct reference */
	if (iblock < EXT4_INODE_DIRECT_BLOCK_COUNT) {
		fblock = ext4_inode_get_direct_block(inode, iblock);
		// Sparse file
		if (fblock == 0) {
			return EOK;
		}

		ext4_inode_set_direct_block(inode, iblock, 0);
		return ext4_balloc_free_block(inode_ref, fblock);
	}


	/* Determine the indirection level needed to get the desired block */
	int level = -1;
	for (int i = 1; i < 4; i++) {
		if (iblock < fs->inode_block_limits[i]) {
			level = i;
			break;
		}
	}

	if (level == -1) {
		return EIO;
	}

	/* Compute offsets for the topmost level */
	aoff64_t block_offset_in_level = iblock - fs->inode_block_limits[level-1];
	uint32_t current_block = ext4_inode_get_indirect_block(inode, level-1);
	uint32_t offset_in_block = block_offset_in_level / fs->inode_blocks_per_level[level-1];

	/* Navigate through other levels, until we find the block number
	 * or find null reference meaning we are dealing with sparse file
	 */
	block_t *block;
	while (level > 0) {
		rc = block_get(&block, fs->device, current_block, 0);
		if (rc != EOK) {
			return rc;
		}

		current_block = uint32_t_le2host(((uint32_t*)block->data)[offset_in_block]);

		// Set zero
		if (level == 1) {
			((uint32_t*)block->data)[offset_in_block] = host2uint32_t_le(0);
			block->dirty = true;
		}

		rc = block_put(block);
		if (rc != EOK) {
			return rc;
		}

		level -= 1;

		/* If we are on the last level, break here as
		 * there is no next level to visit
		 */
		if (level == 0) {
			break;
		}

		/* Visit the next level */
		block_offset_in_level %= fs->inode_blocks_per_level[level];
		offset_in_block = block_offset_in_level / fs->inode_blocks_per_level[level-1];
	}

	fblock = current_block;

	if (fblock == 0) {
		return EOK;
	}

	return ext4_balloc_free_block(inode_ref, fblock);

}

int ext4_filesystem_append_inode_block(ext4_inode_ref_t *inode_ref,
		uint32_t *fblock, uint32_t *iblock)
{
	int rc;

	EXT4FS_DBG("");

	// Handle extents separately
	if (ext4_superblock_has_feature_incompatible(
			inode_ref->fs->superblock, EXT4_FEATURE_INCOMPAT_EXTENTS) &&
			ext4_inode_has_flag(inode_ref->inode, EXT4_INODE_FLAG_EXTENTS)) {

		return ext4_extent_append_block(inode_ref, iblock, fblock);

	}

	ext4_superblock_t *sb = inode_ref->fs->superblock;

	// Compute next block index and allocate data block
	uint64_t inode_size = ext4_inode_get_size(sb, inode_ref->inode);
	uint32_t block_size = ext4_superblock_get_block_size(sb);

	// TODO zarovnat inode size a ne assert!!!
	assert(inode_size % block_size == 0);

	// Logical blocks are numbered from 0
	uint32_t new_block_idx = inode_size / block_size;

	uint32_t phys_block;
	rc =  ext4_balloc_alloc_block(inode_ref, &phys_block);
	if (rc != EOK) {
		return rc;
	}

	rc = ext4_filesystem_set_inode_data_block_index(inode_ref, new_block_idx, phys_block);
	if (rc != EOK) {
		ext4_balloc_free_block(inode_ref, phys_block);
		return rc;
	}

	ext4_inode_set_size(inode_ref->inode, inode_size + block_size);

	inode_ref->dirty = true;

	*fblock = phys_block;
	*iblock = new_block_idx;
	return EOK;
}

int ext4_filesystem_add_orphan(ext4_inode_ref_t *inode_ref)
{
	uint32_t next_orphan = ext4_superblock_get_last_orphan(
			inode_ref->fs->superblock);
	ext4_inode_set_deletion_time(inode_ref->inode, next_orphan);
	ext4_superblock_set_last_orphan(
			inode_ref->fs->superblock, inode_ref->index);
	inode_ref->dirty = true;

	return EOK;
}

int ext4_filesystem_delete_orphan(ext4_inode_ref_t *inode_ref)
{
	int rc;

	uint32_t last_orphan = ext4_superblock_get_last_orphan(
			inode_ref->fs->superblock);
	assert(last_orphan > 0);

	uint32_t next_orphan = ext4_inode_get_deletion_time(inode_ref->inode);

	if (last_orphan == inode_ref->index) {
		ext4_superblock_set_last_orphan(inode_ref->fs->superblock, next_orphan);
		ext4_inode_set_deletion_time(inode_ref->inode, 0);
		inode_ref->dirty = true;
		return EOK;
	}

	ext4_inode_ref_t *current;
	rc = ext4_filesystem_get_inode_ref(inode_ref->fs, last_orphan, &current);
	if (rc != EOK) {
		return rc;
	}
	next_orphan = ext4_inode_get_deletion_time(current->inode);

	bool found;

	while (next_orphan != 0) {
		if (next_orphan == inode_ref->index) {
			next_orphan = ext4_inode_get_deletion_time(inode_ref->inode);
			ext4_inode_set_deletion_time(current->inode, next_orphan);
			current->dirty = true;
			found = true;
			break;
		}

		ext4_filesystem_put_inode_ref(current);

		rc = ext4_filesystem_get_inode_ref(inode_ref->fs, next_orphan, &current);
		if (rc != EOK) {
			return rc;
		}
		next_orphan = ext4_inode_get_deletion_time(current->inode);

	}

	ext4_inode_set_deletion_time(inode_ref->inode, 0);

	rc = ext4_filesystem_put_inode_ref(current);
	if (rc != EOK) {
		return rc;
	}

	if (!found) {
		return ENOENT;
	}

	return EOK;
}

/**
 * @}
 */ 
