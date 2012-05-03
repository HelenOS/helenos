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
 * @file	libext4_superblock.c
 * @brief	Ext4 superblock operations.
 */

#include <byteorder.h>
#include <errno.h>
#include <libblock.h>
#include <malloc.h>
#include "libext4.h"

/** Get number of i-nodes in the whole filesystem.
 *
 * @param sb		superblock
 * @return			number of i-nodes
 */
uint32_t ext4_superblock_get_inodes_count(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->inodes_count);
}

/** Set number of i-nodes in the whole filesystem.
 *
 * @param sb		superblock
 * @param count		number of i-nodes
 */
void ext4_superblock_set_inodes_count(ext4_superblock_t *sb, uint32_t count)
{
	sb->inodes_count = host2uint32_t_le(count);
}

/** Get number of data blocks in the whole filesystem.
 *
 * @param sb		superblock
 * @return			number of data blocks
 */
uint64_t ext4_superblock_get_blocks_count(ext4_superblock_t *sb)
{
	return ((uint64_t)uint32_t_le2host(sb->blocks_count_hi) << 32) |
			uint32_t_le2host(sb->blocks_count_lo);
}

/** Set number of data blocks in the whole filesystem.
 *
 * @param sb		superblock
 * @param count		number of data blocks
 */
void ext4_superblock_set_blocks_count(ext4_superblock_t *sb, uint64_t count)
{
	sb->blocks_count_lo = host2uint32_t_le((count << 32) >> 32);
	sb->blocks_count_hi = host2uint32_t_le(count >> 32);
}

/** Get number of reserved data blocks in the whole filesystem.
 *
 * @param sb		superblock
 * @return			number of reserved data blocks
 */
uint64_t ext4_superblock_get_reserved_blocks_count(ext4_superblock_t *sb)
{
	return ((uint64_t)uint32_t_le2host(sb->reserved_blocks_count_hi) << 32) |
			uint32_t_le2host(sb->reserved_blocks_count_lo);
}

/** Set number of reserved data blocks in the whole filesystem.
 *
 * @param sb		superblock
 * @param count		number of reserved data blocks
 */
void ext4_superblock_set_reserved_blocks_count(ext4_superblock_t *sb, uint64_t count)
{
	sb->reserved_blocks_count_lo = host2uint32_t_le((count << 32) >> 32);
	sb->reserved_blocks_count_hi = host2uint32_t_le(count >> 32);
}

/** Get number of free data blocks in the whole filesystem.
 *
 * @param sb		superblock
 * @return			number of free data blocks
 */
uint64_t ext4_superblock_get_free_blocks_count(ext4_superblock_t *sb)
{
	return ((uint64_t)uint32_t_le2host(sb->free_blocks_count_hi) << 32) |
			uint32_t_le2host(sb->free_blocks_count_lo);
}

/** Set number of free data blocks in the whole filesystem.
 *
 * @param sb		superblock
 * @param count 	number of free data blocks
 */
void ext4_superblock_set_free_blocks_count(ext4_superblock_t *sb, uint64_t count)
{
	sb->free_blocks_count_lo = host2uint32_t_le((count << 32) >> 32);
	sb->free_blocks_count_hi = host2uint32_t_le(count >> 32);
}

/** Get number of free i-nodes in the whole filesystem.
 *
 * @param sb		superblock
 * @return			number of free i-nodes
 */
uint32_t ext4_superblock_get_free_inodes_count(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->free_inodes_count);
}

/** Set number of free i-nodes in the whole filesystem.
 *
 * @param sb		superblock
 * @param count 	number of free i-nodes
 */
void ext4_superblock_set_free_inodes_count(ext4_superblock_t *sb, uint32_t count)
{
	sb->free_inodes_count = host2uint32_t_le(count);
}

/** Get index of first data block (block, where is located superblock)
 *
 * @param sb		superblock
 * @return			index of the first data block
 */
uint32_t ext4_superblock_get_first_data_block(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->first_data_block);
}

/** Set index of first data block (block, where is located superblock)
 *
 * @param sb		superblock
 * @param first 	index of the first data block
 */
void ext4_superblock_set_first_data_block(ext4_superblock_t *sb, uint32_t first)
{
	sb->first_data_block = host2uint32_t_le(first);
}

/** Get logarithmic block size (1024 << size == block_size)
 *
 * @param sb		superblock
 * @return			logarithmic block size
 */
uint32_t ext4_superblock_get_log_block_size(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->log_block_size);
}

/** Set logarithmic block size (1024 << size == block_size)
 *
 * @param sb		superblock
 * @return			logarithmic block size
 */
void ext4_superblock_set_log_block_size(ext4_superblock_t *sb, uint32_t log_size)
{
	sb->log_block_size = host2uint32_t_le(log_size);
}

/** Get size of data block (in bytes).
 *
 * @param sb		superblock
 * @return			size of data block
 */
uint32_t ext4_superblock_get_block_size(ext4_superblock_t *sb)
{
	return 1024 << ext4_superblock_get_log_block_size(sb);
}

/** Set size of data block (in bytes).
 *
 * @param sb		superblock
 * @param size		size of data block (must be power of 2, at least 1024)
 */
void ext4_superblock_set_block_size(ext4_superblock_t *sb, uint32_t size)
{
	uint32_t log = 0;
	uint32_t tmp = size / EXT4_MIN_BLOCK_SIZE;

	tmp >>= 1;
	while (tmp) {
		log++;
		tmp >>= 1;
	}

	ext4_superblock_set_log_block_size(sb, log);
}

/** Get number of data blocks per block group (except last BG)
 *
 * @param sb		superblock
 * @return			data blocks per block group
 */
uint32_t ext4_superblock_get_blocks_per_group(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->blocks_per_group);
}

/** Set number of data blocks per block group (except last BG)
 *
 * @param sb		superblock
 * @param blocks	data blocks per block group
 */
void ext4_superblock_set_blocks_per_group(ext4_superblock_t *sb, uint32_t blocks)
{
	sb->blocks_per_group = host2uint32_t_le(blocks);
}

/** Get number of i-nodes per block group (except last BG)
 *
 * @param sb		superblock
 * @return			i-nodes per block group
 */
uint32_t ext4_superblock_get_inodes_per_group(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->inodes_per_group);
}

/** Set number of i-nodes per block group (except last BG)
 *
 * @param sb		superblock
 * @param inodes	i-nodes per block group
 */
void ext4_superblock_set_inodes_per_group(ext4_superblock_t *sb, uint32_t inodes)
{
	sb->inodes_per_group = host2uint32_t_le(inodes);
}

/** Get time when filesystem was mounted (POSIX time).
 *
 * @param sb		superblock
 * @return			mount time
 */
uint32_t ext4_superblock_get_mount_time(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->mount_time);
}

/** Set time when filesystem was mounted (POSIX time).
 *
 * @param sb		superblock
 * @param time		mount time
 */
void ext4_superblock_set_mount_time(ext4_superblock_t *sb, uint32_t time)
{
	sb->mount_time = host2uint32_t_le(time);
}

/** Get time when filesystem was last accesed by write operation (POSIX time).
 *
 * @param sb		superblock
 * @return			write time
 */
uint32_t ext4_superblock_get_write_time(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->write_time);
}

/** Set time when filesystem was last accesed by write operation (POSIX time).
 *
 * @param sb		superblock
 * @param time		write time
 */
void ext4_superblock_set_write_time(ext4_superblock_t *sb, uint32_t time)
{
	sb->write_time = host2uint32_t_le(time);
}

/** Get number of mount from last filesystem check.
 *
 * @param sb		superblock
 * @return			number of mounts
 */
uint16_t ext4_superblock_get_mount_count(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->mount_count);
}

/** Set number of mount from last filesystem check.
 *
 * @param sb		superblock
 * @param count		number of mounts
 */
void ext4_superblock_set_mount_count(ext4_superblock_t *sb, uint16_t count)
{
	sb->mount_count = host2uint16_t_le(count);
}

/** Get maximum number of mount from last filesystem check.
 *
 * @param sb		superblock
 * @return			maximum number of mounts
 */
uint16_t ext4_superblock_get_max_mount_count(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->max_mount_count);
}

/** Set maximum number of mount from last filesystem check.
 *
 * @param sb		superblock
 * @param count		maximum number of mounts
 */
void ext4_superblock_set_max_mount_count(ext4_superblock_t *sb, uint16_t count)
{
	sb->max_mount_count = host2uint16_t_le(count);
}

/** Get superblock magic value.
 *
 * @param sb		superblock
 * @return			magic value
 */
uint16_t ext4_superblock_get_magic(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->magic);
}

/** Set superblock magic value.
 *
 * @param sb		superblock
 * @param			magic value
 */
void ext4_superblock_set_magic(ext4_superblock_t *sb, uint16_t magic)
{
	sb->magic = host2uint16_t_le(magic);
}

/** TODO comment
 *
 */
uint16_t ext4_superblock_get_state(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->state);
}

/** TODO comment
 *
 */
void ext4_superblock_set_state(ext4_superblock_t *sb, uint16_t state)
{
	sb->state = host2uint16_t_le(state);
}

/** TODO comment
 *
 */
uint16_t ext4_superblock_get_errors(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->errors);
}

/** TODO comment
 *
 */
void ext4_superblock_set_errors(ext4_superblock_t *sb, uint16_t errors)
{
	sb->errors = host2uint16_t_le(errors);
}

/** TODO comment
 *
 */
uint16_t ext4_superblock_get_minor_rev_level(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->minor_rev_level);
}

/** TODO comment
 *
 */
void ext4_superblock_set_minor_rev_level(ext4_superblock_t *sb, uint16_t level)
{
	sb->minor_rev_level = host2uint16_t_le(level);
}

/** TODO comment
 *
 */
uint32_t ext4_superblock_get_last_check_time(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->last_check_time);
}

/** TODO comment
 *
 */
void ext4_superblock_set_last_check_time(ext4_superblock_t *sb, uint32_t time)
{
	sb->state = host2uint32_t_le(time);
}

/** TODO comment
 *
 */
uint32_t ext4_superblock_get_check_interval(ext4_superblock_t *sb){
	return uint32_t_le2host(sb->check_interval);
}

/** TODO comment
 *
 */
void ext4_superblock_set_check_interval(ext4_superblock_t *sb, uint32_t interval)
{
	sb->check_interval = host2uint32_t_le(interval);
}

/** TODO comment
 *
 */
uint32_t ext4_superblock_get_creator_os(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->creator_os);
}

/** TODO comment
 *
 */
void ext4_superblock_set_creator_os(ext4_superblock_t *sb, uint32_t os)
{
	sb->creator_os = host2uint32_t_le(os);
}

/** TODO comment
 *
 */
uint32_t ext4_superblock_get_rev_level(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->rev_level);
}

/** TODO comment
 *
 */
void ext4_superblock_set_rev_level(ext4_superblock_t *sb, uint32_t level)
{
	sb->rev_level = host2uint32_t_le(level);
}

/** TODO comment
 *
 */
uint16_t ext4_superblock_get_def_resuid(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->def_resuid);
}

/** TODO comment
 *
 */
void ext4_superblock_set_def_resuid(ext4_superblock_t *sb, uint16_t uid)
{
	sb->def_resuid = host2uint16_t_le(uid);
}

/** TODO comment
 *
 */
uint16_t ext4_superblock_get_def_resgid(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->def_resgid);
}

/** TODO comment
 *
 */
void ext4_superblock_set_def_resgid(ext4_superblock_t *sb, uint16_t gid)
{
	sb->def_resgid = host2uint16_t_le(gid);
}

/** TODO comment
 *
 */
uint32_t ext4_superblock_get_first_inode(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->first_inode);
}

/** TODO comment
 *
 */
void ext4_superblock_set_first_inode(ext4_superblock_t *sb, uint32_t first_inode)
{
	sb->first_inode = host2uint32_t_le(first_inode);
}

/** TODO comment
 *
 */
uint16_t ext4_superblock_get_inode_size(ext4_superblock_t *sb)
{
	if (ext4_superblock_get_rev_level(sb) == 0) {
		return EXT4_REV0_INODE_SIZE;
	}
	return uint16_t_le2host(sb->inode_size);
}

/** TODO comment
 *
 */
void ext4_superblock_set_inode_size(ext4_superblock_t *sb, uint16_t size)
{
	sb->inode_size = host2uint16_t_le(size);
}

/** TODO comment
 *
 */
uint16_t ext4_superblock_get_block_group_number(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->block_group_number);
}

/** TODO comment
 *
 */
void ext4_superblock_set_block_group_number(ext4_superblock_t *sb, uint16_t bg)
{
	sb->block_group_number = host2uint16_t_le(bg);
}

/** TODO comment
 *
 */
uint32_t ext4_superblock_get_features_compatible(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->features_compatible);
}

/** TODO comment
 *
 */
void ext4_superblock_set_features_compatible(ext4_superblock_t *sb, uint32_t features)
{
	sb->features_compatible = host2uint32_t_le(features);
}

/** TODO comment
 *
 */
uint32_t ext4_superblock_get_features_incompatible(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->features_incompatible);
}

/** TODO comment
 *
 */
void ext4_superblock_set_features_incompatible(ext4_superblock_t *sb, uint32_t features)
{
	sb->features_incompatible = host2uint32_t_le(features);
}

/** TODO comment
 *
 */
uint32_t ext4_superblock_get_features_read_only(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->features_read_only);
}

/** TODO comment
 *
 */
void ext4_superblock_set_features_read_only(ext4_superblock_t *sb, uint32_t features)
{
	sb->features_read_only = host2uint32_t_le(features);
}

/** TODO comment
 *
 */
const uint8_t * ext4_superblock_get_uuid(ext4_superblock_t *sb)
{
	return sb->uuid;
}

/** TODO comment
 *
 */
void ext4_superblock_set_uuid(ext4_superblock_t *sb, const uint8_t *uuid)
{
	memcpy(sb->uuid, uuid, sizeof(sb->uuid));
}

const char * ext4_superblock_get_volume_name(ext4_superblock_t *sb)
{
	return sb->volume_name;
}

/** TODO comment
 *
 */
void ext4_superblock_set_volume_name(ext4_superblock_t *sb, const char *name)
{
	memcpy(sb->volume_name, name, sizeof(sb->volume_name));
}

/** TODO comment
 *
 */
const char * ext4_superblock_get_last_mounted(ext4_superblock_t *sb)
{
	return sb->last_mounted;
}

/** TODO comment
 *
 */
void ext4_superblock_set_last_mounted(ext4_superblock_t *sb, const char *last)
{
	memcpy(sb->last_mounted, last, sizeof(sb->last_mounted));
}

/** TODO comment
 *
 */
uint32_t ext4_superblock_get_last_orphan(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->last_orphan);
}

/** TODO comment
 *
 */
void ext4_superblock_set_last_orphan(ext4_superblock_t *sb, uint32_t last_orphan)
{
	sb->last_orphan = host2uint32_t_le(last_orphan);
}

/** TODO comment
 *
 */
uint32_t* ext4_superblock_get_hash_seed(ext4_superblock_t *sb)
{
	return sb->hash_seed;
}

/** TODO comment
 *
 */
uint8_t ext4_superblock_get_default_hash_version(ext4_superblock_t *sb)
{
	return sb->default_hash_version;
}

/** TODO comment
 *
 */
void ext4_superblock_set_default_hash_version(ext4_superblock_t *sb, uint8_t version)
{
	sb->default_hash_version = version;
}

/** TODO comment
 *
 */
uint16_t ext4_superblock_get_desc_size(ext4_superblock_t *sb)
{
	uint16_t size = uint16_t_le2host(sb->desc_size);

	if (size < EXT4_BLOCK_MIN_GROUP_DESCRIPTOR_SIZE) {
		size = EXT4_BLOCK_MIN_GROUP_DESCRIPTOR_SIZE;
	}

	return size;
}

/** TODO comment
 *
 */
void ext4_superblock_set_desc_size(ext4_superblock_t *sb, uint16_t size)
{
	sb->desc_size = host2uint16_t_le(size);
}

/** TODO comment
 *
 */
uint32_t ext4_superblock_get_flags(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->flags);
}

/** TODO comment
 *
 */
void ext4_superblock_set_flags(ext4_superblock_t *sb, uint32_t flags)
{
	sb->flags = host2uint32_t_le(flags);
}


/*
 * More complex superblock operations
 */

/** TODO comment
 *
 */
bool ext4_superblock_has_flag(ext4_superblock_t *sb, uint32_t flag)
{
	if (ext4_superblock_get_flags(sb) & flag) {
		return true;
	}
	return false;
}

/** TODO comment
 *
 */
bool ext4_superblock_has_feature_compatible(ext4_superblock_t *sb, uint32_t feature)
{
	if (ext4_superblock_get_features_compatible(sb) & feature) {
		return true;
	}
	return false;
}

/** TODO comment
 *
 */
bool ext4_superblock_has_feature_incompatible(ext4_superblock_t *sb, uint32_t feature)
{
	if (ext4_superblock_get_features_incompatible(sb) & feature) {
		return true;
	}
	return false;
}

/** TODO comment
 *
 */
bool ext4_superblock_has_feature_read_only(ext4_superblock_t *sb, uint32_t feature)
{
	if (ext4_superblock_get_features_read_only(sb) & feature) {
		return true;
	}
	return false;
}

/** TODO comment
 *
 */
int ext4_superblock_read_direct(service_id_t service_id,
    ext4_superblock_t **superblock)
{
	int rc;

	void *data = malloc(EXT4_SUPERBLOCK_SIZE);
	if (data == NULL) {
		return ENOMEM;
	}

	rc = block_read_bytes_direct(service_id, EXT4_SUPERBLOCK_OFFSET,
	    EXT4_SUPERBLOCK_SIZE, data);

	if (rc != EOK) {
		free(data);
		return rc;
	}

	(*superblock) = data;

	return EOK;
}

/** TODO comment
 *
 */
int ext4_superblock_write_direct(service_id_t service_id,
		ext4_superblock_t *sb)
{
	int rc;
	uint32_t phys_block_size;

	rc = block_get_bsize(service_id, &phys_block_size);
	if (rc != EOK) {
		// TODO error
		return rc;
	}

	uint64_t first_block = EXT4_SUPERBLOCK_OFFSET / phys_block_size;
	uint32_t block_count = EXT4_SUPERBLOCK_SIZE / phys_block_size;

	if (EXT4_SUPERBLOCK_SIZE % phys_block_size) {
		block_count++;
	}

	return block_write_direct(service_id, first_block, block_count, sb);

}

/** TODO comment
 *
 */
int ext4_superblock_check_sanity(ext4_superblock_t *sb)
{
	if (ext4_superblock_get_magic(sb) != EXT4_SUPERBLOCK_MAGIC) {
		return ENOTSUP;
	}

	// block size
	// desc size


	// TODO more checks !!!

	return EOK;
}

/** TODO comment
 *
 */
uint32_t ext4_superblock_get_block_group_count(ext4_superblock_t *sb)
{
	uint64_t blocks_count = ext4_superblock_get_blocks_count(sb);
	uint32_t blocks_per_group = ext4_superblock_get_blocks_per_group(sb);

	uint32_t block_groups_count = blocks_count / blocks_per_group;

	if (blocks_count % blocks_per_group) {
		block_groups_count++;
	}

	return block_groups_count;

}

/** TODO comment
 *
 */
uint32_t ext4_superblock_get_blocks_in_group(ext4_superblock_t *sb, uint32_t bgid)
{
	uint32_t block_group_count = ext4_superblock_get_block_group_count(sb);
	uint32_t blocks_per_group = ext4_superblock_get_blocks_per_group(sb);
	uint64_t total_blocks = ext4_superblock_get_blocks_count(sb);

	if (bgid < block_group_count - 1) {
		return blocks_per_group;
	} else {
		return (total_blocks - ((block_group_count - 1) * blocks_per_group));
	}

}

/** TODO comment
 *
 */
uint32_t ext4_superblock_get_inodes_in_group(ext4_superblock_t *sb, uint32_t bgid)
{
	uint32_t block_group_count = ext4_superblock_get_block_group_count(sb);
	uint32_t inodes_per_group = ext4_superblock_get_inodes_per_group(sb);
	uint32_t total_inodes = ext4_superblock_get_inodes_count(sb);

	if (bgid < block_group_count - 1) {
		return inodes_per_group;
	} else {
		return (total_inodes - ((block_group_count - 1) * inodes_per_group));
	}

}

/**
 * @}
 */ 
