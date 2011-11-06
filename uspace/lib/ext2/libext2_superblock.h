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

#ifndef LIBEXT2_LIBEXT2_SUPERBLOCK_H_
#define LIBEXT2_LIBEXT2_SUPERBLOCK_H_

#include <libblock.h>

typedef struct ext2_superblock {
	uint32_t	total_inode_count; // Total number of inodes
	uint32_t	total_block_count; // Total number of blocks
	uint32_t	reserved_block_count; // Total number of reserved blocks
	uint32_t	free_block_count; // Total number of free blocks
	uint32_t	free_inode_count; // Total number of free inodes
	uint32_t	first_block; // Block containing the superblock (either 0 or 1)
	uint32_t	block_size_log2; // log_2(block_size)
	int32_t		fragment_size_log2; // log_2(fragment size)
	uint32_t	blocks_per_group; // Number of blocks in one block group
	uint32_t	fragments_per_group; // Number of fragments per block group
	uint32_t	inodes_per_group; // Number of inodes per block group
	uint8_t		unused2[12];
	uint16_t	magic; // Magic value
	uint16_t	state; // State (mounted/unmounted)
	uint16_t	error_behavior; // What to do when errors are encountered
	uint16_t	rev_minor; // Minor revision level
	uint8_t		unused3[8];
	uint32_t	os; // OS that created the filesystem
	uint32_t	rev_major; // Major revision level
	uint8_t		unused4[4];
	
	// Following is for ext2 revision 1 only
	uint32_t	first_inode;
	uint16_t	inode_size;
	uint16_t	unused5;
	uint32_t	features_compatible;
	uint32_t	features_incompatible;
	uint32_t	features_read_only;
	uint8_t		uuid[16]; // UUID TODO: Create a library for UUIDs
	uint8_t		volume_name[16];

// TODO: add __attribute__((aligned(...)) for better performance?
//       (it is necessary to ensure the superblock is correctly aligned then
//        though)
} __attribute__ ((packed)) ext2_superblock_t;

#define EXT2_SUPERBLOCK_MAGIC		0xEF53
#define EXT2_SUPERBLOCK_SIZE		1024
#define EXT2_SUPERBLOCK_OFFSET		1024
#define EXT2_SUPERBLOCK_LAST_BYTE	(EXT2_SUPERBLOCK_OFFSET + \
									 EXT2_SUPERBLOCK_SIZE -1)
#define EXT2_SUPERBLOCK_OS_LINUX	0
#define EXT2_SUPERBLOCK_OS_HURD		1


extern uint16_t	ext2_superblock_get_magic(ext2_superblock_t *);
extern uint32_t	ext2_superblock_get_first_block(ext2_superblock_t *);
extern uint32_t	ext2_superblock_get_block_size_log2(ext2_superblock_t *);
extern uint32_t	ext2_superblock_get_block_size(ext2_superblock_t *);
extern int32_t	ext2_superblock_get_fragment_size_log2(ext2_superblock_t *);
extern uint32_t	ext2_superblock_get_fragment_size(ext2_superblock_t *);
extern uint32_t	ext2_superblock_get_blocks_per_group(ext2_superblock_t *);
extern uint32_t	ext2_superblock_get_fragments_per_group(ext2_superblock_t *);
extern uint16_t	ext2_superblock_get_state(ext2_superblock_t *);
extern uint16_t	ext2_superblock_get_rev_minor(ext2_superblock_t *);
extern uint32_t	ext2_superblock_get_rev_major(ext2_superblock_t *);
extern uint32_t	ext2_superblock_get_os(ext2_superblock_t *);
extern uint32_t	ext2_superblock_get_first_inode(ext2_superblock_t *);
extern uint16_t	ext2_superblock_get_inode_size(ext2_superblock_t *);
extern uint32_t	ext2_superblock_get_total_inode_count(ext2_superblock_t *);
extern uint32_t	ext2_superblock_get_total_block_count(ext2_superblock_t *);
extern uint32_t	ext2_superblock_get_reserved_block_count(ext2_superblock_t *);
extern uint32_t	ext2_superblock_get_free_block_count(ext2_superblock_t *);
extern uint32_t	ext2_superblock_get_free_inode_count(ext2_superblock_t *);
extern uint32_t	ext2_superblock_get_block_group_count(ext2_superblock_t *);
extern uint32_t	ext2_superblock_get_inodes_per_group(ext2_superblock_t *);
extern uint32_t	ext2_superblock_get_features_compatible(ext2_superblock_t *);
extern uint32_t	ext2_superblock_get_features_incompatible(ext2_superblock_t *);
extern uint32_t	ext2_superblock_get_features_read_only(ext2_superblock_t *);

extern int ext2_superblock_read_direct(service_id_t, ext2_superblock_t **);
extern int ext2_superblock_check_sanity(ext2_superblock_t *);

#endif

/** @}
 */
