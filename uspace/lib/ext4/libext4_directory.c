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
 * @file	libext4_directory.c
 * @brief	Ext4 directory structure operations.
 */

#include <byteorder.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include "libext4.h"

static int ext4_directory_iterator_set(ext4_directory_iterator_t *,
    uint32_t);


uint32_t ext4_directory_entry_ll_get_inode(ext4_directory_entry_ll_t *de)
{
	return uint32_t_le2host(de->inode);
}

void ext4_directory_entry_ll_set_inode(ext4_directory_entry_ll_t *de,
		uint32_t inode)
{
	de->inode = host2uint32_t_le(inode);
}

uint16_t ext4_directory_entry_ll_get_entry_length(
		ext4_directory_entry_ll_t *de)
{
	return uint16_t_le2host(de->entry_length);
}

void ext4_directory_entry_ll_set_entry_length(ext4_directory_entry_ll_t *de,
		uint16_t length)
{
	de->entry_length = host2uint16_t_le(length);
}

uint16_t ext4_directory_entry_ll_get_name_length(
    ext4_superblock_t *sb, ext4_directory_entry_ll_t *de)
{
	if (ext4_superblock_get_rev_level(sb) == 0 &&
	    ext4_superblock_get_minor_rev_level(sb) < 5) {

		return ((uint16_t)de->name_length_high) << 8 |
			    ((uint16_t)de->name_length);

	}
	return de->name_length;

}

void ext4_directory_entry_ll_set_name_length(ext4_superblock_t *sb,
		ext4_directory_entry_ll_t *de, uint16_t length)
{
	de->name_length = (length << 8) >> 8;

	if (ext4_superblock_get_rev_level(sb) == 0 &&
		    ext4_superblock_get_minor_rev_level(sb) < 5) {

		de->name_length_high = length >> 8;
	}
}

uint8_t ext4_directory_entry_ll_get_inode_type(
		ext4_superblock_t *sb, ext4_directory_entry_ll_t *de)
{
	if (ext4_superblock_get_rev_level(sb) > 0 ||
		    ext4_superblock_get_minor_rev_level(sb) >= 5) {

			return de->inode_type;
	}

	return EXT4_DIRECTORY_FILETYPE_UNKNOWN;

}

void ext4_directory_entry_ll_set_inode_type(
		ext4_superblock_t *sb, ext4_directory_entry_ll_t *de, uint8_t type)
{
	if (ext4_superblock_get_rev_level(sb) > 0 ||
			ext4_superblock_get_minor_rev_level(sb) >= 5) {

		de->inode_type = type;
	}

	// else do nothing

}

int ext4_directory_iterator_init(ext4_directory_iterator_t *it,
    ext4_filesystem_t *fs, ext4_inode_ref_t *inode_ref, aoff64_t pos)
{
	it->inode_ref = inode_ref;
	it->fs = fs;
	it->current = NULL;
	it->current_offset = 0;
	it->current_block = NULL;

	return ext4_directory_iterator_seek(it, pos);
}


int ext4_directory_iterator_next(ext4_directory_iterator_t *it)
{
	uint16_t skip;

	assert(it->current != NULL);

	skip = ext4_directory_entry_ll_get_entry_length(it->current);

	return ext4_directory_iterator_seek(it, it->current_offset + skip);
}


int ext4_directory_iterator_seek(ext4_directory_iterator_t *it, aoff64_t pos)
{
	int rc;

	uint64_t size = ext4_inode_get_size(it->fs->superblock, it->inode_ref->inode);

	/* The iterator is not valid until we seek to the desired position */
	it->current = NULL;

	/* Are we at the end? */
	if (pos >= size) {
		if (it->current_block) {
			rc = block_put(it->current_block);
			it->current_block = NULL;
			if (rc != EOK) {
				return rc;
			}
		}

		it->current_offset = pos;
		return EOK;
	}

	uint32_t block_size = ext4_superblock_get_block_size(it->fs->superblock);
	aoff64_t current_block_idx = it->current_offset / block_size;
	aoff64_t next_block_idx = pos / block_size;

	/* If we don't have a block or are moving accross block boundary,
	 * we need to get another block
	 */
	if (it->current_block == NULL || current_block_idx != next_block_idx) {
		if (it->current_block) {
			rc = block_put(it->current_block);
			it->current_block = NULL;
			if (rc != EOK) {
				return rc;
			}
		}

		uint32_t next_block_phys_idx;
		rc = ext4_filesystem_get_inode_data_block_index(it->fs,
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

	it->current_offset = pos;

	return ext4_directory_iterator_set(it, block_size);
}

static int ext4_directory_iterator_set(ext4_directory_iterator_t *it,
    uint32_t block_size)
{

	it->current = NULL;

	uint32_t offset_in_block = it->current_offset % block_size;

	/* Ensure proper alignment */
	if ((offset_in_block % 4) != 0) {
		return EIO;
	}

	/* Ensure that the core of the entry does not overflow the block */
	if (offset_in_block > block_size - 8) {
		return EIO;
	}

	ext4_directory_entry_ll_t *entry = it->current_block->data + offset_in_block;

	/* Ensure that the whole entry does not overflow the block */
	uint16_t length = ext4_directory_entry_ll_get_entry_length(entry);
	if (offset_in_block + length > block_size) {
		return EIO;
	}

	/* Ensure the name length is not too large */
	if (ext4_directory_entry_ll_get_name_length(it->fs->superblock,
	    entry) > length-8) {
		return EIO;
	}

	it->current = entry;
	return EOK;
}


int ext4_directory_iterator_fini(ext4_directory_iterator_t *it)
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

int ext4_directory_append_block(ext4_filesystem_t *fs,
		ext4_inode_ref_t *inode_ref, uint32_t *fblock, uint32_t *iblock)
{
	int rc;

	// Compute next block index and allocate data block
	uint64_t inode_size = ext4_inode_get_size(fs->superblock, inode_ref->inode);
	uint32_t block_size = ext4_superblock_get_block_size(fs->superblock);
	uint32_t new_block_idx = inode_size / block_size;

	uint32_t phys_block;
	rc =  ext4_balloc_alloc_block(fs, inode_ref, &phys_block);
	if (rc != EOK) {
		return rc;
	}

	rc = ext4_filesystem_set_inode_data_block_index(fs, inode_ref, new_block_idx, phys_block);
	if (rc != EOK) {
		ext4_balloc_free_block(fs, inode_ref, phys_block);
		return rc;
	}

	inode_size += block_size;
	ext4_inode_set_size(inode_ref->inode, inode_size);

	inode_ref->dirty = true;

	*fblock = phys_block;
	*iblock = new_block_idx;
	return EOK;
}

void ext4_directory_write_entry(ext4_superblock_t *sb,
		ext4_directory_entry_ll_t *entry, uint16_t entry_len,
		ext4_inode_ref_t *child, const char *name, size_t name_len)
{
	ext4_directory_entry_ll_set_inode(entry, child->index);
	ext4_directory_entry_ll_set_entry_length(entry, entry_len);
	ext4_directory_entry_ll_set_name_length(sb, entry, name_len);

	if (ext4_inode_is_type(sb, child->inode, EXT4_INODE_MODE_DIRECTORY)) {
		ext4_directory_entry_ll_set_inode_type(
				sb, entry, EXT4_DIRECTORY_FILETYPE_DIR);
	} else {
		ext4_directory_entry_ll_set_inode_type(
				sb, entry, EXT4_DIRECTORY_FILETYPE_REG_FILE);
	}
	memcpy(entry->name, name, name_len);
}

int ext4_directory_add_entry(ext4_filesystem_t *fs, ext4_inode_ref_t * parent,
		const char *entry_name, ext4_inode_ref_t *child)
{
	int rc;

	EXT4FS_DBG("adding entry to directory \%u [ino = \%u, name = \%s]", parent->index, child->index, entry_name);

	uint16_t name_len = strlen(entry_name);

	// Index adding (if allowed)
	if (ext4_superblock_has_feature_compatible(fs->superblock, EXT4_FEATURE_COMPAT_DIR_INDEX) &&
			ext4_inode_has_flag(parent->inode, EXT4_INODE_FLAG_INDEX)) {

		rc = ext4_directory_dx_add_entry(fs, parent, child, name_len, entry_name);

		// Check if index is not corrupted
		if (rc != EXT4_ERR_BAD_DX_DIR) {

			if (rc != EOK) {
				return rc;
			}

			return EOK;
		}

		// Needed to clear dir index flag
		ext4_inode_clear_flag(parent->inode, EXT4_INODE_FLAG_INDEX);
		parent->dirty = true;

		EXT4FS_DBG("index is corrupted - doing linear algorithm, index flag cleared");
	}

	// Linear algorithm

	ext4_directory_iterator_t it;
	rc = ext4_directory_iterator_init(&it, fs, parent, 0);
	if (rc != EOK) {
		return rc;
	}

	uint32_t block_size = ext4_superblock_get_block_size(fs->superblock);
	uint16_t required_len = 8 + name_len + (4 - name_len % 4);

	while (it.current != NULL) {
		uint32_t entry_inode = ext4_directory_entry_ll_get_inode(it.current);
		uint16_t rec_len = ext4_directory_entry_ll_get_entry_length(it.current);

		if ((entry_inode == 0) && (rec_len >= required_len)) {

			ext4_directory_write_entry(fs->superblock, it.current, rec_len,
					child, entry_name, name_len);
			it.current_block->dirty = true;
			return ext4_directory_iterator_fini(&it);
		}

		if (entry_inode != 0) {
			uint16_t used_name_len = ext4_directory_entry_ll_get_name_length(
					fs->superblock, it.current);

			uint16_t used_space = 8 + used_name_len;
			if ((used_name_len % 4) != 0) {
				used_space += 4 - (used_name_len % 4);
			}
			uint16_t free_space = rec_len - used_space;

			EXT4FS_DBG("rec_len = \%u, used_space = \%u, free space = \%u", rec_len, used_space, free_space);

			if (free_space >= required_len) {

				// Cut tail of current entry
				ext4_directory_entry_ll_set_entry_length(it.current, used_space);

				// SEEK manually
				uint32_t local_offset = (it.current_offset % block_size);
				local_offset += used_space;
				ext4_directory_entry_ll_t *new_entry = it.current_block->data + local_offset;

				// We are sure, that both entries are in the same data block
				// dirtyness will be set now

				ext4_directory_write_entry(fs->superblock, new_entry,
						free_space, child, entry_name, name_len);
				it.current_block->dirty = true;
				return ext4_directory_iterator_fini(&it);
			}

		}

		rc = ext4_directory_iterator_next(&it);
		if (rc != EOK) {
			return rc;
		}
	}

	// Destroy iterator
	ext4_directory_iterator_fini(&it);

	EXT4FS_DBG("NO FREE SPACE - needed to allocate block");

	uint32_t fblock;
	uint32_t iblock;
	rc = ext4_directory_append_block(fs, parent, &fblock, &iblock);
	if (rc != EOK) {
		return rc;
	}

	// Load block
	block_t *new_block;
	rc = block_get(&new_block, fs->device, fblock, BLOCK_FLAGS_NOREAD);
	if (rc != EOK) {
		return rc;
	}

	// Fill block with zeroes
	memset(new_block->data, 0, block_size);

	ext4_directory_entry_ll_t *block_entry = new_block->data;

	ext4_directory_write_entry(fs->superblock, block_entry, block_size, 
	child, entry_name, name_len);

	new_block->dirty = true;
	rc = block_put(new_block);
	if (rc != EOK) {
		return rc;
	}

	return EOK;
}

int ext4_directory_find_entry(ext4_directory_iterator_t *it,
		ext4_inode_ref_t *parent, const char *name)
{
	int rc;
	uint32_t name_size = strlen(name);

	// Index search
	if (ext4_superblock_has_feature_compatible(it->fs->superblock, EXT4_FEATURE_COMPAT_DIR_INDEX) &&
			ext4_inode_has_flag(parent->inode, EXT4_INODE_FLAG_INDEX)) {

		rc = ext4_directory_dx_find_entry(it, it->fs, parent, name_size, name);

		// Check if index is not corrupted
		if (rc != EXT4_ERR_BAD_DX_DIR) {

			if (rc != EOK) {
				return rc;
			}
			return EOK;
		}

		EXT4FS_DBG("index is corrupted - doing linear search");

	}

	bool found = false;
	// Linear search
	while (it->current != NULL) {
		uint32_t inode = ext4_directory_entry_ll_get_inode(it->current);

		/* ignore empty directory entries */
		if (inode != 0) {
			uint16_t entry_name_size = ext4_directory_entry_ll_get_name_length(
					it->fs->superblock, it->current);

			if (entry_name_size == name_size && bcmp(name, it->current->name,
				    name_size) == 0) {
				found = true;
				break;
			}
		}

		rc = ext4_directory_iterator_next(it);
		if (rc != EOK) {
			return rc;
		}
	}

	if (!found) {
		return ENOENT;
	}

	return EOK;
}


int ext4_directory_remove_entry(ext4_filesystem_t* fs,
		ext4_inode_ref_t *parent, const char *name)
{
	int rc;

	if (!ext4_inode_is_type(fs->superblock, parent->inode,
	    EXT4_INODE_MODE_DIRECTORY)) {
		return ENOTDIR;
	}

	ext4_directory_iterator_t it;
	rc = ext4_directory_iterator_init(&it, fs, parent, 0);
	if (rc != EOK) {
		return rc;
	}

	rc = ext4_directory_find_entry(&it, parent, name);
	if (rc != EOK) {
		ext4_directory_iterator_fini(&it);
		return rc;
	}

	uint32_t block_size = ext4_superblock_get_block_size(fs->superblock);
	uint32_t pos = it.current_offset % block_size;

	ext4_directory_entry_ll_set_inode(it.current, 0);

	if (pos != 0) {
		uint32_t offset = 0;

		ext4_directory_entry_ll_t *tmp_dentry = it.current_block->data;
		uint16_t tmp_dentry_length =
				ext4_directory_entry_ll_get_entry_length(tmp_dentry);

		while ((offset + tmp_dentry_length) < pos) {
			offset += ext4_directory_entry_ll_get_entry_length(tmp_dentry);
			tmp_dentry = it.current_block->data + offset;
			tmp_dentry_length =
					ext4_directory_entry_ll_get_entry_length(tmp_dentry);
		}

		assert(tmp_dentry_length + offset == pos);

		uint16_t del_entry_length =
				ext4_directory_entry_ll_get_entry_length(it.current);
		ext4_directory_entry_ll_set_entry_length(tmp_dentry,
				tmp_dentry_length + del_entry_length);

	}


	it.current_block->dirty = true;

	ext4_directory_iterator_fini(&it);
	return EOK;
}


/**
 * @}
 */ 
