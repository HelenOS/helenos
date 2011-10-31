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

#ifndef LIBEXT4_LIBEXT4_BLOCK_GROUP_H_
#define LIBEXT4_LIBEXT4_BLOCK_GROUP_H_

#include <libblock.h>
#include <sys/types.h>
#include "libext4_block_group.h"
/*
 * Structure of a blocks group descriptor
 */
typedef struct ext4_block_group {
	uint32_t block_bitmap_lo; // Blocks bitmap block
	uint32_t inode_bitmap_lo; // Inodes bitmap block
	uint32_t inode_table_first_block_lo; // Inodes table block
	uint16_t free_blocks_count_lo; // Free blocks count
	uint16_t free_inodes_count_lo; // Free inodes count
	uint16_t used_dirs_count_lo; // Directories count
	uint16_t flags; // EXT4_BG_flags (INODE_UNINIT, etc)
	uint32_t reserved[2]; // Likely block/inode bitmap checksum
	uint16_t itable_unused_lo; // Unused inodes count
	uint16_t checksum; // crc16(sb_uuid+group+desc)
	/* -------------- */
	uint32_t block_bitmap_hi; // Blocks bitmap block MSB
	uint32_t inode_bitmap_hi; // Inodes bitmap block MSB
	uint32_t inode_table_first_block_hi; // Inodes table block MSB
	uint16_t free_blocks_count_hi; // Free blocks count MSB
	uint16_t free_inodes_count_hi; // Free inodes count MSB
	uint16_t used_dirs_count_hi; // Directories count MSB
	uint16_t itable_unused_hi;  // Unused inodes count MSB
	uint32_t reserved2[3]; // Padding
} ext4_block_group_t;

typedef struct ext4_block_group_ref {
	block_t *block; // Reference to a block containing this block group descr
	ext4_block_group_t *block_group;
} ext4_block_group_ref_t;

#define EXT4_BLOCK_MIN_GROUP_DESCRIPTOR_SIZE 32

extern uint64_t ext4_block_group_get_block_bitmap(ext4_block_group_t *);
extern uint64_t ext4_block_group_get_inode_bitmap(ext4_block_group_t *);
extern uint64_t ext4_block_group_get_inode_table_first_block(ext4_block_group_t *);
extern uint32_t ext4_block_group_get_free_blocks_count(ext4_block_group_t *);
extern uint32_t ext4_block_group_get_free_inodes_count(ext4_block_group_t *);
extern uint32_t ext4_block_group_get_used_dirs_count(ext4_block_group_t *);
extern uint16_t ext4_block_group_get_flags(ext4_block_group_t *);
extern uint32_t ext4_block_group_get_itable_unused(ext4_block_group_t *);
extern uint16_t ext4_block_group_get_checksum(ext4_block_group_t *);

#endif

/**
 * @}
 */
