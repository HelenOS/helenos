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

#ifndef LIBEXT2_LIBEXT2_INODE_H_
#define LIBEXT2_LIBEXT2_INODE_H_

#include <libblock.h>

typedef struct ext2_inode {
	uint16_t mode;
	uint16_t user_id;
	uint32_t size;
	uint8_t unused[16];
	uint16_t group_id;
	uint16_t usage_count; // Hard link count, when 0 the inode is to be freed
	uint32_t reserved_512_blocks; // Size of this inode in 512-byte blocks
	uint32_t flags;
	uint8_t unused2[4];
	uint32_t direct_blocks[12]; // Direct block ids stored in this inode
	uint32_t single_indirect_block;
	uint32_t double_indirect_block;
	uint32_t triple_indirect_block;
} ext2_inode_t;

typedef struct ext2_inode_ref {
	block_t *block; // Reference to a block containing this inode
	ext2_inode_t *inode;
} ext2_inode_ref_t;



#endif

/** @}
 */
