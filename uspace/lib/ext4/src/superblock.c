/*
 * Copyright (c) 2018 Jiri Svoboda
 * Copyright (c) 2011 Martin Sucha
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
 * @file  superblock.c
 * @brief Ext4 superblock operations.
 */

#include <align.h>
#include <block.h>
#include <byteorder.h>
#include <errno.h>
#include <mem.h>
#include <stdlib.h>
#include <str.h>
#include <time.h>
#include "ext4/cfg.h"
#include "ext4/superblock.h"

/** Get number of i-nodes in the whole filesystem.
 *
 * @param sb Superblock
 *
 * @return Number of i-nodes
 *
 */
uint32_t ext4_superblock_get_inodes_count(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->inodes_count);
}

/** Set number of i-nodes in the whole filesystem.
 *
 * @param sb    Superblock
 * @param count Number of i-nodes
 *
 */
void ext4_superblock_set_inodes_count(ext4_superblock_t *sb, uint32_t count)
{
	sb->inodes_count = host2uint32_t_le(count);
}

/** Get number of data blocks in the whole filesystem.
 *
 * @param sb Superblock
 *
 * @return Number of data blocks
 *
 */
uint64_t ext4_superblock_get_blocks_count(ext4_superblock_t *sb)
{
	return ((uint64_t) uint32_t_le2host(sb->blocks_count_hi) << 32) |
	    uint32_t_le2host(sb->blocks_count_lo);
}

/** Set number of data blocks in the whole filesystem.
 *
 * @param sb    Superblock
 * @param count Number of data blocks
 *
 */
void ext4_superblock_set_blocks_count(ext4_superblock_t *sb, uint64_t count)
{
	sb->blocks_count_lo = host2uint32_t_le((count << 32) >> 32);
	sb->blocks_count_hi = host2uint32_t_le(count >> 32);
}

/** Get number of reserved data blocks in the whole filesystem.
 *
 * @param sb Superblock
 *
 * @return Number of reserved data blocks
 *
 */
uint64_t ext4_superblock_get_reserved_blocks_count(ext4_superblock_t *sb)
{
	return ((uint64_t)
	    uint32_t_le2host(sb->reserved_blocks_count_hi) << 32) |
	    uint32_t_le2host(sb->reserved_blocks_count_lo);
}

/** Set number of reserved data blocks in the whole filesystem.
 *
 * @param sb    Superblock
 * @param count Number of reserved data blocks
 *
 */
void ext4_superblock_set_reserved_blocks_count(ext4_superblock_t *sb,
    uint64_t count)
{
	sb->reserved_blocks_count_lo = host2uint32_t_le((count << 32) >> 32);
	sb->reserved_blocks_count_hi = host2uint32_t_le(count >> 32);
}

/** Get number of free data blocks in the whole filesystem.
 *
 * @param sb Superblock
 *
 * @return Number of free data blocks
 *
 */
uint64_t ext4_superblock_get_free_blocks_count(ext4_superblock_t *sb)
{
	return ((uint64_t)
	    uint32_t_le2host(sb->free_blocks_count_hi) << 32) |
	    uint32_t_le2host(sb->free_blocks_count_lo);
}

/** Set number of free data blocks in the whole filesystem.
 *
 * @param sb    Superblock
 * @param count Number of free data blocks
 *
 */
void ext4_superblock_set_free_blocks_count(ext4_superblock_t *sb,
    uint64_t count)
{
	sb->free_blocks_count_lo = host2uint32_t_le((count << 32) >> 32);
	sb->free_blocks_count_hi = host2uint32_t_le(count >> 32);
}

/** Get number of free i-nodes in the whole filesystem.
 *
 * @param sb Superblock
 *
 * @return Number of free i-nodes
 *
 */
uint32_t ext4_superblock_get_free_inodes_count(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->free_inodes_count);
}

/** Set number of free i-nodes in the whole filesystem.
 *
 * @param sb    Superblock
 * @param count Number of free i-nodes
 *
 */
void ext4_superblock_set_free_inodes_count(ext4_superblock_t *sb,
    uint32_t count)
{
	sb->free_inodes_count = host2uint32_t_le(count);
}

/** Get index of first data block (block where the superblock is located)
 *
 * @param sb Superblock
 *
 * @return Index of the first data block
 *
 */
uint32_t ext4_superblock_get_first_data_block(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->first_data_block);
}

/** Set index of first data block (block where the superblock is located)
 *
 * @param sb    Superblock
 * @param first Index of the first data block
 *
 */
void ext4_superblock_set_first_data_block(ext4_superblock_t *sb,
    uint32_t first)
{
	sb->first_data_block = host2uint32_t_le(first);
}

/** Get logarithmic block size (1024 << size == block_size)
 *
 * @param sb Superblock
 *
 * @return Logarithmic block size
 *
 */
uint32_t ext4_superblock_get_log_block_size(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->log_block_size);
}

/** Set logarithmic block size (1024 << size == block_size)
 *
 * @param sb Superblock
 *
 * @return Logarithmic block size
 *
 */
void ext4_superblock_set_log_block_size(ext4_superblock_t *sb,
    uint32_t log_size)
{
	sb->log_block_size = host2uint32_t_le(log_size);
}

/** Get size of data block (in bytes).
 *
 * @param sb Superblock
 *
 * @return Size of data block
 *
 */
uint32_t ext4_superblock_get_block_size(ext4_superblock_t *sb)
{
	return 1024 << ext4_superblock_get_log_block_size(sb);
}

/** Set size of data block (in bytes).
 *
 * @param sb   Superblock
 * @param size Size of data block (must be power of 2, at least 1024)
 *
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
 * @param sb Superblock
 *
 * @return Logarithmic fragment size
 *
 */
uint32_t ext4_superblock_get_log_frag_size(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->log_frag_size);
}

/** Set logarithmic fragment size (1024 << size)
 *
 * @param sb        Superblock
 * @param frag_size Logarithmic fragment size
 *
 */
void ext4_superblock_set_log_frag_size(ext4_superblock_t *sb,
    uint32_t frag_size)
{
	sb->log_frag_size = host2uint32_t_le(frag_size);
}

/** Get size of fragment (in bytes).
 *
 * @param sb Superblock
 *
 * @return Size of fragment
 *
 */
uint32_t ext4_superblock_get_frag_size(ext4_superblock_t *sb)
{
	return 1024 << ext4_superblock_get_log_frag_size(sb);
}

/** Set size of fragment (in bytes).
 *
 * @param sb   Superblock
 * @param size Size of fragment (must be power of 2, at least 1024)
 *
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
 * @param sb Superblock
 *
 * @return Data blocks per block group
 *
 */
uint32_t ext4_superblock_get_blocks_per_group(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->blocks_per_group);
}

/** Set number of data blocks per block group (except last BG)
 *
 * @param sb     Superblock
 * @param blocks Data blocks per block group
 *
 */
void ext4_superblock_set_blocks_per_group(ext4_superblock_t *sb,
    uint32_t blocks)
{
	sb->blocks_per_group = host2uint32_t_le(blocks);
}

/** Get number of fragments per block group (except last BG)
 *
 * @param sb Superblock
 *
 * @return Fragments per block group
 *
 */
uint32_t ext4_superblock_get_frags_per_group(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->frags_per_group);
}

/** Set number of fragment per block group (except last BG)
 *
 * @param sb    Superblock
 * @param frags Fragments per block group
 */
void ext4_superblock_set_frags_per_group(ext4_superblock_t *sb, uint32_t frags)
{
	sb->frags_per_group = host2uint32_t_le(frags);
}

/** Get number of i-nodes per block group (except last BG)
 *
 * @param sb Superblock
 *
 * @return I-nodes per block group
 *
 */
uint32_t ext4_superblock_get_inodes_per_group(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->inodes_per_group);
}

/** Set number of i-nodes per block group (except last BG)
 *
 * @param sb     Superblock
 * @param inodes I-nodes per block group
 *
 */
void ext4_superblock_set_inodes_per_group(ext4_superblock_t *sb,
    uint32_t inodes)
{
	sb->inodes_per_group = host2uint32_t_le(inodes);
}

/** Get time when filesystem was mounted (POSIX time).
 *
 * @param sb Superblock
 *
 * @return Mount time
 *
 */
uint32_t ext4_superblock_get_mount_time(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->mount_time);
}

/** Set time when filesystem was mounted (POSIX time).
 *
 * @param sb   Superblock
 * @param time Mount time
 *
 */
void ext4_superblock_set_mount_time(ext4_superblock_t *sb, uint32_t time)
{
	sb->mount_time = host2uint32_t_le(time);
}

/** Get time when filesystem was last accesed by write operation (POSIX time).
 *
 * @param sb Superblock
 *
 * @return Write time
 *
 */
uint32_t ext4_superblock_get_write_time(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->write_time);
}

/** Set time when filesystem was last accesed by write operation (POSIX time).
 *
 * @param sb   Superblock
 * @param time Write time
 *
 */
void ext4_superblock_set_write_time(ext4_superblock_t *sb, uint32_t time)
{
	sb->write_time = host2uint32_t_le(time);
}

/** Get number of mount from last filesystem check.
 *
 * @param sb Superblock
 *
 * @return Number of mounts
 *
 */
uint16_t ext4_superblock_get_mount_count(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->mount_count);
}

/** Set number of mount from last filesystem check.
 *
 * @param sb    Superblock
 * @param count Number of mounts
 *
 */
void ext4_superblock_set_mount_count(ext4_superblock_t *sb, uint16_t count)
{
	sb->mount_count = host2uint16_t_le(count);
}

/** Get maximum number of mount from last filesystem check.
 *
 * @param sb Superblock
 *
 * @return Maximum number of mounts
 *
 */
uint16_t ext4_superblock_get_max_mount_count(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->max_mount_count);
}

/** Set maximum number of mount from last filesystem check.
 *
 * @param sb    Superblock
 * @param count Maximum number of mounts
 *
 */
void ext4_superblock_set_max_mount_count(ext4_superblock_t *sb, uint16_t count)
{
	sb->max_mount_count = host2uint16_t_le(count);
}

/** Get superblock magic value.
 *
 * @param sb Superblock
 *
 * @return Magic value
 *
 */
uint16_t ext4_superblock_get_magic(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->magic);
}

/** Set superblock magic value.
 *
 * @param sb    Superblock
 * @param magic Magic value
 *
 */
void ext4_superblock_set_magic(ext4_superblock_t *sb, uint16_t magic)
{
	sb->magic = host2uint16_t_le(magic);
}

/** Get filesystem state.
 *
 * @param sb Superblock
 *
 * @return Filesystem state
 *
 */
uint16_t ext4_superblock_get_state(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->state);
}

/** Set filesystem state.
 *
 * @param sb    Superblock
 * @param state Filesystem state
 *
 */
void ext4_superblock_set_state(ext4_superblock_t *sb, uint16_t state)
{
	sb->state = host2uint16_t_le(state);
}

/** Get behavior code when errors detected.
 *
 * @param sb Superblock
 *
 * @return Behavior code
 *
 */
uint16_t ext4_superblock_get_errors(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->errors);
}

/** Set behavior code when errors detected.
 *
 * @param sb     Superblock
 * @param errors Behavior code
 *
 */
void ext4_superblock_set_errors(ext4_superblock_t *sb, uint16_t errors)
{
	sb->errors = host2uint16_t_le(errors);
}

/** Get minor revision level of the filesystem.
 *
 * @param sb Superblock
 *
 * @return Minor revision level
 *
 */
uint16_t ext4_superblock_get_minor_rev_level(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->minor_rev_level);
}

/** Set minor revision level of the filesystem.
 *
 * @param sb    Superblock
 * @param level Minor revision level
 *
 */
void ext4_superblock_set_minor_rev_level(ext4_superblock_t *sb, uint16_t level)
{
	sb->minor_rev_level = host2uint16_t_le(level);
}

/** Get time of the last filesystem check.
 *
 * @param sb Superblock
 *
 * @return Time of the last check (POSIX)
 *
 */
uint32_t ext4_superblock_get_last_check_time(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->last_check_time);
}

/** Set time of the last filesystem check.
 *
 * @param sb   Superblock
 * @param time Time of the last check (POSIX)
 *
 */
void ext4_superblock_set_last_check_time(ext4_superblock_t *sb, uint32_t time)
{
	sb->last_check_time = host2uint32_t_le(time);
}

/** Get maximum time interval between two filesystem checks.
 *
 * @param sb Superblock
 *
 * @return Time interval between two check (POSIX)
 *
 */
uint32_t ext4_superblock_get_check_interval(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->check_interval);
}

/** Set maximum time interval between two filesystem checks.
 *
 * @param sb       Superblock
 * @param interval Time interval between two check (POSIX)
 *
 */
void ext4_superblock_set_check_interval(ext4_superblock_t *sb, uint32_t interval)
{
	sb->check_interval = host2uint32_t_le(interval);
}

/** Get operation system identifier, on which the filesystem was created.
 *
 * @param sb Superblock
 *
 * @return Operation system identifier
 *
 */
uint32_t ext4_superblock_get_creator_os(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->creator_os);
}

/** Set operation system identifier, on which the filesystem was created.
 *
 * @param sb Superblock
 * @param os Operation system identifier
 *
 */
void ext4_superblock_set_creator_os(ext4_superblock_t *sb, uint32_t os)
{
	sb->creator_os = host2uint32_t_le(os);
}

/** Get revision level of the filesystem.
 *
 * @param sb Superblock
 *
 * @return Revision level
 *
 */
uint32_t ext4_superblock_get_rev_level(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->rev_level);
}

/** Set revision level of the filesystem.
 *
 * @param sb    Superblock
 * @param level Revision level
 *
 */
void ext4_superblock_set_rev_level(ext4_superblock_t *sb, uint32_t level)
{
	sb->rev_level = host2uint32_t_le(level);
}

/** Get default user id for reserved blocks.
 *
 * @param sb Superblock
 *
 * @return Default user id for reserved blocks.
 *
 */
uint16_t ext4_superblock_get_def_resuid(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->def_resuid);
}

/** Set default user id for reserved blocks.
 *
 * @param sb  Superblock
 * @param uid Default user id for reserved blocks.
 *
 */
void ext4_superblock_set_def_resuid(ext4_superblock_t *sb, uint16_t uid)
{
	sb->def_resuid = host2uint16_t_le(uid);
}

/** Get default group id for reserved blocks.
 *
 * @param sb Superblock
 *
 * @return Default group id for reserved blocks.
 *
 */
uint16_t ext4_superblock_get_def_resgid(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->def_resgid);
}

/** Set default group id for reserved blocks.
 *
 * @param sb  Superblock
 * @param gid Default group id for reserved blocks.
 *
 */
void ext4_superblock_set_def_resgid(ext4_superblock_t *sb, uint16_t gid)
{
	sb->def_resgid = host2uint16_t_le(gid);
}

/** Get index of the first i-node, which can be used for allocation.
 *
 * @param sb Superblock
 *
 * @return I-node index
 *
 */
uint32_t ext4_superblock_get_first_inode(ext4_superblock_t *sb)
{
	if (ext4_superblock_get_rev_level(sb) == 0)
		return EXT4_REV0_FIRST_INO;

	return uint32_t_le2host(sb->first_inode);
}

/** Set index of the first i-node, which can be used for allocation.
 *
 * @param sb          Superblock
 * @param first_inode I-node index
 *
 */
void ext4_superblock_set_first_inode(ext4_superblock_t *sb,
    uint32_t first_inode)
{
	sb->first_inode = host2uint32_t_le(first_inode);
}

/** Get size of i-node structure.
 *
 * For the oldest revision return constant number.
 *
 * @param sb Superblock
 *
 * @return Size of i-node structure
 *
 */
uint16_t ext4_superblock_get_inode_size(ext4_superblock_t *sb)
{
	if (ext4_superblock_get_rev_level(sb) == 0)
		return EXT4_REV0_INODE_SIZE;

	return uint16_t_le2host(sb->inode_size);
}

/** Set size of i-node structure.
 *
 * @param sb   Superblock
 * @param size Size of i-node structure
 *
 */
void ext4_superblock_set_inode_size(ext4_superblock_t *sb, uint16_t size)
{
	sb->inode_size = host2uint16_t_le(size);
}

/** Get index of block group, where superblock copy is located.
 *
 * @param sb Superblock
 *
 * @return Block group index
 *
 */
uint16_t ext4_superblock_get_block_group_index(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->block_group_index);
}

/** Set index of block group, where superblock copy is located.
 *
 * @param sb   Superblock
 * @param bgid Block group index
 *
 */
void ext4_superblock_set_block_group_index(ext4_superblock_t *sb, uint16_t bgid)
{
	sb->block_group_index = host2uint16_t_le(bgid);
}

/** Get compatible features supported by the filesystem.
 *
 * @param sb Superblock
 *
 * @return Compatible features bitmap
 *
 */
uint32_t ext4_superblock_get_features_compatible(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->features_compatible);
}

/** Set compatible features supported by the filesystem.
 *
 * @param sb       Superblock
 * @param features Compatible features bitmap
 *
 */
void ext4_superblock_set_features_compatible(ext4_superblock_t *sb,
    uint32_t features)
{
	sb->features_compatible = host2uint32_t_le(features);
}

/** Get incompatible features supported by the filesystem.
 *
 * @param sb Superblock
 *
 * @return Incompatible features bitmap
 *
 */
uint32_t ext4_superblock_get_features_incompatible(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->features_incompatible);
}

/** Set incompatible features supported by the filesystem.
 *
 * @param sb       Superblock
 * @param features Incompatible features bitmap
 *
 */
void ext4_superblock_set_features_incompatible(ext4_superblock_t *sb,
    uint32_t features)
{
	sb->features_incompatible = host2uint32_t_le(features);
}

/** Get compatible features supported by the filesystem.
 *
 * @param sb Superblock
 *
 * @return Read-only compatible features bitmap
 *
 */
uint32_t ext4_superblock_get_features_read_only(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->features_read_only);
}

/** Set compatible features supported by the filesystem.
 *
 * @param sb      Superblock
 * @param feature Read-only compatible features bitmap
 *
 */
void ext4_superblock_set_features_read_only(ext4_superblock_t *sb,
    uint32_t features)
{
	sb->features_read_only = host2uint32_t_le(features);
}

/** Get UUID of the filesystem.
 *
 * @param sb superblock
 *
 * @return Pointer to UUID array
 *
 */
void ext4_superblock_get_uuid(ext4_superblock_t *sb, uuid_t *uuid)
{
	uuid_decode(sb->uuid, uuid);
}

/** Set UUID of the filesystem.
 *
 * @param sb   Superblock
 * @param uuid Pointer to UUID
 *
 */
void ext4_superblock_set_uuid(ext4_superblock_t *sb, uuid_t *uuid)
{
	uuid_encode(uuid, sb->uuid);
}

/** Get name of the filesystem volume.
 *
 * @param sb Superblock
 *
 * @return Name of the volume
 *
 */
errno_t ext4_superblock_get_volume_name(ext4_superblock_t *sb, char *buf,
    size_t bufsz)
{
	size_t i;
	size_t wi;
	char32_t ch;
	errno_t rc;

	i = 0;
	wi = 0;
	while (sb->volume_name[i] != '\0' && i < sizeof(sb->volume_name)) {
		/* ISO 8859-1 codes map to identical Unicode code points */
		ch = (char32_t)(uint8_t)sb->volume_name[i];
		rc = chr_encode(ch, buf, &wi, bufsz - 1);
		if (rc != EOK)
			return rc;

		i++;
	}

	buf[wi] = '\0';
	return EOK;
}

/** Set name of the filesystem volume.
 *
 * @param sb   Superblock
 * @param name New name of the volume
 * @return EOK on success or error code
 */
errno_t ext4_superblock_set_volume_name(ext4_superblock_t *sb, const char *name)
{
	size_t off;
	char32_t ch;
	size_t wi;

	off = 0;
	wi = 0;
	while (wi < sizeof(sb->volume_name)) {
		ch = str_decode(name, &off, STR_NO_LIMIT);
		if (ch == 0)
			break;
		if (ch > 255)
			return EINVAL;

		sb->volume_name[wi++] = ch;
	}

	while (wi < sizeof(sb->volume_name))
		sb->volume_name[wi++] = '\0';

	return EOK;
}

/** Get name of the directory, where this filesystem was mounted at last.
 *
 * @param sb Superblock
 *
 * @return Directory name
 *
 */
const char *ext4_superblock_get_last_mounted(ext4_superblock_t *sb)
{
	return sb->last_mounted;
}

/** Set name of the directory, where this filesystem was mounted at last.
 *
 * @param sb   Superblock
 * @param last Directory name
 *
 */
void ext4_superblock_set_last_mounted(ext4_superblock_t *sb, const char *last)
{
	memcpy(sb->last_mounted, last, sizeof(sb->last_mounted));
}

/** Get last orphaned i-node index.
 *
 * Orphans are stored in linked list.
 *
 * @param sb Superblock
 *
 * @return Last orphaned i-node index
 *
 */
uint32_t ext4_superblock_get_last_orphan(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->last_orphan);
}

/** Set last orphaned i-node index.
 *
 * Orphans are stored in linked list.
 *
 * @param sb          Superblock
 * @param last_orphan Last orphaned i-node index
 *
 */
void ext4_superblock_set_last_orphan(ext4_superblock_t *sb,
    uint32_t last_orphan)
{
	sb->last_orphan = host2uint32_t_le(last_orphan);
}

/** Get hash seed for directory index hash function.
 *
 * @param sb Superblock
 *
 * @return Hash seed pointer
 *
 */
const uint32_t *ext4_superblock_get_hash_seed(ext4_superblock_t *sb)
{
	return sb->hash_seed;
}

/** Set hash seed for directory index hash function.
 *
 * @param sb   Superblock
 * @param seed Hash seed pointer
 *
 */
void ext4_superblock_set_hash_seed(ext4_superblock_t *sb, const uint32_t *seed)
{
	memcpy(sb->hash_seed, seed, sizeof(sb->hash_seed));
}

/** Get default version of the hash algorithm version for directory index.
 *
 * @param sb Superblock
 *
 * @return Default hash version
 *
 */
uint8_t ext4_superblock_get_default_hash_version(ext4_superblock_t *sb)
{
	return sb->default_hash_version;
}

/** Set default version of the hash algorithm version for directory index.
 *
 * @param sb      Superblock
 * @param version Default hash version
 *
 */
void ext4_superblock_set_default_hash_version(ext4_superblock_t *sb,
    uint8_t version)
{
	sb->default_hash_version = version;
}

/** Get size of block group descriptor structure.
 *
 * Output value is checked for minimal size.
 *
 * @param sb Superblock
 *
 * @return Size of block group descriptor
 *
 */
uint16_t ext4_superblock_get_desc_size(ext4_superblock_t *sb)
{
	uint16_t size = uint16_t_le2host(sb->desc_size);

	if (size < EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		size = EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE;

	return size;
}

/** Set size of block group descriptor structure.
 *
 * Input value is checked for minimal size.
 *
 * @param sb   Superblock
 * @param size Size of block group descriptor
 *
 */
void ext4_superblock_set_desc_size(ext4_superblock_t *sb, uint16_t size)
{
	if (size < EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		sb->desc_size =
		    host2uint16_t_le(EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE);

	sb->desc_size = host2uint16_t_le(size);
}

/** Get superblock flags.
 *
 * @param sb Superblock
 *
 * @return Flags from the superblock
 *
 */
uint32_t ext4_superblock_get_flags(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->flags);
}

/** Set superblock flags.
 *
 * @param sb    Superblock
 * @param flags Flags for the superblock
 *
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
 * @param sb   Superblock
 * @param flag Flag to be checked
 *
 * @return True, if superblock has the flag
 *
 */
bool ext4_superblock_has_flag(ext4_superblock_t *sb, uint32_t flag)
{
	if (ext4_superblock_get_flags(sb) & flag)
		return true;

	return false;
}

/** Check if filesystem supports compatible feature.
 *
 * @param sb      Superblock
 * @param feature Feature to be checked
 *
 * @return True, if filesystem supports the feature
 *
 */
bool ext4_superblock_has_feature_compatible(ext4_superblock_t *sb,
    uint32_t feature)
{
	if (ext4_superblock_get_features_compatible(sb) & feature)
		return true;

	return false;
}

/** Check if filesystem supports incompatible feature.
 *
 * @param sb      Superblock
 * @param feature Feature to be checked
 *
 * @return True, if filesystem supports the feature
 *
 */
bool ext4_superblock_has_feature_incompatible(ext4_superblock_t *sb,
    uint32_t feature)
{
	if (ext4_superblock_get_features_incompatible(sb) & feature)
		return true;

	return false;
}

/** Check if filesystem supports read-only compatible feature.
 *
 * @param sb      Superblock
 * @param feature Feature to be checked
 *
 * @return True, if filesystem supports the feature
 *
 */
bool ext4_superblock_has_feature_read_only(ext4_superblock_t *sb,
    uint32_t feature)
{
	if (ext4_superblock_get_features_read_only(sb) & feature)
		return true;

	return false;
}

/** Read superblock directly from block device.
 *
 * @param service_id Block device identifier
 * @param sb         Output pointer to memory structure
 *
 * @return Eerror code.
 *
 */
errno_t ext4_superblock_read_direct(service_id_t service_id, ext4_superblock_t **sb)
{
	/* Allocated memory for superblock structure */
	void *data = malloc(EXT4_SUPERBLOCK_SIZE);
	if (data == NULL)
		return ENOMEM;

	/* Read data from block device */
	errno_t rc = block_read_bytes_direct(service_id, EXT4_SUPERBLOCK_OFFSET,
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
 * @param service_id Block device identifier
 * @param sb         Superblock to be written
 *
 * @return Error code
 *
 */
errno_t ext4_superblock_write_direct(service_id_t service_id, ext4_superblock_t *sb)
{
	/* Load physical block size from block device */
	size_t phys_block_size;
	errno_t rc = block_get_bsize(service_id, &phys_block_size);
	if (rc != EOK)
		return rc;

	/* Compute address of the first block */
	uint64_t first_block = EXT4_SUPERBLOCK_OFFSET / phys_block_size;

	/* Compute number of block to write */
	size_t block_count = EXT4_SUPERBLOCK_SIZE / phys_block_size;

	/* Check alignment */
	if (EXT4_SUPERBLOCK_SIZE % phys_block_size)
		block_count++;

	/* Write data */
	return block_write_direct(service_id, first_block, block_count, sb);
}

/** Release the memory allocated for the superblock structure
 *
 * @param sb         Superblock to be freed
 *
 */
void ext4_superblock_release(ext4_superblock_t *sb)
{
	free(sb);
}

/** Check sanity of the superblock.
 *
 * This check is performed at mount time.
 * Checks are described by one-line comments in the code.
 *
 * @param sb Superblock to check
 *
 * @return Error code
 *
 */
errno_t ext4_superblock_check_sanity(ext4_superblock_t *sb)
{
	if (ext4_superblock_get_magic(sb) != EXT4_SUPERBLOCK_MAGIC)
		return ENOTSUP;

	if (ext4_superblock_get_inodes_count(sb) == 0)
		return ENOTSUP;

	if (ext4_superblock_get_blocks_count(sb) == 0)
		return ENOTSUP;

	if (ext4_superblock_get_blocks_per_group(sb) == 0)
		return ENOTSUP;

	if (ext4_superblock_get_inodes_per_group(sb) == 0)
		return ENOTSUP;

	if (ext4_superblock_get_inode_size(sb) < 128)
		return ENOTSUP;

	if (ext4_superblock_get_first_inode(sb) < 11)
		return ENOTSUP;

	if (ext4_superblock_get_desc_size(sb) <
	    EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		return ENOTSUP;

	if (ext4_superblock_get_desc_size(sb) >
	    EXT4_MAX_BLOCK_GROUP_DESCRIPTOR_SIZE)
		return ENOTSUP;

	return EOK;
}

/** Compute number of block groups in the filesystem.
 *
 * @param sb Superblock
 *
 * @return Number of block groups
 *
 */
uint32_t ext4_superblock_get_block_group_count(ext4_superblock_t *sb)
{
	uint64_t blocks_count = ext4_superblock_get_blocks_count(sb) - 1;
	uint32_t blocks_per_group = ext4_superblock_get_blocks_per_group(sb);

	uint32_t block_groups_count = blocks_count / blocks_per_group;

	if (blocks_count % blocks_per_group)
		block_groups_count++;

	return block_groups_count;
}

/** Compute number of blocks in specified block group.
 *
 * @param sb   Superblock
 * @param bgid Block group index
 *
 * @return Number of blocks
 *
 */
uint32_t ext4_superblock_get_blocks_in_group(ext4_superblock_t *sb, uint32_t bgid)
{
	uint32_t block_group_count =
	    ext4_superblock_get_block_group_count(sb);
	uint32_t blocks_per_group =
	    ext4_superblock_get_blocks_per_group(sb);
	uint64_t total_blocks =
	    ext4_superblock_get_blocks_count(sb);
	uint32_t first_block =
	    ext4_superblock_get_first_data_block(sb);

	if (bgid < block_group_count - 1)
		return blocks_per_group;
	else
		return (total_blocks - ((block_group_count - 1) * blocks_per_group)) - first_block;
}

/** Compute number of i-nodes in specified block group.
 *
 * @param sb   Superblock
 * @param bgid Block group index
 *
 * @return Number of i-nodes
 *
 */
uint32_t ext4_superblock_get_inodes_in_group(ext4_superblock_t *sb, uint32_t bgid)
{
	uint32_t block_group_count =
	    ext4_superblock_get_block_group_count(sb);
	uint32_t inodes_per_group =
	    ext4_superblock_get_inodes_per_group(sb);
	uint32_t total_inodes =
	    ext4_superblock_get_inodes_count(sb);

	if (bgid < block_group_count - 1)
		return inodes_per_group;
	else
		return (total_inodes - ((block_group_count - 1) * inodes_per_group));
}

/** Get the backup groups used with SPARSE_SUPER2
 *
 * @param sb    Pointer to the superblock
 * @param g1    Output pointer to the first backup group
 * @param g2    Output pointer to the second backup group
 */
void ext4_superblock_get_backup_groups_sparse2(ext4_superblock_t *sb,
    uint32_t *g1, uint32_t *g2)
{
	*g1 = uint32_t_le2host(sb->backup_bgs[0]);
	*g2 = uint32_t_le2host(sb->backup_bgs[1]);
}

/** Set the backup groups (SPARSE SUPER2)
 *
 * @param sb    Pointer to the superblock
 * @param g1    Index of the first group
 * @param g2    Index of the second group
 */
void ext4_superblock_set_backup_groups_sparse2(ext4_superblock_t *sb,
    uint32_t g1, uint32_t g2)
{
	sb->backup_bgs[0] = host2uint32_t_le(g1);
	sb->backup_bgs[1] = host2uint32_t_le(g2);
}

/** Get the number of blocks (per group) reserved to GDT expansion
 *
 * @param sb    Pointer to the superblock
 *
 * @return      Number of blocks
 */
uint32_t ext4_superblock_get_reserved_gdt_blocks(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->reserved_gdt_blocks);
}

/** Set the number of blocks (per group) reserved to GDT expansion
 *
 * @param sb    Pointer to the superblock
 * @param n     Number of reserved blocks
 */
void ext4_superblock_set_reserved_gdt_blocks(ext4_superblock_t *sb,
    uint32_t n)
{
	sb->reserved_gdt_blocks = host2uint32_t_le(n);
}

/** Get the size of the flex groups
 *
 * @param sb	Pointer to the superblock
 *
 * @return	Size of the flex groups
 */
uint32_t ext4_superblock_get_flex_group_size(ext4_superblock_t *sb)
{
	return 2 << sb->log_groups_per_flex;
}

/* Check if n is a power of p */
static bool is_power_of(uint32_t n, unsigned p)
{
	if (p == 1 && n != p)
		return false;

	while (n != p) {
		if (n < p)
			return false;
		else if ((n % p) != 0)
			return false;

		n /= p;
	}

	return true;
}

/** Get the number of blocks used by superblock + gdt + reserved gdt backups
 *
 * @param sb    Superblock
 * @param idx   Block group index
 *
 * @return      Number of blocks
 */
uint32_t ext4_superblock_get_group_backup_blocks(ext4_superblock_t *sb,
    uint32_t idx)
{
	uint32_t r = 0;
	bool has_backups = false;

	/* First step: determine if the block group contains the backups */

	if (idx <= 1)
		has_backups = true;
	else {
		if (ext4_superblock_has_feature_compatible(sb,
		    EXT4_FEATURE_COMPAT_SPARSE_SUPER2)) {
			uint32_t g1, g2;

			ext4_superblock_get_backup_groups_sparse2(sb,
			    &g1, &g2);

			if (idx == g1 || idx == g2)
				has_backups = true;
		} else if (!ext4_superblock_has_feature_read_only(sb,
		    EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER)) {
			/*
			 * Very old fs were all block groups have
			 * superblock and block descriptors backups.
			 */
			has_backups = true;
		} else {
			if ((idx & 1) && (is_power_of(idx, 3) ||
			    is_power_of(idx, 5) || is_power_of(idx, 7)))
				has_backups = true;
		}
	}

	if (has_backups) {
		uint32_t bg_count;
		uint32_t bg_desc_sz;
		uint32_t gdt_table; /* Size of the GDT in blocks */
		uint32_t block_size = ext4_superblock_get_block_size(sb);

		/*
		 * Now we know that this block group has backups,
		 * we have to compute how many blocks are reserved
		 * for them
		 */

		/* This accounts for the superblock */
		r++;

		/* Add the number of blocks used for the GDT */
		bg_count = ext4_superblock_get_block_group_count(sb);
		bg_desc_sz = ext4_superblock_get_desc_size(sb);
		gdt_table = ROUND_UP(bg_count * bg_desc_sz, block_size) /
		    block_size;

		r += gdt_table;

		/* And now the number of reserved GDT blocks */
		r += ext4_superblock_get_reserved_gdt_blocks(sb);
	}

	return r;
}

/** Create superblock for new file system.
 *
 * @param dev_bsize Device block size
 * @param dev_bcnt Device number of blocks
 * @param cfg Configuration of new file system
 * @param rsb Place to store pointer to newly allocated superblock
 * @return EOK on success or error code
 */
errno_t ext4_superblock_create(size_t dev_bsize, uint64_t dev_bcnt,
    ext4_cfg_t *cfg, ext4_superblock_t **rsb)
{
	ext4_superblock_t *sb;
	uuid_t uuid;
	uint32_t cur_ts;
	uint64_t first_block = 0;
	uint64_t fs_blocks;
	uint32_t blocks_count;
	uint32_t free_blocks;
	uint32_t inodes_count;
	uint64_t blocks_group;
	uint64_t inodes_group;
	uint32_t inodes_block;
	uint32_t inode_table_blocks;
	uint32_t res_blocks;
	uint32_t ngroups;
	uint32_t idx;
	size_t fs_bsize;
	size_t fs_bsize_log;
	errno_t rc;
	struct timespec ts;

	sb = calloc(1, sizeof(ext4_superblock_t));
	if (sb == NULL)
		return ENOMEM;

	rc = uuid_generate(&uuid);
	if (rc != EOK)
		goto error;

	/* Current UNIX time */
	getrealtime(&ts); // XXX ISO C does not say what the epoch is
	cur_ts = ts.tv_sec;

	fs_bsize = cfg->bsize;
	switch (fs_bsize) {
	case 1024:
		first_block = 1;
		fs_bsize_log = 0;
		blocks_group = 8192;
		break;
	case 2048:
		fs_bsize_log = 1;
		blocks_group = 8192 * 2;
		break;
	case 4096:
		fs_bsize_log = 2;
		blocks_group = 8192 * 4;
		break;
	default:
		return ENOTSUP;
	}

	if (fs_bsize % dev_bsize == 0) {
		/* Small device blocks */
		fs_blocks = dev_bcnt / (fs_bsize / dev_bsize);
	} else {
		/* Large device blocks */
		fs_blocks = dev_bcnt * (dev_bsize / fs_bsize);
	}

	/* Inodes per group */
	inodes_block = fs_bsize / EXT4_REV0_INODE_SIZE;
	inodes_group = min((fs_blocks - first_block) / 8,
	    blocks_group / 4);
	if (inodes_group < 16)
		inodes_group = 16;

	/* Align up to multiple of inodes_block */
	if (inodes_group % inodes_block != 0)
		inodes_group += inodes_block - inodes_group % inodes_block;
	inode_table_blocks = inodes_group / inodes_block;

	/* Number of groups */
	ngroups = ((fs_blocks - first_block) + blocks_group - 1) / blocks_group;

	/* Count of all blocks in groups */
	blocks_count = fs_blocks - first_block;

	/* Count of all inodes */
	inodes_count = ngroups * inodes_group;

	/* Count of blocks reserved for superuser */
	res_blocks = (blocks_count + 19) / 20;

	free_blocks = blocks_count;

	ext4_superblock_set_magic(sb, EXT4_SUPERBLOCK_MAGIC);
	ext4_superblock_set_inodes_count(sb, inodes_count);
	/* This seems to include the boot block, despite the ext2fs docs */
	ext4_superblock_set_blocks_count(sb, fs_blocks);
	ext4_superblock_set_reserved_blocks_count(sb, res_blocks);
	ext4_superblock_set_free_blocks_count(sb, free_blocks);
	ext4_superblock_set_free_inodes_count(sb, inodes_count);
	ext4_superblock_set_first_data_block(sb, first_block);
	ext4_superblock_set_log_block_size(sb, fs_bsize_log);
	/* Fragment size should be equal to block size */
	ext4_superblock_set_log_frag_size(sb, fs_bsize_log);
	ext4_superblock_set_blocks_per_group(sb, blocks_group);
	/* Should be the same as blocks per group. */
	ext4_superblock_set_frags_per_group(sb, blocks_group);
	ext4_superblock_set_inodes_per_group(sb, inodes_group);
	ext4_superblock_set_mount_time(sb, 0);
	ext4_superblock_set_write_time(sb, cur_ts);
	ext4_superblock_set_mount_count(sb, 0);
	ext4_superblock_set_max_mount_count(sb, (uint16_t)-1);
	ext4_superblock_set_state(sb, EXT4_SUPERBLOCK_STATE_VALID_FS);
	ext4_superblock_set_errors(sb, EXT4_SUPERBLOCK_ERRORS_CONTINUE);
	ext4_superblock_set_minor_rev_level(sb, 0); // XXX
	ext4_superblock_set_last_check_time(sb, cur_ts);
	ext4_superblock_set_check_interval(sb, 0);
	ext4_superblock_set_creator_os(sb, EXT4_SUPERBLOCK_OS_LINUX);
	if (cfg->version >= extver_ext2)
		ext4_superblock_set_rev_level(sb, EXT4_DYNAMIC_REV);
	else
		ext4_superblock_set_rev_level(sb, EXT4_GOOD_OLD_REV);
	ext4_superblock_set_def_resuid(sb, 0);
	ext4_superblock_set_def_resgid(sb, 0);

	if (cfg->version >= extver_ext2) {
		/* Dynamic rev */
		ext4_superblock_set_first_inode(sb, EXT4_REV0_FIRST_INO);
		ext4_superblock_set_inode_size(sb, EXT4_REV0_INODE_SIZE);
		ext4_superblock_set_block_group_index(sb, 0); // XXX
		ext4_superblock_set_features_compatible(sb, 0);
		ext4_superblock_set_features_incompatible(sb, 0);
		ext4_superblock_set_features_read_only(sb, 0);

		ext4_superblock_set_uuid(sb, &uuid);

		rc = ext4_superblock_set_volume_name(sb, cfg->volume_name);
		if (rc != EOK)
			goto error;

		/* 64-byte Latin-1 string padded with null characters */
		ext4_superblock_set_last_mounted(sb,
		    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
		    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");
		sb->algorithm_usage_bitmap = 0;
	}
#if 0
	/* Journalling */
	ext4_superblock_set_desc_size(sb, EXT4_MAX_BLOCK_GROUP_DESCRIPTOR_SIZE);
#endif

	/* Compute free blocks */
	free_blocks = blocks_count;
	for (idx = 0; idx < ngroups; idx++) {
		free_blocks -= ext4_superblock_get_group_backup_blocks(sb, idx);
		/* One for block bitmap, one for inode bitamp */
		free_blocks -= 2;
		free_blocks -= inode_table_blocks;
	}

	ext4_superblock_set_free_blocks_count(sb, free_blocks);

	*rsb = sb;
	return EOK;
error:
	free(sb);
	return rc;
}

/**
 * @}
 */
