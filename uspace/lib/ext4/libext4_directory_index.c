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
 * @file	libext4_directory_index.c
 * @brief	Ext4 directory index operations.
 */

#include <byteorder.h>
#include <errno.h>
#include <malloc.h>
#include <sort.h>
#include <string.h>
#include "libext4.h"

typedef struct ext4_dx_sort_entry {
	uint32_t hash;
	uint32_t rec_len;
	void *dentry;
} ext4_dx_sort_entry_t;


uint8_t ext4_directory_dx_root_info_get_hash_version(
		ext4_directory_dx_root_info_t *root_info)
{
	return root_info->hash_version;
}

void ext4_directory_dx_root_info_set_hash_version(
		ext4_directory_dx_root_info_t *root_info, uint8_t version)
{
	root_info->hash_version = version;
}

uint8_t ext4_directory_dx_root_info_get_info_length(
		ext4_directory_dx_root_info_t *root_info)
{
	return root_info->info_length;
}

void ext4_directory_dx_root_info_set_info_length(
		ext4_directory_dx_root_info_t *root_info, uint8_t info_length)
{
	root_info->info_length = info_length;
}

uint8_t ext4_directory_dx_root_info_get_indirect_levels(
		ext4_directory_dx_root_info_t *root_info)
{
	return root_info->indirect_levels;
}

void ext4_directory_dx_root_info_set_indirect_levels(
		ext4_directory_dx_root_info_t *root_info, uint8_t levels)
{
	root_info->indirect_levels = levels;
}

uint16_t ext4_directory_dx_countlimit_get_limit(
		ext4_directory_dx_countlimit_t *countlimit)
{
	return uint16_t_le2host(countlimit->limit);
}

void ext4_directory_dx_countlimit_set_limit(
		ext4_directory_dx_countlimit_t *countlimit, uint16_t limit)
{
	countlimit->limit = host2uint16_t_le(limit);
}

uint16_t ext4_directory_dx_countlimit_get_count(
		ext4_directory_dx_countlimit_t *countlimit)
{
	return uint16_t_le2host(countlimit->count);
}

void ext4_directory_dx_countlimit_set_count(
		ext4_directory_dx_countlimit_t *countlimit, uint16_t count)
{
	countlimit->count = host2uint16_t_le(count);
}

uint32_t ext4_directory_dx_entry_get_hash(ext4_directory_dx_entry_t *entry)
{
	return uint32_t_le2host(entry->hash);
}

void ext4_directory_dx_entry_set_hash(ext4_directory_dx_entry_t *entry,
		uint32_t hash)
{
	entry->hash = host2uint32_t_le(hash);
}

uint32_t ext4_directory_dx_entry_get_block(ext4_directory_dx_entry_t *entry)
{
	return uint32_t_le2host(entry->block);
}

void ext4_directory_dx_entry_set_block(ext4_directory_dx_entry_t *entry,
		uint32_t block)
{
	entry->block = host2uint32_t_le(block);
}


/**************************************************************************/

int ext4_directory_dx_init(ext4_inode_ref_t *dir)
{
	int rc;

	uint32_t fblock;
	rc = ext4_filesystem_get_inode_data_block_index(dir, 0, &fblock);
	if (rc != EOK) {
		return rc;
	}

	block_t *block;
	rc = block_get(&block, dir->fs->device, fblock, BLOCK_FLAGS_NONE);
	if (rc != EOK) {
		return rc;
	}

	EXT4FS_DBG("fblock = \%u", fblock);

	ext4_directory_dx_root_t *root = block->data;

	ext4_directory_entry_ll_t *dot = (ext4_directory_entry_ll_t *)&root->dots[0];
	ext4_directory_entry_ll_t *dot_dot = (ext4_directory_entry_ll_t *)&root->dots[1];

	EXT4FS_DBG("dot len = \%u, dotdot len = \%u", ext4_directory_entry_ll_get_entry_length(dot), ext4_directory_entry_ll_get_entry_length(dot_dot));

//	uint32_t block_size = ext4_superblock_get_block_size(dir->fs->superblock);
//	uint16_t len = block_size - sizeof(ext4_directory_dx_dot_entry_t);

//	ext4_directory_entry_ll_set_entry_length(dot_dot, len);

	ext4_directory_dx_root_info_t *info = &(root->info);

	uint8_t hash_version =
			ext4_superblock_get_default_hash_version(dir->fs->superblock);

	ext4_directory_dx_root_info_set_hash_version(info, hash_version);
	ext4_directory_dx_root_info_set_indirect_levels(info, 0);
	ext4_directory_dx_root_info_set_info_length(info, 8);

	ext4_directory_dx_countlimit_t *countlimit =
			(ext4_directory_dx_countlimit_t *)&root->entries;
	ext4_directory_dx_countlimit_set_count(countlimit, 1);

	uint32_t block_size = ext4_superblock_get_block_size(dir->fs->superblock);
	uint32_t entry_space = block_size - 2 * sizeof(ext4_directory_dx_dot_entry_t)
		- sizeof(ext4_directory_dx_root_info_t);
	uint16_t root_limit = entry_space / sizeof(ext4_directory_dx_entry_t);
	ext4_directory_dx_countlimit_set_limit(countlimit, root_limit);

	uint32_t iblock;
	rc = ext4_filesystem_append_inode_block(dir, &fblock, &iblock);
	if (rc != EOK) {
		block_put(block);
		return rc;
	}

	block_t *new_block;
	rc = block_get(&new_block, dir->fs->device, fblock, BLOCK_FLAGS_NOREAD);
	if (rc != EOK) {
		block_put(block);
		return rc;
	}

	ext4_directory_entry_ll_t *block_entry = new_block->data;
	ext4_directory_entry_ll_set_entry_length(block_entry, block_size);
	ext4_directory_entry_ll_set_inode(block_entry, 0);

	new_block->dirty = true;
	rc = block_put(new_block);
	if (rc != EOK) {
		block_put(block);
		return rc;
	}

	ext4_directory_dx_entry_t *entry = root->entries;
	ext4_directory_dx_entry_set_block(entry, iblock);

	block->dirty = true;

	rc = block_put(block);
	if (rc != EOK) {
		return rc;
	}

	return EOK;
}

static int ext4_directory_hinfo_init(ext4_hash_info_t *hinfo, block_t *root_block,
		ext4_superblock_t *sb, size_t name_len, const char *name)
{

	ext4_directory_dx_root_t *root = (ext4_directory_dx_root_t *)root_block->data;

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

	uint32_t block_size = ext4_superblock_get_block_size(sb);

	uint32_t entry_space = block_size;
	entry_space -= 2 * sizeof(ext4_directory_dx_dot_entry_t);
	entry_space -= sizeof(ext4_directory_dx_root_info_t);
    entry_space = entry_space / sizeof(ext4_directory_dx_entry_t);

    uint16_t limit = ext4_directory_dx_countlimit_get_limit((ext4_directory_dx_countlimit_t *)&root->entries);
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
		ext4_inode_ref_t *inode_ref, block_t *root_block,
		ext4_directory_dx_block_t **dx_block, ext4_directory_dx_block_t *dx_blocks)
{
	int rc;

	ext4_directory_dx_block_t *tmp_dx_block = dx_blocks;

	ext4_directory_dx_root_t *root = (ext4_directory_dx_root_t *)root_block->data;
	ext4_directory_dx_entry_t *entries = (ext4_directory_dx_entry_t *)&root->entries;

	uint16_t limit = ext4_directory_dx_countlimit_get_limit((ext4_directory_dx_countlimit_t *)entries);
	uint8_t indirect_level = ext4_directory_dx_root_info_get_indirect_levels(&root->info);

	block_t *tmp_block = root_block;
	ext4_directory_dx_entry_t *p, *q, *m, *at;
	while (true) {

		uint16_t count = ext4_directory_dx_countlimit_get_count((ext4_directory_dx_countlimit_t *)entries);
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

		tmp_dx_block->block = tmp_block;
		tmp_dx_block->entries = entries;
		tmp_dx_block->position = at;

        if (indirect_level == 0) {
        	*dx_block = tmp_dx_block;
        	return EOK;
        }

		uint32_t next_block = ext4_directory_dx_entry_get_block(at);

        indirect_level--;

        uint32_t fblock;
        rc = ext4_filesystem_get_inode_data_block_index(
        		inode_ref, next_block, &fblock);
        if (rc != EOK) {
        	return rc;
        }

        rc = block_get(&tmp_block, inode_ref->fs->device, fblock, BLOCK_FLAGS_NONE);
        if (rc != EOK) {
        	return rc;
        }

		entries = ((ext4_directory_dx_node_t *) tmp_block->data)->entries;
		limit = ext4_directory_dx_countlimit_get_limit(
				(ext4_directory_dx_countlimit_t *)entries);

        uint16_t entry_space = ext4_superblock_get_block_size(inode_ref->fs->superblock)
        		- sizeof(ext4_directory_dx_dot_entry_t);
        entry_space = entry_space / sizeof(ext4_directory_dx_entry_t);


		if (limit != entry_space) {
			block_put(tmp_block);
        	return EXT4_ERR_BAD_DX_DIR;
		}

		++tmp_dx_block;
	}

	// Unreachable
	return EOK;
}


static int ext4_directory_dx_next_block(ext4_inode_ref_t *inode_ref, uint32_t hash,
		ext4_directory_dx_block_t *handle, ext4_directory_dx_block_t *handles)
{
	int rc;


    uint32_t num_handles = 0;
    ext4_directory_dx_block_t *p = handle;

    while (1) {

    	p->position++;
    	uint16_t count = ext4_directory_dx_countlimit_get_count((ext4_directory_dx_countlimit_t *)p->entries);

    	if (p->position < p->entries + count) {
    		break;
    	}

    	if (p == handles) {
    		return 0;
    	}

    	num_handles++;
    	p--;
    }

    uint32_t current_hash = ext4_directory_dx_entry_get_hash(p->position);

    if ((hash & 1) == 0) {
    	if ((current_hash & ~1) != hash) {
    		return 0;
    	}
    }


    while (num_handles--) {

    	uint32_t block_idx = ext4_directory_dx_entry_get_block(p->position);
    	uint32_t block_addr;
    	rc = ext4_filesystem_get_inode_data_block_index(inode_ref, block_idx, &block_addr);
    	if (rc != EOK) {
    		return rc;
    	}

    	block_t *block;
    	rc = block_get(&block, inode_ref->fs->device, block_addr, BLOCK_FLAGS_NONE);
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

int ext4_directory_dx_find_entry(ext4_directory_search_result_t *result,
		ext4_inode_ref_t *inode_ref, size_t name_len, const char *name)
{
	int rc;

	// get direct block 0 (index root)
	uint32_t root_block_addr;
	rc = ext4_filesystem_get_inode_data_block_index(inode_ref, 0, &root_block_addr);
	if (rc != EOK) {
		return rc;
	}

	ext4_filesystem_t *fs = inode_ref->fs;

	block_t *root_block;
	rc = block_get(&root_block, fs->device, root_block_addr, BLOCK_FLAGS_NONE);
	if (rc != EOK) {
		return rc;
	}

	ext4_hash_info_t hinfo;
	rc = ext4_directory_hinfo_init(&hinfo, root_block, fs->superblock, name_len, name);
	if (rc != EOK) {
		block_put(root_block);
		return EXT4_ERR_BAD_DX_DIR;
	}

	// Hardcoded number 2 means maximum height of index tree !!!
	ext4_directory_dx_block_t dx_blocks[2];
	ext4_directory_dx_block_t *dx_block, *tmp;
	rc = ext4_directory_dx_get_leaf(&hinfo, inode_ref, root_block, &dx_block, dx_blocks);
	if (rc != EOK) {
		block_put(root_block);
		return EXT4_ERR_BAD_DX_DIR;
	}


	do {

		uint32_t leaf_block_idx = ext4_directory_dx_entry_get_block(dx_block->position);
		uint32_t leaf_block_addr;
    	rc = ext4_filesystem_get_inode_data_block_index(inode_ref, leaf_block_idx, &leaf_block_addr);
    	if (rc != EOK) {
    		goto cleanup;
    	}

    	block_t *leaf_block;
		rc = block_get(&leaf_block, fs->device, leaf_block_addr, BLOCK_FLAGS_NONE);
		if (rc != EOK) {
			goto cleanup;
		}

		ext4_directory_entry_ll_t *res_dentry;
		rc = ext4_directory_find_in_block(leaf_block, fs->superblock, name_len, name, &res_dentry);

		// Found => return it
		if (rc == EOK) {
			result->block = leaf_block;
			result->dentry = res_dentry;
			goto cleanup;
		}

		// Not found, leave untouched
		block_put(leaf_block);

		if (rc != ENOENT) {
			goto cleanup;
		}

		rc = ext4_directory_dx_next_block(inode_ref, hinfo.hash, dx_block, &dx_blocks[0]);
		if (rc < 0) {
			goto cleanup;
		}

	} while (rc == 1);

	rc = ENOENT;

cleanup:

	tmp = dx_blocks;
	while (tmp <= dx_block) {
		block_put(tmp->block);
		++tmp;
	}
	return rc;
}

static int ext4_directory_dx_entry_comparator(void *arg1, void *arg2, void *dummy)
{
	ext4_dx_sort_entry_t *entry1 = arg1;
	ext4_dx_sort_entry_t *entry2 = arg2;

	if (entry1->hash == entry2->hash) {
		return 0;
	}

	if (entry1->hash < entry2->hash) {
		return -1;
	} else {
		return 1;
	}

}

static void ext4_directory_dx_insert_entry(
		ext4_directory_dx_block_t *index_block, uint32_t hash, uint32_t iblock)
{
	ext4_directory_dx_entry_t *old_index_entry = index_block->position;
	ext4_directory_dx_entry_t *new_index_entry = old_index_entry + 1;

	ext4_directory_dx_countlimit_t *countlimit =
			(ext4_directory_dx_countlimit_t *)index_block->entries;
	uint32_t count = ext4_directory_dx_countlimit_get_count(countlimit);

	ext4_directory_dx_entry_t *start_index = index_block->entries;
	size_t bytes = (void *)(start_index + count) - (void *)(new_index_entry);

	memmove(new_index_entry + 1, new_index_entry, bytes);

	ext4_directory_dx_entry_set_block(new_index_entry, iblock);
	ext4_directory_dx_entry_set_hash(new_index_entry, hash);

	ext4_directory_dx_countlimit_set_count(countlimit, count + 1);

	index_block->block->dirty = true;
}

static int ext4_directory_dx_split_data(ext4_filesystem_t *fs,
		ext4_inode_ref_t *inode_ref, ext4_hash_info_t *hinfo,
		block_t *old_data_block, ext4_directory_dx_block_t *index_block, block_t **new_data_block)
{
	int rc = EOK;

	// Allocate buffer for directory entries
	uint32_t block_size = ext4_superblock_get_block_size(fs->superblock);
	void *entry_buffer = malloc(block_size);
	if (entry_buffer == NULL) {
		return ENOMEM;
	}

	// dot entry has the smallest size available
	uint32_t max_entry_count =  block_size / sizeof(ext4_directory_dx_dot_entry_t);

	// Allocate sort entry
	ext4_dx_sort_entry_t *sort_array = malloc(max_entry_count * sizeof(ext4_dx_sort_entry_t));
	if (sort_array == NULL) {
		free(entry_buffer);
		return ENOMEM;
	}

	uint32_t idx = 0;
	uint32_t real_size = 0;

	// Initialize hinfo
	ext4_hash_info_t tmp_hinfo;
	memcpy(&tmp_hinfo, hinfo, sizeof(ext4_hash_info_t));

	// Load all valid entries to the buffer
	ext4_directory_entry_ll_t *dentry = old_data_block->data;
	void *entry_buffer_ptr = entry_buffer;
	while ((void *)dentry < old_data_block->data + block_size) {

		// Read only valid entries
		if (ext4_directory_entry_ll_get_inode(dentry) != 0) {

			uint8_t len = ext4_directory_entry_ll_get_name_length(fs->superblock, dentry);
			ext4_hash_string(&tmp_hinfo, len, (char *)dentry->name);

			uint32_t rec_len = 8 + len;

			if ((rec_len % 4) != 0) {
				rec_len += 4 - (rec_len % 4);
			}

			memcpy(entry_buffer_ptr, dentry, rec_len);

			sort_array[idx].dentry = entry_buffer_ptr;
			sort_array[idx].rec_len = rec_len;
			sort_array[idx].hash = tmp_hinfo.hash;

			entry_buffer_ptr += rec_len;
			real_size += rec_len;
			idx++;
		}

		dentry = (void *)dentry + ext4_directory_entry_ll_get_entry_length(dentry);
	}

	qsort(sort_array, idx, sizeof(ext4_dx_sort_entry_t),
			ext4_directory_dx_entry_comparator, NULL);

	uint32_t new_fblock;
	uint32_t new_iblock;
	rc = ext4_filesystem_append_inode_block(inode_ref, &new_fblock, &new_iblock);
	if (rc != EOK) {
		free(sort_array);
		free(entry_buffer);
		return rc;
	}

	// Load new block
	block_t *new_data_block_tmp;
	rc = block_get(&new_data_block_tmp, fs->device, new_fblock, BLOCK_FLAGS_NOREAD);
	if (rc != EOK) {
		free(sort_array);
		free(entry_buffer);
		return rc;
	}

	// Distribute entries to splitted blocks (by size)
	uint32_t new_hash = 0;
	uint32_t current_size = 0;
	uint32_t mid = 0;
	for (uint32_t i = 0; i < idx; ++i) {
		if ((current_size + sort_array[i].rec_len) > (real_size / 2)) {
			new_hash = sort_array[i].hash;
			mid = i;
			break;
		}

		current_size += sort_array[i].rec_len;
	}

	uint32_t continued = 0;
	if (new_hash == sort_array[mid-1].hash) {
		continued = 1;
	}

	uint32_t offset = 0;
	void *ptr;

	// First part - to the old block
	for (uint32_t i = 0; i < mid; ++i) {
		ptr = old_data_block->data + offset;
		memcpy(ptr, sort_array[i].dentry, sort_array[i].rec_len);

		ext4_directory_entry_ll_t *tmp = ptr;
		if (i < (mid - 1)) {
			ext4_directory_entry_ll_set_entry_length(tmp, sort_array[i].rec_len);
		} else {
			ext4_directory_entry_ll_set_entry_length(tmp, block_size - offset);
		}

		offset += sort_array[i].rec_len;
	}

	// Second part - to the new block
	offset = 0;
	for (uint32_t i = mid; i < idx; ++i) {
		ptr = new_data_block_tmp->data + offset;
		memcpy(ptr, sort_array[i].dentry, sort_array[i].rec_len);

		ext4_directory_entry_ll_t *tmp = ptr;
		if (i < (idx - 1)) {
			ext4_directory_entry_ll_set_entry_length(tmp, sort_array[i].rec_len);
		} else {
			ext4_directory_entry_ll_set_entry_length(tmp, block_size - offset);
		}

		offset += sort_array[i].rec_len;
	}

	old_data_block->dirty = true;
	new_data_block_tmp->dirty = true;

	free(sort_array);
	free(entry_buffer);

	ext4_directory_dx_insert_entry(index_block, new_hash + continued, new_iblock);

	*new_data_block = new_data_block_tmp;

	return EOK;
}


static int ext4_directory_dx_split_index(ext4_filesystem_t *fs,
		ext4_inode_ref_t *inode_ref, ext4_directory_dx_block_t *dx_blocks,
		ext4_directory_dx_block_t *dx_block)
{
	int rc;

	ext4_directory_dx_entry_t *entries;
	if (dx_block == dx_blocks) {
		entries = ((ext4_directory_dx_root_t *) dx_block->block->data)->entries;
	} else {
		entries = ((ext4_directory_dx_node_t *) dx_block->block->data)->entries;
	}

	ext4_directory_dx_countlimit_t *countlimit =
			(ext4_directory_dx_countlimit_t *)entries;
	uint16_t leaf_limit = ext4_directory_dx_countlimit_get_limit(countlimit);
	uint16_t leaf_count = ext4_directory_dx_countlimit_get_count(countlimit);

	// Check if is necessary to split index block
	if (leaf_limit == leaf_count) {
		EXT4FS_DBG("need to split index block !!!");

		unsigned int levels = dx_block - dx_blocks;

		ext4_directory_dx_entry_t *root_entries =
					((ext4_directory_dx_root_t *)dx_blocks[0].block->data)->entries;

		ext4_directory_dx_countlimit_t *root_countlimit =
				(ext4_directory_dx_countlimit_t *)root_entries;
		uint16_t root_limit =
				ext4_directory_dx_countlimit_get_limit(root_countlimit);
		uint16_t root_count =
				ext4_directory_dx_countlimit_get_count(root_countlimit);

		if ((levels > 0) && (root_limit == root_count)) {
			EXT4FS_DBG("Directory index is full");
			return ENOSPC;
		}

		uint32_t new_fblock;
		uint32_t new_iblock;
		rc =  ext4_filesystem_append_inode_block(
				inode_ref, &new_fblock, &new_iblock);
		if (rc != EOK) {
			return rc;
		}

		// New block allocated
		block_t * new_block;
		rc = block_get(&new_block, fs->device, new_fblock, BLOCK_FLAGS_NOREAD);
		if (rc != EOK) {
			return rc;
		}

		ext4_directory_dx_node_t *new_node = new_block->data;
		ext4_directory_dx_entry_t *new_entries = new_node->entries;

		uint32_t block_size = ext4_superblock_get_block_size(fs->superblock);

		if (levels > 0) {
			EXT4FS_DBG("split index leaf node");
			uint32_t count_left = leaf_count / 2;
			uint32_t count_right = leaf_count - count_left;
			uint32_t hash_right =
					ext4_directory_dx_entry_get_hash(entries + count_left);

			memcpy((void *) new_entries, (void *) (entries + count_left),
					count_right * sizeof(ext4_directory_dx_entry_t));

			ext4_directory_dx_countlimit_t *left_countlimit =
					(ext4_directory_dx_countlimit_t *)entries;
			ext4_directory_dx_countlimit_t *right_countlimit =
					(ext4_directory_dx_countlimit_t *)new_entries;

			ext4_directory_dx_countlimit_set_count(left_countlimit, count_left);
			ext4_directory_dx_countlimit_set_count(right_countlimit, count_right);

			uint32_t entry_space = block_size - sizeof(ext4_fake_directory_entry_t);
			uint32_t node_limit = entry_space / sizeof(ext4_directory_dx_entry_t);
			ext4_directory_dx_countlimit_set_limit(right_countlimit, node_limit);

			// Which index block is target for new entry
			uint32_t position_index = (dx_block->position - dx_block->entries);
			if (position_index >= count_left) {

				dx_block->block->dirty = true;

				block_t *block_tmp = dx_block->block;
				dx_block->block = new_block;
				dx_block->position = new_entries + position_index - count_left;
				dx_block->entries = new_entries;

				new_block = block_tmp;

			}

			ext4_directory_dx_insert_entry(dx_blocks, hash_right, new_iblock);

			return block_put(new_block);

		} else {
			EXT4FS_DBG("create second level");

			memcpy((void *) new_entries, (void *) entries,
					leaf_count * sizeof(ext4_directory_dx_entry_t));

			ext4_directory_dx_countlimit_t *new_countlimit =
					(ext4_directory_dx_countlimit_t *)new_entries;

			uint32_t entry_space = block_size - sizeof(ext4_fake_directory_entry_t);
			uint32_t node_limit = entry_space / sizeof(ext4_directory_dx_entry_t);
			ext4_directory_dx_countlimit_set_limit(new_countlimit, node_limit);

			// Set values in root node
			ext4_directory_dx_countlimit_t *new_root_countlimit =
					(ext4_directory_dx_countlimit_t *)entries;

			ext4_directory_dx_countlimit_set_count(new_root_countlimit, 1);
			ext4_directory_dx_entry_set_block(entries, new_iblock);

			((ext4_directory_dx_root_t *)dx_blocks[0].block->data)->info.indirect_levels = 1;

			/* Add new access path frame */
			dx_block = dx_blocks + 1;
			dx_block->position = dx_block->position - entries + new_entries;
			dx_block->entries = new_entries;
			dx_block->block = new_block;
		}

	}

	return EOK;
}

int ext4_directory_dx_add_entry(ext4_inode_ref_t *parent,
		ext4_inode_ref_t *child, const char *name)
{
	int rc = EOK;
	int rc2 = EOK;

	// get direct block 0 (index root)
	uint32_t root_block_addr;
	rc = ext4_filesystem_get_inode_data_block_index(parent, 0, &root_block_addr);
	if (rc != EOK) {
		return rc;
	}

	ext4_filesystem_t *fs = parent->fs;

	block_t *root_block;
	rc = block_get(&root_block, fs->device, root_block_addr, BLOCK_FLAGS_NONE);
	if (rc != EOK) {
		return rc;
	}

	uint32_t name_len = strlen(name);
	ext4_hash_info_t hinfo;
	rc = ext4_directory_hinfo_init(&hinfo, root_block, fs->superblock, name_len, name);
	if (rc != EOK) {
		block_put(root_block);
		EXT4FS_DBG("hinfo initialization error");
		return EXT4_ERR_BAD_DX_DIR;
	}

	// Hardcoded number 2 means maximum height of index tree !!!
	ext4_directory_dx_block_t dx_blocks[2];
	ext4_directory_dx_block_t *dx_block, *dx_it;
	rc = ext4_directory_dx_get_leaf(&hinfo, parent, root_block, &dx_block, dx_blocks);
	if (rc != EOK) {
		EXT4FS_DBG("get leaf error");
		rc = EXT4_ERR_BAD_DX_DIR;
		goto release_index;
	}


	// Try to insert to existing data block
	uint32_t leaf_block_idx = ext4_directory_dx_entry_get_block(dx_block->position);
	uint32_t leaf_block_addr;
   	rc = ext4_filesystem_get_inode_data_block_index(parent, leaf_block_idx, &leaf_block_addr);
   	if (rc != EOK) {
   		goto release_index;
   	}


   	block_t *target_block;
   	rc = block_get(&target_block, fs->device, leaf_block_addr, BLOCK_FLAGS_NONE);
   	if (rc != EOK) {
   		goto release_index;
   	}


   	rc = ext4_directory_try_insert_entry(fs->superblock, target_block, child, name, name_len);
   	if (rc == EOK) {
   		goto release_target_index;
   	}

    EXT4FS_DBG("no free space found");

	rc = ext4_directory_dx_split_index(fs, parent, dx_blocks, dx_block);
	if (rc != EOK) {
		goto release_target_index;
	}

	block_t *new_block = NULL;
	rc = ext4_directory_dx_split_data(fs, parent, &hinfo, target_block, dx_block, &new_block);
	if (rc != EOK) {
		rc2 = rc;
		goto release_target_index;
	}

	// Where to save new entry
	uint32_t new_block_hash = ext4_directory_dx_entry_get_hash(dx_block->position + 1);
	if (hinfo.hash >= new_block_hash) {
		rc = ext4_directory_try_insert_entry(fs->superblock, new_block, child, name, name_len);
	} else {
		rc = ext4_directory_try_insert_entry(fs->superblock, target_block, child, name, name_len);
	}




	rc = block_put(new_block);
	if (rc != EOK) {
		EXT4FS_DBG("error writing new block");
		return rc;
	}

release_target_index:

	rc2 = rc;

	rc = block_put(target_block);
	if (rc != EOK) {
		EXT4FS_DBG("error writing target block");
		return rc;
	}

release_index:

	if (rc != EOK) {
		rc2 = rc;
	}

	dx_it = dx_blocks;

	while (dx_it <= dx_block) {
		rc = block_put(dx_it->block);
		if (rc != EOK) {
			EXT4FS_DBG("error writing index block");
			return rc;
		}
		dx_it++;
	}

	return rc2;
}


/**
 * @}
 */ 
