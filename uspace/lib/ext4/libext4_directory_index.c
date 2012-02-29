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
 * @file	libext4_directory_index.c
 * @brief	Ext4 directory index operations.
 */

#include <byteorder.h>
#include <errno.h>
#include <malloc.h>
#include <sort.h>
#include <string.h>
#include "libext4.h"


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
		ext4_filesystem_t *fs, ext4_inode_t *inode, block_t *root_block,
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

        uint16_t entry_space = ext4_superblock_get_block_size(fs->superblock)
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


static int ext4_directory_dx_find_dir_entry(block_t *block,
		ext4_superblock_t *sb, size_t name_len, const char *name,
		ext4_directory_entry_ll_t **res_entry, aoff64_t *block_offset)
{

	aoff64_t offset = 0;
	ext4_directory_entry_ll_t *dentry = (ext4_directory_entry_ll_t *)block->data;
	uint8_t *addr_limit = block->data + ext4_superblock_get_block_size(sb);

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
		uint16_t dentry_len = ext4_directory_entry_ll_get_entry_length(dentry);

        if (dentry_len == 0) {
        	// Error
        	return -1;
        }

		offset += dentry_len;
		dentry = (ext4_directory_entry_ll_t *)((uint8_t *)dentry + dentry_len);
	}

	return 0;
}

static int ext4_directory_dx_next_block(ext4_filesystem_t *fs, ext4_inode_t *inode, uint32_t hash,
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
    	rc = ext4_filesystem_get_inode_data_block_index(fs, inode, block_idx, &block_addr);
    	if (rc != EOK) {
    		return rc;
    	}

    	block_t *block;
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

	// get direct block 0 (index root)
	uint32_t root_block_addr;
	rc = ext4_filesystem_get_inode_data_block_index(fs, inode_ref->inode, 0, &root_block_addr);
	if (rc != EOK) {
		return rc;
	}

	block_t *root_block;
	rc = block_get(&root_block, fs->device, root_block_addr, BLOCK_FLAGS_NONE);
	if (rc != EOK) {
		it->current_block = NULL;
		return rc;
	}

	ext4_hash_info_t hinfo;
	rc = ext4_directory_hinfo_init(&hinfo, root_block, fs->superblock, len, name);
	if (rc != EOK) {
		block_put(root_block);
		return EXT4_ERR_BAD_DX_DIR;
	}

	// Hardcoded number 2 means maximum height of index tree !!!
	ext4_directory_dx_block_t dx_blocks[2];
	ext4_directory_dx_block_t *dx_block;
	rc = ext4_directory_dx_get_leaf(&hinfo, fs, inode_ref->inode, root_block, &dx_block, dx_blocks);
	if (rc != EOK) {
		block_put(root_block);
		return EXT4_ERR_BAD_DX_DIR;
	}


	do {

		uint32_t leaf_block_idx = ext4_directory_dx_entry_get_block(dx_block->position);
		uint32_t leaf_block_addr;
    	rc = ext4_filesystem_get_inode_data_block_index(fs, inode_ref->inode, leaf_block_idx, &leaf_block_addr);
    	if (rc != EOK) {
    		return EXT4_ERR_BAD_DX_DIR;
    	}

    	block_t *leaf_block;
		rc = block_get(&leaf_block, fs->device, leaf_block_addr, BLOCK_FLAGS_NONE);
		if (rc != EOK) {
			return EXT4_ERR_BAD_DX_DIR;
		}

		aoff64_t block_offset;
		ext4_directory_entry_ll_t *res_dentry;
		rc = ext4_directory_dx_find_dir_entry(leaf_block, fs->superblock, len, name,
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

		rc = ext4_directory_dx_next_block(fs, inode_ref->inode, hinfo.hash, dx_block, &dx_blocks[0]);
		if (rc < 0) {
			// TODO cleanup
			return EXT4_ERR_BAD_DX_DIR;
		}

	} while (rc == 1);

	return ENOENT;
}

typedef struct ext4_dx_sort_entry {
	uint32_t hash;
	uint32_t rec_len;
	void *dentry;
} ext4_dx_sort_entry_t;

static int dx_entry_comparator(void *arg1, void *arg2, void *dummy)
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
	int rc;

	uint32_t block_size = ext4_superblock_get_block_size(fs->superblock);
	void *entry_buffer = malloc(block_size);
	if (entry_buffer == NULL) {
		return ENOMEM;
	}

	// Copy data to buffer
	memcpy(entry_buffer, old_data_block->data, block_size);

	// dot entry has the smallest size available
	uint32_t max_entry_count =  block_size / sizeof(ext4_directory_dx_dot_entry_t);

	ext4_dx_sort_entry_t *sort_array = malloc(max_entry_count * sizeof(ext4_dx_sort_entry_t));
	if (sort_array == NULL) {
		free(entry_buffer);
		return ENOMEM;
	}

	ext4_directory_entry_ll_t *dentry = old_data_block->data;

	uint32_t idx = 0;
	uint32_t real_size = 0;
	void *entry_buffer_ptr = entry_buffer;

	ext4_hash_info_t tmp_hinfo;
	memcpy(&tmp_hinfo, hinfo, sizeof(ext4_hash_info_t));

	while ((void *)dentry < old_data_block->data + block_size) {

		// Read only valid entries
		if (ext4_directory_entry_ll_get_inode(dentry) != 0) {
			char *name = (char *)dentry->name;

			uint8_t len = ext4_directory_entry_ll_get_name_length(fs->superblock, dentry);
			ext4_hash_string(&tmp_hinfo, len, name);

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

	qsort(sort_array, idx, sizeof(ext4_dx_sort_entry_t), dx_entry_comparator, NULL);

	uint32_t new_fblock;
	uint32_t new_iblock;
	rc = ext4_directory_append_block(fs, inode_ref, &new_fblock, &new_iblock);
	if (rc != EOK) {
		free(sort_array);
		free(entry_buffer);
		return rc;
	}

//	EXT4FS_DBG("new block appended (iblock = \%u, fblock = \%u)", new_iblock, new_fblock);

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


int ext4_directory_dx_add_entry(ext4_filesystem_t *fs,
		ext4_inode_ref_t *parent, ext4_inode_ref_t *child,
		size_t name_size, const char *name)
{
	int rc;

	// get direct block 0 (index root)
	uint32_t root_block_addr;
	rc = ext4_filesystem_get_inode_data_block_index(fs, parent->inode, 0, &root_block_addr);
	if (rc != EOK) {
		return rc;
	}

	block_t *root_block;
	rc = block_get(&root_block, fs->device, root_block_addr, BLOCK_FLAGS_NONE);
	if (rc != EOK) {
		return rc;
	}

	ext4_hash_info_t hinfo;
	rc = ext4_directory_hinfo_init(&hinfo, root_block, fs->superblock, name_size, name);
	if (rc != EOK) {
		block_put(root_block);
		return EXT4_ERR_BAD_DX_DIR;
	}

	// Hardcoded number 2 means maximum height of index tree !!!
	ext4_directory_dx_block_t dx_blocks[2];
	ext4_directory_dx_block_t *dx_block;
	rc = ext4_directory_dx_get_leaf(&hinfo, fs, parent->inode, root_block, &dx_block, dx_blocks);
	if (rc != EOK) {
		block_put(root_block);
		return EXT4_ERR_BAD_DX_DIR;
	}


	// Try to insert to existing data block
	uint32_t leaf_block_idx = ext4_directory_dx_entry_get_block(dx_block->position);
	uint32_t leaf_block_addr;
   	rc = ext4_filesystem_get_inode_data_block_index(fs, parent->inode, leaf_block_idx, &leaf_block_addr);
   	if (rc != EOK) {
   		return EXT4_ERR_BAD_DX_DIR;
   	}


   	block_t *target_block;
   	rc = block_get(&target_block, fs->device, leaf_block_addr, BLOCK_FLAGS_NONE);
   	if (rc != EOK) {
   		return EXT4_ERR_BAD_DX_DIR;
   	}

   	uint32_t block_size = ext4_superblock_get_block_size(fs->superblock);
   	uint16_t required_len = 8 + name_size + (4 - name_size % 4);

   	ext4_directory_entry_ll_t *de = target_block->data;
   	ext4_directory_entry_ll_t *stop = target_block->data + block_size;

   	while (de < stop) {

   		uint32_t de_inode = ext4_directory_entry_ll_get_inode(de);
   		uint16_t de_rec_len = ext4_directory_entry_ll_get_entry_length(de);

   		if ((de_inode == 0) && (de_rec_len >= required_len)) {
   			ext4_directory_write_entry(fs->superblock, de, de_rec_len,
   				child, name, name_size);

   				// TODO cleanup
   				target_block->dirty = true;
   				rc = block_put(target_block);
   				if (rc != EOK) {
   					return EXT4_ERR_BAD_DX_DIR;
   				}
   				return EOK;
   		}

   		if (de_inode != 0) {
   			uint16_t used_name_len = ext4_directory_entry_ll_get_name_length(
   					fs->superblock, de);

   			uint16_t used_space = 8 + used_name_len;
   			if ((used_name_len % 4) != 0) {
   				used_space += 4 - (used_name_len % 4);
   			}
   			uint16_t free_space = de_rec_len - used_space;

   			if (free_space >= required_len) {

   				// Cut tail of current entry
   				ext4_directory_entry_ll_set_entry_length(de, used_space);
   				ext4_directory_entry_ll_t *new_entry = (void *)de + used_space;
   				ext4_directory_write_entry(fs->superblock, new_entry,
   					free_space, child, name, name_size);

   				// TODO cleanup
   				target_block->dirty = true;
   				rc = block_put(target_block);
				if (rc != EOK) {
					return EXT4_ERR_BAD_DX_DIR;
				}
				return EOK;

   			}

   		}

   		de = (void *)de + de_rec_len;
   	}

    EXT4FS_DBG("no free space found");

	ext4_directory_dx_entry_t *entries = ((ext4_directory_dx_node_t *) dx_block->block->data)->entries;
	uint16_t leaf_limit = ext4_directory_dx_countlimit_get_limit((ext4_directory_dx_countlimit_t *)entries);
	uint16_t leaf_count = ext4_directory_dx_countlimit_get_count((ext4_directory_dx_countlimit_t *)entries);

	ext4_directory_dx_entry_t *root_entries = ((ext4_directory_dx_node_t *) dx_blocks[0].block->data)->entries;

	if (leaf_limit == leaf_count) {
		EXT4FS_DBG("need to split index block !!!");

		unsigned int levels = dx_block - dx_blocks;

		uint16_t root_limit = ext4_directory_dx_countlimit_get_limit((ext4_directory_dx_countlimit_t *)root_entries);
		uint16_t root_count = ext4_directory_dx_countlimit_get_count((ext4_directory_dx_countlimit_t *)root_entries);

		if ((levels > 0) && (root_limit == root_count)) {
			EXT4FS_DBG("Directory index is full");

			// ENOSPC - cleanup !!!
			return ENOSPC;
		}

		uint32_t new_fblock;
		uint32_t new_iblock;
		rc =  ext4_directory_append_block(fs, parent, &new_fblock, &new_iblock);
		if (rc != EOK) {
			// TODO error
		}

		// New block allocated
		block_t * new_block;
		rc = block_get(&new_block, fs->device, new_fblock, BLOCK_FLAGS_NOREAD);
		if (rc != EOK) {
			// TODO error
		}

		// Initialize block
		memset(new_block->data, 0, block_size);

		if (levels > 0) {
			EXT4FS_DBG("split index");
			uint32_t count_left = leaf_count / 2;
			uint32_t count_right = leaf_count - count_left;
			uint32_t hash_right = ext4_directory_dx_entry_get_hash(entries);

			ext4_directory_dx_node_t *new_node = new_block->data;
			ext4_directory_dx_entry_t *new_entries = new_node->entries;

			memcpy((void *) new_entries, (void *) (entries + count_left),
					count_right * sizeof(ext4_directory_dx_entry_t));

			ext4_directory_dx_countlimit_t *left_countlimit = (ext4_directory_dx_countlimit_t *)entries;
			ext4_directory_dx_countlimit_t *right_countlimit = (ext4_directory_dx_countlimit_t *)new_entries;

			ext4_directory_dx_countlimit_set_count(left_countlimit, count_left);
			ext4_directory_dx_countlimit_set_count(right_countlimit, count_right);

	        uint32_t entry_space = block_size - sizeof(ext4_fake_directory_entry_t);
	        uint32_t node_limit = entry_space / sizeof(ext4_directory_dx_entry_t);

	        ext4_directory_dx_countlimit_set_limit(right_countlimit, node_limit);

	        // Which index block is target for new entry
	        uint32_t position_index = (dx_block->position - dx_block->entries);
	        if (position_index >= count_left) {
	        	dx_block->position = new_entries + position_index - count_left;
	        	dx_block->entries = new_entries;
	        	entries = dx_block->entries;

	        	dx_block->block->dirty = true;
	        	block_put(dx_block->block);
	        	dx_block->block = new_block;
	        }

	    	ext4_directory_dx_insert_entry(dx_blocks, hash_right, new_iblock);


		} else {
			EXT4FS_DBG("create second level");
		}
	}

	block_t *new_block = NULL;
	rc = ext4_directory_dx_split_data(fs, parent, &hinfo, target_block, dx_block, &new_block);
	if (rc != EOK) {
		// TODO error
	}

	// TODO Where to save new entry
	uint32_t new_block_hash = ext4_directory_dx_entry_get_hash(dx_block->position + 1);
	if (hinfo.hash >= new_block_hash) {
		de = new_block->data;
		stop = new_block->data + block_size;
	} else {
		de = target_block->data;
		stop = target_block->data + block_size;
	}

   	while (de < stop) {

   		uint32_t de_inode = ext4_directory_entry_ll_get_inode(de);
   		uint16_t de_rec_len = ext4_directory_entry_ll_get_entry_length(de);

   		if ((de_inode == 0) && (de_rec_len >= required_len)) {
   			ext4_directory_write_entry(fs->superblock, de, de_rec_len,
   				child, name, name_size);
   				goto success;
   		}

   		if (de_inode != 0) {
   			uint16_t used_name_len = ext4_directory_entry_ll_get_name_length(
   					fs->superblock, de);

   			uint16_t used_space = 8 + used_name_len;
   			if ((used_name_len % 4) != 0) {
   				used_space += 4 - (used_name_len % 4);
   			}
   			uint16_t free_space = de_rec_len - used_space;

   			if (free_space >= required_len) {

   				// Cut tail of current entry
   				ext4_directory_entry_ll_set_entry_length(de, used_space);
   				ext4_directory_entry_ll_t *new_entry = (void *)de + used_space;
   				ext4_directory_write_entry(fs->superblock, new_entry,
   					free_space, child, name, name_size);

   				goto success;
   			}

   		}

   		de = (void *)de + de_rec_len;
   	}

success:

	rc = block_put(target_block);
	if (rc != EOK) {
		EXT4FS_DBG("error writing target block");
	}
	rc = block_put(new_block);
	if (rc != EOK) {
		EXT4FS_DBG("error writing new block");
	}

	ext4_directory_dx_block_t *dx_it = dx_blocks;

	while (dx_it <= dx_block) {
		rc = block_put(dx_it->block);
		if (rc != EOK) {
			EXT4FS_DBG("error writing index block \%u", (uint32_t)dx_it->block->pba);
		}
		dx_it++;
	}

	return EOK;
}


/**
 * @}
 */ 
