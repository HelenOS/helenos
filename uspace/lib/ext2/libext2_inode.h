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
#include "libext2_superblock.h"

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
	uint32_t indirect_blocks[3];
	uint32_t version;
	uint32_t file_acl;
	union {
		uint32_t dir_acl;
		uint32_t size_high; // For regular files in version >= 1
	} __attribute__ ((packed));
	uint8_t unused3[6];
	uint16_t mode_high; // Hurd only
	uint16_t user_id_high; // Linux/Hurd only
	uint16_t group_id_high; // Linux/Hurd only
} __attribute__ ((packed)) ext2_inode_t;

#define EXT2_INODE_MODE_FIFO		0x1000
#define EXT2_INODE_MODE_CHARDEV		0x2000
#define EXT2_INODE_MODE_DIRECTORY	0x4000
#define EXT2_INODE_MODE_BLOCKDEV	0x6000
#define EXT2_INODE_MODE_FILE		0x8000
#define EXT2_INODE_MODE_SOFTLINK	0xA000
#define EXT2_INODE_MODE_SOCKET		0xC000
#define EXT2_INODE_MODE_ACCESS_MASK	0x0FFF
#define EXT2_INODE_MODE_TYPE_MASK	0xF000
#define EXT2_INODE_DIRECT_BLOCKS	12

#define EXT2_INODE_ROOT_INDEX		2

typedef struct ext2_inode_ref {
	block_t *block; // Reference to a block containing this inode
	ext2_inode_t *inode;
	uint32_t index; // Index number of this inode
} ext2_inode_ref_t;

extern uint32_t ext2_inode_get_mode(ext2_superblock_t *, ext2_inode_t *);
extern bool ext2_inode_is_type(ext2_superblock_t *, ext2_inode_t *, uint32_t);
extern uint32_t ext2_inode_get_user_id(ext2_superblock_t *, ext2_inode_t *);
extern uint64_t ext2_inode_get_size(ext2_superblock_t *, ext2_inode_t *);
extern uint32_t ext2_inode_get_group_id(ext2_superblock_t *, ext2_inode_t *);
extern uint16_t ext2_inode_get_usage_count(ext2_inode_t *);
extern uint32_t ext2_inode_get_reserved_512_blocks(ext2_inode_t *);
extern uint32_t ext2_inode_get_reserved_blocks(ext2_superblock_t *, 
    ext2_inode_t *);
extern uint32_t ext2_inode_get_flags(ext2_inode_t *);
extern uint32_t ext2_inode_get_direct_block(ext2_inode_t *, uint8_t);
extern uint32_t ext2_inode_get_indirect_block(ext2_inode_t *, uint8_t level);



#endif

/** @}
 */
