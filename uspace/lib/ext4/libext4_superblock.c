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

/** Get logarithmic fragment size (1024 << size)
 *
 * @param sb		superblock
 * @return			logarithmic fragment size
 */
uint32_t ext4_superblock_get_log_frag_size(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->log_frag_size);
}

/** Set logarithmic fragment size (1024 << size)
 *
 * @param sb		superblock
 * @return			logarithmic fragment size
 */

void ext4_superblock_set_log_frag_size(ext4_superblock_t *sb, uint32_t frag_size)
{
	sb->log_frag_size = host2uint32_t_le(frag_size);
}

/** Get size of fragment (in bytes).
 *
 * @param sb		superblock
 * @return			size of fragment
 */
uint32_t ext4_superblock_get_frag_size(ext4_superblock_t *sb)
{
	return 1024 << ext4_superblock_get_log_frag_size(sb);
}

/** Set size of fragment (in bytes).
 *
 * @param sb		superblock
 * @param size		size of fragment (must be power of 2, at least 1024)
 */
void ext4_superblock_set_frag_size(ext4_superblock_t *sb, uint32_t size)
{
	uint32_t log = 0;
	uint32_t tmp = size / EXT4_MIN_BLOCK_SIZE;

	tmp >>= 1;
	while (tmp) {
		log++;
		tmp >>= 1;
	}

	ext4_superblock_set_log_frag_size(sb, log);
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

/** Get number of fragments per block group (except last BG)
 *
 * @param sb		superblock
 * @return			fragments per block group
 */
uint32_t ext4_superblock_get_frags_per_group(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->frags_per_group);
}

/** Set number of fragment per block group (except last BG)
 *
 * @param sb		superblock
 * @param frags		fragments per block group
 */
void ext4_superblock_set_frags_per_group(ext4_superblock_t *sb, uint32_t frags)
{
	sb->frags_per_group = host2uint32_t_le(frags);
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

/** Get filesystem state.
 *
 * @param sb		superblock
 * @return			filesystem state
 */
uint16_t ext4_superblock_get_state(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->state);
}

/** Set filesystem state.
 *
 * @param sb		superblock
 * @param state		filesystem state
 */
void ext4_superblock_set_state(ext4_superblock_t *sb, uint16_t state)
{
	sb->state = host2uint16_t_le(state);
}

/** Get behavior code when errors detected.
 *
 * @param sb		superblock
 * @return			behavior code
 */
uint16_t ext4_superblock_get_errors(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->errors);
}

/** Set behavior code when errors detected.
 *
 * @param sb		superblock
 * @param errors 	behavior code
 */
void ext4_superblock_set_errors(ext4_superblock_t *sb, uint16_t errors)
{
	sb->errors = host2uint16_t_le(errors);
}

/** Get minor revision level of the filesystem.
 *
 * @param sb		superblock
 * @return			minor revision level
 */
uint16_t ext4_superblock_get_minor_rev_level(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->minor_rev_level);
}

/** Set minor revision level of the filesystem.
 *
 * @param sb		superblock
 * @param level 	minor revision level
 */
void ext4_superblock_set_minor_rev_level(ext4_superblock_t *sb, uint16_t level)
{
	sb->minor_rev_level = host2uint16_t_le(level);
}

/** Get time of the last filesystem check.
 *
 * @param sb		superblock
 * @return			time of the last check (POSIX)
 */
uint32_t ext4_superblock_get_last_check_time(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->last_check_time);
}

/** Set time of the last filesystem check.
 *
 * @param sb		superblock
 * @param time		time of the last check (POSIX)
 */
void ext4_superblock_set_last_check_time(ext4_superblock_t *sb, uint32_t time)
{
	sb->state = host2uint32_t_le(time);
}

/** Get maximum time interval between two filesystem checks.
 *
 * @param sb		superblock
 * @return			time interval between two check (POSIX)
 */
uint32_t ext4_superblock_get_check_interval(ext4_superblock_t *sb){
	return uint32_t_le2host(sb->check_interval);
}

/** Set maximum time interval between two filesystem checks.
 *
 * @param sb			superblock
 * @param interval		time interval between two check (POSIX)
 */
void ext4_superblock_set_check_interval(ext4_superblock_t *sb, uint32_t interval)
{
	sb->check_interval = host2uint32_t_le(interval);
}

/** Get operation system identifier, on which the filesystem was created.
 *
 * @param sb		superblock
 * @return			operation system identifier
 */
uint32_t ext4_superblock_get_creator_os(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->creator_os);
}

/** Set operation system identifier, on which the filesystem was created.
 *
 * @param sb		superblock
 * @param os		operation system identifier
 */
void ext4_superblock_set_creator_os(ext4_superblock_t *sb, uint32_t os)
{
	sb->creator_os = host2uint32_t_le(os);
}

/** Get revision level of the filesystem.
 *
 * @param sb		superblock
 * @return			revision level
 */
uint32_t ext4_superblock_get_rev_level(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->rev_level);
}

/** Set revision level of the filesystem.
 *
 * @param sb		superblock
 * @param level 	revision level
 */
void ext4_superblock_set_rev_level(ext4_superblock_t *sb, uint32_t level)
{
	sb->rev_level = host2uint32_t_le(level);
}

/** Get default user id for reserved blocks.
 *
 * @param sb		superblock
 * @return			default user id for reserved blocks.
 */
uint16_t ext4_superblock_get_def_resuid(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->def_resuid);
}

/** Set default user id for reserved blocks.
 *
 * @param sb		superblock
 * @param uid		default user id for reserved blocks.
 */
void ext4_superblock_set_def_resuid(ext4_superblock_t *sb, uint16_t uid)
{
	sb->def_resuid = host2uint16_t_le(uid);
}

/** Get default group id for reserved blocks.
 *
 * @param sb		superblock
 * @return			default group id for reserved blocks.
 */
uint16_t ext4_superblock_get_def_resgid(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->def_resgid);
}

/** Set default group id for reserved blocks.
 *
 * @param sb		superblock
 * @param gid		default group id for reserved blocks.
 */
void ext4_superblock_set_def_resgid(ext4_superblock_t *sb, uint16_t gid)
{
	sb->def_resgid = host2uint16_t_le(gid);
}

/** Get index of the first i-node, which can be used for allocation.
 *
 * @param sb		superblock
 * @return			i-node index
 */
uint32_t ext4_superblock_get_first_inode(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->first_inode);
}

/** Set index of the first i-node, which can be used for allocation.
 *
 * @param sb			superblock
 * @param first_inode	i-node index
 */
void ext4_superblock_set_first_inode(ext4_superblock_t *sb, uint32_t first_inode)
{
	sb->first_inode = host2uint32_t_le(first_inode);
}

/** Get size of i-node structure.
 *
 * For the oldest revision return constant number.
 *
 * @param sb			superblock
 * @return				size of i-node structure
 */
uint16_t ext4_superblock_get_inode_size(ext4_superblock_t *sb)
{
	if (ext4_superblock_get_rev_level(sb) == 0) {
		return EXT4_REV0_INODE_SIZE;
	}
	return uint16_t_le2host(sb->inode_size);
}

/** Set size of i-node structure.
 *
 * @param sb			superblock
 * @param size			size of i-node structure
 */
void ext4_superblock_set_inode_size(ext4_superblock_t *sb, uint16_t size)
{
	sb->inode_size = host2uint16_t_le(size);
}

/** Get index of block group, where superblock copy is located.
 *
 * @param sb			superblock
 * @return				block group index
 */
uint16_t ext4_superblock_get_block_group_index(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->block_group_index);
}

/** Set index of block group, where superblock copy is located.
 *
 * @param sb			superblock
 * @param bgid			block group index
 */
void ext4_superblock_set_block_group_index(ext4_superblock_t *sb, uint16_t bgid)
{
	sb->block_group_index = host2uint16_t_le(bgid);
}

/** Get compatible features supported by the filesystem.
 *
 * @param sb		superblock
 * @return			compatible features bitmap
 */
uint32_t ext4_superblock_get_features_compatible(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->features_compatible);
}

/** Set compatible features supported by the filesystem.
 *
 * @param sb			superblock
 * @param features		compatible features bitmap
 */
void ext4_superblock_set_features_compatible(ext4_superblock_t *sb, uint32_t features)
{
	sb->features_compatible = host2uint32_t_le(features);
}

/** Get incompatible features supported by the filesystem.
 *
 * @param sb		superblock
 * @return			incompatible features bitmap
 */
uint32_t ext4_superblock_get_features_incompatible(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->features_incompatible);
}

/** Set incompatible features supported by the filesystem.
 *
 * @param sb			superblock
 * @param features		incompatible features bitmap
 */
void ext4_superblock_set_features_incompatible(ext4_superblock_t *sb, uint32_t features)
{
	sb->features_incompatible = host2uint32_t_le(features);
}

/** Get compatible features supported by the filesystem.
 *
 * @param sb		superblock
 * @return			read-only compatible features bitmap
 */
uint32_t ext4_superblock_get_features_read_only(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->features_read_only);
}

/** Set compatible features supported by the filesystem.
 *
 * @param sb			superblock
 * @param feature		read-only compatible features bitmap
 */
void ext4_superblock_set_features_read_only(ext4_superblock_t *sb, uint32_t features)
{
	sb->features_read_only = host2uint32_t_le(features);
}

/** Get UUID of the filesystem.
 *
 * @param sb		superblock
 * @return 			pointer to UUID array
 */
const uint8_t * ext4_superblock_get_uuid(ext4_superblock_t *sb)
{
	return sb->uuid;
}

/** Set UUID of the filesystem.
 *
 * @param sb		superblock
 * @param uuid		pointer to UUID array
 */
void ext4_superblock_set_uuid(ext4_superblock_t *sb, const uint8_t *uuid)
{
	memcpy(sb->uuid, uuid, sizeof(sb->uuid));
}

/** Get name of the filesystem volume.
 *
 * @param sb		superblock
 * @return			name of the volume
 */
const char * ext4_superblock_get_volume_name(ext4_superblock_t *sb)
{
	return sb->volume_name;
}

/** Set name of the filesystem volume.
 *
 * @param sb		superblock
 * @param name		new name of the volume
 */
void ext4_superblock_set_volume_name(ext4_superblock_t *sb, const char *name)
{
	memcpy(sb->volume_name, name, sizeof(sb->volume_name));
}

/** Get name of the directory, where this filesystem was mounted at last.
 *
 * @param sb		superblock
 * @return 			directory name
 */
const char * ext4_superblock_get_last_mounted(ext4_superblock_t *sb)
{
	return sb->last_mounted;
}

/** Set name of the directory, where this filesystem was mounted at last.
 *
 * @param sb		superblock
 * @param last		directory name
 */
void ext4_superblock_set_last_mounted(ext4_superblock_t *sb, const char *last)
{
	memcpy(sb->last_mounted, last, sizeof(sb->last_mounted));
}

/** Get last orphaned i-node index.
 *
 * Orphans are stored in linked list.
 *
 * @param sb		superblock
 * @return			last orphaned i-node index
 */
uint32_t ext4_superblock_get_last_orphan(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->last_orphan);
}

/** Set last orphaned i-node index.
 *
 * Orphans are stored in linked list.
 *
 * @param sb			superblock
 * @param last_orphan	last orphaned i-node index
 */
void ext4_superblock_set_last_orphan(ext4_superblock_t *sb, uint32_t last_orphan)
{
	sb->last_orphan = host2uint32_t_le(last_orphan);
}

/** Get hash seed for directory index hash function.
 *
 * @param sb		superblock
 * @return			hash seed pointer
 */
const uint32_t * ext4_superblock_get_hash_seed(ext4_superblock_t *sb)
{
	return sb->hash_seed;
}

/** Set hash seed for directory index hash function.
 *
 * @param sb		superblock
 * @param seed		hash seed pointer
 */
void ext4_superblock_set_hash_seed(ext4_superblock_t *sb, const uint32_t *seed)
{
	memcpy(sb->hash_seed, seed, sizeof(sb->hash_seed));
}

/** Get default version of the hash algorithm version for directory index.
 *
 * @param sb		superblock
 * @return			default hash version
 */
uint8_t ext4_superblock_get_default_hash_version(ext4_superblock_t *sb)
{
	return sb->default_hash_version;
}

/** Set default version of the hash algorithm version for directory index.
 *
 * @param sb		superblock
 * @param version	default hash version
 */
void ext4_superblock_set_default_hash_version(ext4_superblock_t *sb, uint8_t version)
{
	sb->default_hash_version = version;
}

/** Get size of block group descriptor structure.
 *
 * Output value is checked for minimal size.
 *
 * @param sb		superblock
 * @return			size of block group descriptor
 */
uint16_t ext4_superblock_get_desc_size(ext4_superblock_t *sb)
{
	uint16_t size = uint16_t_le2host(sb->desc_size);

	if (size < EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE) {
		size = EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE;
	}

	return size;
}

/** Set size of block group descriptor structure.
 *
 * Input value is checked for minimal size.
 *
 * @param sb		superblock
 * @param size 		size of block group descriptor
 */
void ext4_superblock_set_desc_size(ext4_superblock_t *sb, uint16_t size)
{
	if (size < EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE) {
		sb->desc_size = host2uint16_t_le(EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE);
	}

	sb->desc_size = host2uint16_t_le(size);
}

/** Get superblock flags.
 *
 * @param sb		superblock
 * @return 			flags from the superblock
 */
uint32_t ext4_superblock_get_flags(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->flags);
}

/** Set superblock flags.
 *
 * @param sb		superblock
 * @param flags		flags for the superblock
 */
void ext4_superblock_set_flags(ext4_superblock_t *sb, uint32_t flags)
{
	sb->flags = host2uint32_t_le(flags);
}

/*
 * More complex superblock operations
 */

/** Check if superblock has specified flag.
 *
 * @param sb			superblock
 * @param flag			flag to be checked
 * @return				true, if superblock has the flag
 */
bool ext4_superblock_has_flag(ext4_superblock_t *sb, uint32_t flag)
{
	if (ext4_superblock_get_flags(sb) & flag) {
		return true;
	}
	return false;
}

/** Check if filesystem supports compatible feature.
 *
 * @param sb			superblock
 * @param feature		feature to be checked
 * @return				true, if filesystem supports the feature
 */
bool ext4_superblock_has_feature_compatible(ext4_superblock_t *sb, uint32_t feature)
{
	if (ext4_superblock_get_features_compatible(sb) & feature) {
		return true;
	}
	return false;
}

/** Check if filesystem supports incompatible feature.
 *
 * @param sb			superblock
 * @param feature		feature to be checked
 * @return				true, if filesystem supports the feature
 */
bool ext4_superblock_has_feature_incompatible(ext4_superblock_t *sb, uint32_t feature)
{
	if (ext4_superblock_get_features_incompatible(sb) & feature) {
		return true;
	}
	return false;
}

/** Check if filesystem supports read-only compatible feature.
 *
 * @param sb			superblock
 * @param feature		feature to be checked
 * @return				true, if filesystem supports the feature
 */
bool ext4_superblock_has_feature_read_only(ext4_superblock_t *sb, uint32_t feature)
{
	if (ext4_superblock_get_features_read_only(sb) & feature) {
		return true;
	}
	return false;
}

/** Read superblock directly from block device.
 *
 * @param service_id		block device identifier
 * @param sb				output pointer to memory structure
 * @return					error code.
 */
int ext4_superblock_read_direct(service_id_t service_id,
    ext4_superblock_t **sb)
{
	int rc;

	/* Allocated memory for superblock structure */
	void *data = malloc(EXT4_SUPERBLOCK_SIZE);
	if (data == NULL) {
		return ENOMEM;
	}

	/* Read data from block device */
	rc = block_read_bytes_direct(service_id, EXT4_SUPERBLOCK_OFFSET,
	    EXT4_SUPERBLOCK_SIZE, data);

	if (rc != EOK) {
		free(data);
		return rc;
	}

	/* Set output value */
	(*sb) = data;

	return EOK;
}

/** Write superblock structure directly to block device.
 *
 * @param service_id		block device identifier
 * @param sb				superblock to be written
 * @return					error code
 */
int ext4_superblock_write_direct(service_id_t service_id,
		ext4_superblock_t *sb)
{
	int rc;
	uint32_t phys_block_size;

	/* Load physical block size from block device */
	rc = block_get_bsize(service_id, &phys_block_size);
	if (rc != EOK) {
		return rc;
	}

	/* Compute address of the first block */
	uint64_t first_block = EXT4_SUPERBLOCK_OFFSET / phys_block_size;
	/* Compute number of block to write */
	uint32_t block_count = EXT4_SUPERBLOCK_SIZE / phys_block_size;

	/* Check alignment */
	if (EXT4_SUPERBLOCK_SIZE % phys_block_size) {
		block_count++;
	}

	/* Write data */
	return block_write_direct(service_id, first_block, block_count, sb);

}

/** Check sanity of the superblock.
 *
 * This check is performed at mount time.
 * Checks are described by one-line comments in the code.
 *
 * @param sb		superblock to check
 * @return			error code
 */
int ext4_superblock_check_sanity(ext4_superblock_t *sb)
{
	if (ext4_superblock_get_magic(sb) != EXT4_SUPERBLOCK_MAGIC) {
		return ENOTSUP;
	}

	if (ext4_superblock_get_inodes_count(sb) == 0) {
		return ENOTSUP;
	}

	if (ext4_superblock_get_blocks_count(sb) == 0) {
		return ENOTSUP;
	}

	if (ext4_superblock_get_blocks_per_group(sb) == 0) {
		return ENOTSUP;
	}

	if (ext4_superblock_get_inodes_per_group(sb) == 0) {
		return ENOTSUP;
	}

	if (ext4_superblock_get_inode_size(sb) < 128) {
		return ENOTSUP;
	}

	if (ext4_superblock_get_first_inode(sb) < 11) {
		return ENOTSUP;
	}

	if (ext4_superblock_get_desc_size(sb) < EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE) {
		return ENOTSUP;
	}

	if (ext4_superblock_get_desc_size(sb) > EXT4_MAX_BLOCK_GROUP_DESCRIPTOR_SIZE) {
		return ENOTSUP;
	}

	// TODO more checks !!!

	return EOK;
}

/** Compute number of block groups in the filesystem.
 *
 * @param sb		superblock
 * @return			number of block groups
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

/** Compute number of blocks in specified block group.
 *
 * @param sb			superblock
 * @param bgid			block group index
 * @return				number of blocks
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

/** Compute number of i-nodes in specified block group.
 *
 * @param sb		superblock
 * @param bgid		block group index
 * @return			number of i-nodes
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
