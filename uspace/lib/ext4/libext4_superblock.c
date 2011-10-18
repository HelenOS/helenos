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
 * @file	libext4_superblock.c
 * @brief	Ext4 superblock operations.
 */

#include <byteorder.h>
#include <errno.h>
#include <libblock.h>
#include <malloc.h>
#include "libext4.h"

uint32_t ext4_superblock_get_inodes_count(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->inodes_count);
}

uint64_t ext4_superblock_get_blocks_count(ext4_superblock_t *sb)
{
	return ((uint64_t)uint32_t_le2host(sb->blocks_count_hi) << 32) |
			uint32_t_le2host(sb->blocks_count_lo);
}

uint64_t ext4_superblock_get_reserved_blocks_count(ext4_superblock_t *sb)
{
	return ((uint64_t)uint32_t_le2host(sb->reserved_blocks_count_hi) << 32) |
			uint32_t_le2host(sb->reserved_blocks_count_lo);
}

uint64_t ext4_superblock_get_free_blocks_count(ext4_superblock_t *sb)
{
	return ((uint64_t)uint32_t_le2host(sb->free_blocks_count_hi) << 32) |
			uint32_t_le2host(sb->free_blocks_count_lo);
}

uint32_t ext4_superblock_get_free_inodes_count(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->free_inodes_count);
}

uint32_t ext4_superblock_get_first_data_block(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->first_data_block);
}

uint32_t ext4_superblock_get_log_block_size(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->log_block_size);
}

uint32_t ext4_superblock_get_block_size(ext4_superblock_t *sb)
{
	return 1024 << ext4_superblock_get_log_block_size(sb);
}


uint32_t ext4_superblock_get_blocks_per_group(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->blocks_per_group);
}

uint32_t ext4_superblock_get_inodes_per_group(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->inodes_per_group);
}

uint32_t ext4_superblock_get_mount_time(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->mount_time);
}

uint32_t ext4_superblock_get_write_time(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->write_time);
}

uint16_t ext4_superblock_get_mount_count(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->mount_count);
}

uint16_t ext4_superblock_get_max_mount_count(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->max_mount_count);
}

uint16_t ext4_superblock_get_magic(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->magic);
}

uint16_t ext4_superblock_get_state(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->state);
}

uint16_t ext4_superblock_get_errors(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->errors);
}


uint16_t ext4_superblock_get_minor_rev_level(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->minor_rev_level);
}

uint32_t ext4_superblock_get_last_check_time(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->last_check_time);
}

uint32_t ext4_superblock_get_check_interval(ext4_superblock_t *sb){
	return uint32_t_le2host(sb->check_interval);
}

uint32_t ext4_superblock_get_creator_os(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->creator_os);
}

uint32_t ext4_superblock_get_rev_level(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->rev_level);
}

uint16_t ext4_superblock_get_inode_size(ext4_superblock_t *sb)
{
	if (ext4_superblock_get_rev_level(sb) == 0) {
		return EXT4_REV0_INODE_SIZE;
	}
	return uint16_t_le2host(sb->inode_size);
}

uint16_t ext4_superblock_get_block_group_number(ext4_superblock_t *sb)
{
	return uint16_t_le2host(sb->block_group_number);
}

uint32_t ext4_superblock_get_features_compatible(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->features_compatible);
}

uint32_t ext4_superblock_get_features_incompatible(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->features_incompatible);
}

uint32_t ext4_superblock_get_features_read_only(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->features_read_only);
}


uint32_t* ext4_superblock_get_hash_seed(ext4_superblock_t *sb)
{
	return sb->hash_seed;
}

uint32_t ext4_superblock_get_flags(ext4_superblock_t *sb)
{
	return uint32_t_le2host(sb->flags);
}


/*
 * More complex superblock functions
 */

bool ext4_superblock_has_flag(ext4_superblock_t *sb, uint32_t flag)
{
	if (ext4_superblock_get_flags(sb) & flag) {
		return true;
	}
	return false;
}

int ext4_superblock_read_direct(service_id_t service_id,
    ext4_superblock_t **superblock)
{
	void *data;
	int rc;

	data = malloc(EXT4_SUPERBLOCK_SIZE);
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

int ext4_superblock_check_sanity(ext4_superblock_t *sb)
{
	if (ext4_superblock_get_magic(sb) != EXT4_SUPERBLOCK_MAGIC) {
		return ENOTSUP;
	}

	// TODO more checks !!!

	return EOK;
}

/**
 * @}
 */ 
