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
 * @brief	TODO
 */

#include <byteorder.h>
#include <errno.h>
#include "libext4.h"

static int ext4_directory_iterator_set(ext4_directory_iterator_t *,
    uint32_t);


uint32_t ext4_directory_entry_ll_get_inode(ext4_directory_entry_ll_t *de)
{
	return uint32_t_le2host(de->inode);
}

uint16_t ext4_directory_entry_ll_get_entry_length(
    ext4_directory_entry_ll_t *de)
{
	return uint16_t_le2host(de->entry_length);
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

uint8_t ext4_directory_dx_root_info_get_hash_version(ext4_directory_dx_root_info_t *root_info)
{
	return root_info->hash_version;
}

uint8_t ext4_directory_dx_root_info_get_info_length(ext4_directory_dx_root_info_t *root_info)
{
	return root_info->info_length;
}

uint8_t ext4_directory_dx_root_info_get_indirect_levels(ext4_directory_dx_root_info_t *root_info)
{
	return root_info->indirect_levels;
}

uint16_t ext4_directory_dx_countlimit_get_limit(ext4_directory_dx_countlimit_t *countlimit)
{
	return uint16_t_le2host(countlimit->limit);
}
uint16_t ext4_directory_dx_countlimit_get_count(ext4_directory_dx_countlimit_t *countlimit)
{
	return uint16_t_le2host(countlimit->count);
}

uint32_t ext4_directory_dx_entry_get_hash(ext4_directory_dx_entry_t *entry)
{
	return uint32_t_le2host(entry->hash);
}

uint32_t ext4_directory_dx_entry_get_block(ext4_directory_dx_entry_t *entry)
{
	return uint32_t_le2host(entry->block);
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

	uint64_t size;
	aoff64_t current_block_idx;
	aoff64_t next_block_idx;
	uint32_t next_block_phys_idx;
	uint32_t block_size;

	size = ext4_inode_get_size(it->fs->superblock, it->inode_ref->inode);

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

	block_size = ext4_superblock_get_block_size(it->fs->superblock);
	current_block_idx = it->current_offset / block_size;
	next_block_idx = pos / block_size;

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
	uint32_t offset_in_block = it->current_offset % block_size;

	it->current = NULL;

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


static int ext4_directory_hinfo_init(ext4_hash_info_t *hinfo, block_t *root_block,
		ext4_superblock_t *sb, size_t name_len, const char *name)
{
	uint32_t block_size, entry_space;
	uint16_t limit;
	ext4_directory_dx_root_t *root;

	root = (ext4_directory_dx_root_t *)root_block->data;

	if (root->info.hash_version != EXT4_HASH_VERSION_TEA &&
			root->info.hash_version != EXT4_HASH_VERSION_HALF_MD4 &&
			root->info.hash_version != EXT4_HASH_VERSION_LEGACY) {
		return EXT4_ERR_BAD_DX_DIR;
	}

	// Check unused flags
	if (root->info.unused_flags != 0) {
		EXT4FS_DBG("ERR: unused_flags = \%u", root->info.unused_flags);
		return EXT4_ERR_BAD_DX_DIR;
	}

	// Check indirect levels
	if (root->info.indirect_levels > 1) {
		EXT4FS_DBG("ERR: indirect_levels = \%u", root->info.indirect_levels);
		return EXT4_ERR_BAD_DX_DIR;
	}

	block_size = ext4_superblock_get_block_size(sb);

	entry_space = block_size;
	entry_space -= 2 * sizeof(ext4_directory_dx_dot_entry_t);
	entry_space -= sizeof(ext4_directory_dx_root_info_t);
    entry_space = entry_space / sizeof(ext4_directory_dx_entry_t);

    limit = ext4_directory_dx_countlimit_get_limit((ext4_directory_dx_countlimit_t *)&root->entries);
    if (limit != entry_space) {
    	return EXT4_ERR_BAD_DX_DIR;
	}

	hinfo->hash_version = ext4_directory_dx_root_info_get_hash_version(&root->info);
	if ((hinfo->hash_version <= EXT4_HASH_VERSION_TEA)
			&& (ext4_superblock_has_flag(sb, EXT4_SUPERBLOCK_FLAGS_UNSIGNED_HASH))) {
		// 3 is magic from ext4 linux implementation
		hinfo->hash_version += 3;
	}

	hinfo->seed = ext4_superblock_get_hash_seed(sb);

	if (name) {
		ext4_hash_string(hinfo, name_len, name);
	}

	return EOK;
}

static int ext4_directory_dx_get_leaf(ext4_hash_info_t *hinfo,
		ext4_filesystem_t *fs, ext4_inode_t *inode, block_t *root_block,
		ext4_directory_dx_handle_t **handle, ext4_directory_dx_handle_t *handles)
{
	int rc;
	uint16_t count, limit, entry_space;
	uint8_t indirect_level;
	ext4_directory_dx_root_t *root;
	ext4_directory_dx_entry_t *p, *q, *m, *at;
	ext4_directory_dx_entry_t *entries;
	block_t *tmp_block = root_block;
	uint32_t fblock, next_block;
	ext4_directory_dx_handle_t *tmp_handle = handles;

	root = (ext4_directory_dx_root_t *)root_block->data;
	entries = (ext4_directory_dx_entry_t *)&root->entries;

	limit = ext4_directory_dx_countlimit_get_limit((ext4_directory_dx_countlimit_t *)entries);
	indirect_level = ext4_directory_dx_root_info_get_indirect_levels(&root->info);

	while (true) {

		count = ext4_directory_dx_countlimit_get_count((ext4_directory_dx_countlimit_t *)entries);
		if ((count == 0) || (count > limit)) {
			return EXT4_ERR_BAD_DX_DIR;
		}

		p = entries + 1;
		q = entries + count - 1;

		while (p <= q) {
			m = p + (q - p) / 2;
			if (ext4_directory_dx_entry_get_hash(m) > hinfo->hash) {
				q = m - 1;
			} else {
				p = m + 1;
			}
		}

		at = p - 1;

		tmp_handle->block = tmp_block;
		tmp_handle->entries = entries;
		tmp_handle->position = at;

        if (indirect_level == 0) {
        	*handle = tmp_handle;
        	return EOK;
        }

		next_block = ext4_directory_dx_entry_get_block(at);

        indirect_level--;

        rc = ext4_filesystem_get_inode_data_block_index(fs, inode, next_block, &fblock);
        if (rc != EOK) {
        	return rc;
        }

        rc = block_get(&tmp_block, fs->device, fblock, BLOCK_FLAGS_NONE);
        if (rc != EOK) {
        	return rc;
        }

		entries = ((ext4_directory_dx_node_t *) tmp_block->data)->entries;
		limit = ext4_directory_dx_countlimit_get_limit((ext4_directory_dx_countlimit_t *)entries);

        entry_space = ext4_superblock_get_block_size(fs->superblock) - sizeof(ext4_directory_dx_dot_entry_t);
        entry_space = entry_space / sizeof(ext4_directory_dx_entry_t);


		if (limit != entry_space) {
			block_put(tmp_block);
        	return EXT4_ERR_BAD_DX_DIR;
		}

		++tmp_handle;
	}

	// Unreachable
	return EOK;
}


static int ext4_dirextory_dx_find_dir_entry(block_t *block,
		ext4_superblock_t *sb, size_t name_len, const char *name,
		ext4_directory_entry_ll_t **res_entry, aoff64_t *block_offset)
{
	ext4_directory_entry_ll_t *dentry;
	uint16_t dentry_len;
	uint8_t *addr_limit;
	aoff64_t offset = 0;

	dentry = (ext4_directory_entry_ll_t *)block->data;
	addr_limit = block->data + ext4_superblock_get_block_size(sb);

	while ((uint8_t *)dentry < addr_limit) {

		if ((uint8_t*) dentry + name_len > addr_limit) {
			break;
		}

		if (dentry->inode != 0) {
			if (name_len == ext4_directory_entry_ll_get_name_length(sb, dentry)) {
				// Compare names
				if (bcmp((uint8_t *)name, dentry->name, name_len) == 0) {
					*block_offset = offset;
					*res_entry = dentry;
					return 1;
				}
			}
		}


		// Goto next entry
		dentry_len = ext4_directory_entry_ll_get_entry_length(dentry);

        if (dentry_len == 0) {
        	// TODO error
        	return -1;
        }

		offset += dentry_len;
		dentry = (ext4_directory_entry_ll_t *)((uint8_t *)dentry + dentry_len);
	}

	return 0;
}

static int ext4_directory_dx_next_block(ext4_filesystem_t *fs, ext4_inode_t *inode, uint32_t hash,
		ext4_directory_dx_handle_t *handle, ext4_directory_dx_handle_t *handles)
{
	ext4_directory_dx_handle_t *p;
	uint16_t count;
	uint32_t num_handles;
	uint32_t current_hash;
	block_t *block;
	uint32_t block_addr, block_idx;
    int rc;

    num_handles = 0;
    p = handle;

    while (1) {

    	p->position++;
    	count = ext4_directory_dx_countlimit_get_count((ext4_directory_dx_countlimit_t *)p->entries);

    	if (p->position < p->entries + count) {
    		break;
    	}

    	if (p == handles) {
    		return 0;
    	}

    	num_handles++;
    	p--;
    }

    current_hash = ext4_directory_dx_entry_get_hash(p->position);

    if ((hash & 1) == 0) {
    	if ((current_hash & ~1) != hash) {
    		return 0;
    	}
    }

    while (num_handles--) {

    	block_idx = ext4_directory_dx_entry_get_block(p->position);
    	rc = ext4_filesystem_get_inode_data_block_index(fs, inode, block_idx, &block_addr);
    	if (rc != EOK) {
    		return rc;
    	}

    	rc = block_get(&block, fs->device, block_addr, BLOCK_FLAGS_NONE);
    	if (rc != EOK) {
    		return rc;
    	}

    	p++;

    	block_put(p->block);
        p->block = block;
        p->entries = ((ext4_directory_dx_node_t *) block->data)->entries;
        p->position = p->entries;
    }

    return 1;

}

int ext4_directory_dx_find_entry(ext4_directory_iterator_t *it,
		ext4_filesystem_t *fs, ext4_inode_ref_t *inode_ref, size_t len, const char *name)
{
	int rc;
	uint32_t root_block_addr, leaf_block_addr, leaf_block_idx;
	aoff64_t block_offset;
	block_t *root_block, *leaf_block;
	ext4_hash_info_t hinfo;
	ext4_directory_entry_ll_t *res_dentry;
	// TODO better names
	ext4_directory_dx_handle_t handles[2], *handle;

	// get direct block 0 (index root)
	rc = ext4_filesystem_get_inode_data_block_index(fs, inode_ref->inode, 0, &root_block_addr);
	if (rc != EOK) {
		return rc;
	}

	rc = block_get(&root_block, fs->device, root_block_addr, BLOCK_FLAGS_NONE);
	if (rc != EOK) {
		it->current_block = NULL;
		return rc;
	}

	rc = ext4_directory_hinfo_init(&hinfo, root_block, fs->superblock, len, name);
	if (rc != EOK) {
		block_put(root_block);
		return EXT4_ERR_BAD_DX_DIR;
	}

	rc = ext4_directory_dx_get_leaf(&hinfo, fs, inode_ref->inode, root_block, &handle, handles);
	if (rc != EOK) {
		block_put(root_block);
		return EXT4_ERR_BAD_DX_DIR;
	}

	do {

		leaf_block_idx = ext4_directory_dx_entry_get_block(handle->position);

    	rc = ext4_filesystem_get_inode_data_block_index(fs, inode_ref->inode, leaf_block_idx, &leaf_block_addr);
    	if (rc != EOK) {
    		return EXT4_ERR_BAD_DX_DIR;
    	}

		rc = block_get(&leaf_block, fs->device, leaf_block_addr, BLOCK_FLAGS_NONE);
		if (rc != EOK) {
			return EXT4_ERR_BAD_DX_DIR;
		}

		rc = ext4_dirextory_dx_find_dir_entry(leaf_block, fs->superblock, len, name,
				&res_dentry, &block_offset);

		// Found => return it
		if (rc == 1) {
			it->fs = fs;
			it->inode_ref = inode_ref;
			it->current_block = leaf_block;
			it->current_offset = block_offset;
			it->current = res_dentry;
			return EOK;
		}

		block_put(leaf_block);

		// ERROR - corrupted index
		if (rc == -1) {
			// TODO cleanup
			return EXT4_ERR_BAD_DX_DIR;
		}

		rc = ext4_directory_dx_next_block(fs, inode_ref->inode, hinfo.hash, handle, &handles[0]);
		if (rc < 0) {
			// TODO cleanup
			return EXT4_ERR_BAD_DX_DIR;
		}

	} while (rc == 1);

	return ENOENT;
}


/**
 * @}
 */ 
