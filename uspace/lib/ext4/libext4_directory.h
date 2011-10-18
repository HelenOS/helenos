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

#ifndef LIBEXT4_LIBEXT4_DIRECTORY_H_
#define LIBEXT4_LIBEXT4_DIRECTORY_H_

#include "libext4_filesystem.h"
#include "libext4_inode.h"

#define EXT4_DIRECTORY_FILENAME_LEN	255

/**
 * Linked list directory entry structure
 */
typedef struct ext4_directory_entry_ll {
	uint32_t inode; // Inode for the entry
	uint16_t entry_length; // Distance to the next directory entry
	uint8_t name_length; // Lower 8 bits of name length
	union {
		uint8_t name_length_high; // Higher 8 bits of name length
		uint8_t inode_type; // Type of referenced inode (in rev >= 0.5)
	} __attribute__ ((packed));
	uint8_t name[EXT4_DIRECTORY_FILENAME_LEN]; // Entry name
} __attribute__ ((packed)) ext4_directory_entry_ll_t;

typedef struct ext4_directory_iterator {
	ext4_filesystem_t *fs;
	ext4_inode_ref_t *inode_ref;
	block_t *current_block;
	aoff64_t current_offset;
	ext4_directory_entry_ll_t *current;
} ext4_directory_iterator_t;


/* Structures for indexed directory */

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
		// TODO insert root info items instead of special datatype
		ext4_directory_dx_root_info_t info;
		ext4_directory_dx_entry_t *entries;
} ext4_directory_dx_root_t;


#define EXT4_DIRECTORY_HTREE_EOF  0x7fffffff


extern uint32_t	ext4_directory_entry_ll_get_inode(ext4_directory_entry_ll_t *);
extern uint16_t	ext4_directory_entry_ll_get_entry_length(
    ext4_directory_entry_ll_t *);
extern uint16_t	ext4_directory_entry_ll_get_name_length(
    ext4_superblock_t *, ext4_directory_entry_ll_t *);

extern int ext4_directory_iterator_init(ext4_directory_iterator_t *,
		ext4_filesystem_t *, ext4_inode_ref_t *, aoff64_t);
extern int ext4_directory_iterator_next(ext4_directory_iterator_t *);
extern int ext4_directory_iterator_seek(ext4_directory_iterator_t *, aoff64_t pos);
extern int ext4_directory_iterator_fini(ext4_directory_iterator_t *);

#endif

/**
 * @}
 */
