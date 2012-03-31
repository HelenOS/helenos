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

#ifndef LIBEXT2_LIBEXT2_DIRECTORY_H_
#define LIBEXT2_LIBEXT2_DIRECTORY_H_

#include <libblock.h>
#include "libext2_filesystem.h"
#include "libext2_inode.h"

/**
 * Linked list directory entry structure
 */
typedef struct ext2_directory_entry_ll {
	uint32_t inode; // Inode for the entry
	uint16_t entry_length; // Distance to the next directory entry
	uint8_t name_length; // Lower 8 bits of name length
	union {
		uint8_t name_length_high; // Higher 8 bits of name length
		uint8_t inode_type; // Type of referenced inode (in rev >= 0.5)
	} __attribute__ ((packed));
	uint8_t name; // First byte of name, if present
} __attribute__ ((packed)) ext2_directory_entry_ll_t;

typedef struct ext2_directory_iterator {
	ext2_filesystem_t *fs;
	ext2_inode_ref_t *inode_ref;
	block_t *current_block;
	aoff64_t current_offset;
	ext2_directory_entry_ll_t *current;
} ext2_directory_iterator_t;


extern uint32_t	ext2_directory_entry_ll_get_inode(ext2_directory_entry_ll_t *);
extern uint16_t	ext2_directory_entry_ll_get_entry_length(
    ext2_directory_entry_ll_t *);
extern uint16_t	ext2_directory_entry_ll_get_name_length(
    ext2_superblock_t *, ext2_directory_entry_ll_t *);

extern int ext2_directory_iterator_init(ext2_directory_iterator_t *,
    ext2_filesystem_t *, ext2_inode_ref_t *, aoff64_t);
extern int ext2_directory_iterator_next(ext2_directory_iterator_t *);
extern int ext2_directory_iterator_seek(ext2_directory_iterator_t *, aoff64_t pos);
extern int ext2_directory_iterator_fini(ext2_directory_iterator_t *);

#endif

/** @}
 */
