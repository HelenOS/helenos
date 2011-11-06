/*
 * Copyright (c) 2011 Martin Sucha
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

/** @addtogroup libext2
 * @{
 */
/**
 * @file
 */

#ifndef LIBEXT2_LIBEXT2_BLOCK_GROUP_H_
#define LIBEXT2_LIBEXT2_BLOCK_GROUP_H_

#include <libblock.h>

typedef struct ext2_block_group {
	uint32_t block_bitmap_block; // Block ID for block bitmap
	uint32_t inode_bitmap_block; // Block ID for inode bitmap
	uint32_t inode_table_first_block; // Block ID of first block of inode table 
	uint16_t free_block_count; // Count of free blocks
	uint16_t free_inode_count; // Count of free inodes
	uint16_t directory_inode_count; // Number of inodes allocated to directories
} ext2_block_group_t;

typedef struct ext2_block_group_ref {
	block_t *block; // Reference to a block containing this block group descr
	ext2_block_group_t *block_group;
} ext2_block_group_ref_t;

#define EXT2_BLOCK_GROUP_DESCRIPTOR_SIZE 32

extern uint32_t	ext2_block_group_get_block_bitmap_block(ext2_block_group_t *);
extern uint32_t	ext2_block_group_get_inode_bitmap_block(ext2_block_group_t *);
extern uint32_t	ext2_block_group_get_inode_table_first_block(ext2_block_group_t *);
extern uint16_t	ext2_block_group_get_free_block_count(ext2_block_group_t *);
extern uint16_t	ext2_block_group_get_free_inode_count(ext2_block_group_t *);
extern uint16_t	ext2_block_group_get_directory_inode_count(ext2_block_group_t *);

extern void	ext2_block_group_set_free_block_count(ext2_block_group_t *, uint16_t);

#endif

/** @}
 */
