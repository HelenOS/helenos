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

#ifndef LIBEXT4_LIBEXT4_DIRECTORY_INDEX_H_
#define LIBEXT4_LIBEXT4_DIRECTORY_INDEX_H_

/* Structures for indexed directory */

typedef struct ext4_directory_dx_countlimit {
	uint16_t limit;
    uint16_t count;
} ext4_directory_dx_countlimit_t;

typedef struct ext4_directory_dx_dot_entry {
	uint32_t inode;
	uint16_t entry_length;
    uint8_t name_length;
    uint8_t inode_type;
    uint8_t name[4];
} ext4_directory_dx_dot_entry_t;

typedef struct ext4_directory_dx_root_info {
	uint32_t reserved_zero;
	uint8_t hash_version;
	uint8_t info_length;
	uint8_t indirect_levels;
	uint8_t unused_flags;
} ext4_directory_dx_root_info_t;

typedef struct ext4_directory_dx_entry {
	uint32_t hash;
	uint32_t block;
} ext4_directory_dx_entry_t;

typedef struct ext4_directory_dx_root {
		ext4_directory_dx_dot_entry_t dots[2];
		ext4_directory_dx_root_info_t info;
		ext4_directory_dx_entry_t entries[0];
} ext4_directory_dx_root_t;

typedef struct ext4_fake_directory_entry {
	uint32_t inode;
	uint16_t entry_length;
	uint8_t name_length;
	uint8_t inode_type;
} ext4_fake_directory_entry_t;

typedef struct ext4_directory_dx_node {
	ext4_fake_directory_entry_t fake;
	ext4_directory_dx_entry_t entries[0];
} ext4_directory_dx_node_t;


typedef struct ext4_directory_dx_block {
	block_t *block;
	ext4_directory_dx_entry_t *entries;
	ext4_directory_dx_entry_t *position;
} ext4_directory_dx_block_t;



#define EXT4_ERR_BAD_DX_DIR			(-75000)
#define EXT4_DIRECTORY_HTREE_EOF	(uint32_t)0x7fffffff


extern uint8_t ext4_directory_dx_root_info_get_hash_version(
		ext4_directory_dx_root_info_t *);
extern void ext4_directory_dx_root_info_set_hash_version(
		ext4_directory_dx_root_info_t *, uint8_t);
extern uint8_t ext4_directory_dx_root_info_get_info_length(
		ext4_directory_dx_root_info_t *);
extern void ext4_directory_dx_root_info_set_info_length(
		ext4_directory_dx_root_info_t *, uint8_t);
extern uint8_t ext4_directory_dx_root_info_get_indirect_levels(
		ext4_directory_dx_root_info_t *);
extern void ext4_directory_dx_root_info_set_indirect_levels(
		ext4_directory_dx_root_info_t *, uint8_t);

extern uint16_t ext4_directory_dx_countlimit_get_limit(
		ext4_directory_dx_countlimit_t *);
extern void ext4_directory_dx_countlimit_set_limit(
		ext4_directory_dx_countlimit_t *, uint16_t);
extern uint16_t ext4_directory_dx_countlimit_get_count(
		ext4_directory_dx_countlimit_t *);
extern void ext4_directory_dx_countlimit_set_count(
		ext4_directory_dx_countlimit_t *, uint16_t);

extern uint32_t ext4_directory_dx_entry_get_hash(ext4_directory_dx_entry_t *);
extern void ext4_directory_dx_entry_set_hash(ext4_directory_dx_entry_t *,
		uint32_t);
extern uint32_t ext4_directory_dx_entry_get_block(ext4_directory_dx_entry_t *);
void ext4_directory_dx_entry_set_block(ext4_directory_dx_entry_t *, uint32_t);

/*********************************************************************************/

extern int ext4_directory_dx_find_entry(ext4_directory_iterator_t *,
		ext4_filesystem_t *, ext4_inode_ref_t *, size_t, const char *);
extern int ext4_directory_dx_add_entry(ext4_filesystem_t *,
		ext4_inode_ref_t *, ext4_inode_ref_t *, size_t, const char *);

#endif

/**
 * @}
 */
