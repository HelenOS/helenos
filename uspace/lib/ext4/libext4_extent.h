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

#ifndef LIBEXT4_LIBEXT4_EXTENT_H_
#define LIBEXT4_LIBEXT4_EXTENT_H_

/*
 * This is the extent on-disk structure.
 * It's used at the bottom of the tree.
 */
typedef struct ext4_extent {
	uint32_t first_block; // First logical block extent covers
	uint16_t block_count; // Number of blocks covered by extent
	uint16_t start_hi;    // High 16 bits of physical block
	uint32_t start_lo;    // Low 32 bits of physical block
} ext4_extent_t;

/*
 * This is index on-disk structure.
 * It's used at all the levels except the bottom.
 */
typedef struct ext4_extent_idx {
	uint32_t block; // Index covers logical blocks from 'block'
	uint32_t leaf_lo; /* Pointer to the physical block of the next
	 	 	 	 	   * level. leaf or next index could be there */
	uint16_t leaf_hi;     /* high 16 bits of physical block */
	uint16_t padding;
} ext4_extent_idx_t;

/*
 * Each block (leaves and indexes), even inode-stored has header.
 */
typedef struct ext4_extent_header {
	uint16_t magic;
	uint16_t entries_count; // Number of valid entries
	uint16_t max_entries_count; // Capacity of store in entries
	uint16_t depth; // Has tree real underlying blocks?
	uint32_t generation; // generation of the tree
} ext4_extent_header_t;

#define EXT4_EXTENT_MAGIC	0xF30A

extern uint16_t ext4_extent_header_get_magic(ext4_extent_header_t *);
extern uint16_t ext4_extent_header_get_entries_count(ext4_extent_header_t *);
extern uint16_t ext4_extent_header_get_max_entries_count(ext4_extent_header_t *);
extern uint16_t ext4_extent_header_get_depth(ext4_extent_header_t *);
extern uint32_t ext4_extent_header_get_generation(ext4_extent_header_t *);

#endif

/**
 * @}
 */
