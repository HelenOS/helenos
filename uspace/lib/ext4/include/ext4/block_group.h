/*
 * Copyright (c) 2011 Martin Sucha
 * Copyright (c) 2012 Frantisek Princ
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

#ifndef LIBEXT4_BLOCK_GROUP_H_
#define LIBEXT4_BLOCK_GROUP_H_

#include <block.h>
#include <stdint.h>
#include "types.h"

extern uint64_t ext4_block_group_get_block_bitmap(ext4_block_group_t *,
    ext4_superblock_t *);
extern void ext4_block_group_set_block_bitmap(ext4_block_group_t *,
    ext4_superblock_t *, uint64_t);
extern uint64_t ext4_block_group_get_inode_bitmap(ext4_block_group_t *,
    ext4_superblock_t *);
extern void ext4_block_group_set_inode_bitmap(ext4_block_group_t *,
    ext4_superblock_t *, uint64_t);
extern uint64_t ext4_block_group_get_inode_table_first_block(
    ext4_block_group_t *, ext4_superblock_t *);
extern void ext4_block_group_set_inode_table_first_block(ext4_block_group_t *,
    ext4_superblock_t *, uint64_t);
extern uint32_t ext4_block_group_get_free_blocks_count(ext4_block_group_t *,
    ext4_superblock_t *);
extern void ext4_block_group_set_free_blocks_count(ext4_block_group_t *,
    ext4_superblock_t *, uint32_t);
extern uint32_t ext4_block_group_get_free_inodes_count(ext4_block_group_t *,
    ext4_superblock_t *);
extern void ext4_block_group_set_free_inodes_count(ext4_block_group_t *,
    ext4_superblock_t *, uint32_t);
extern void ext4_block_group_set_free_inodes_count(ext4_block_group_t *,
    ext4_superblock_t *, uint32_t);
extern uint32_t ext4_block_group_get_used_dirs_count(ext4_block_group_t *,
    ext4_superblock_t *);
extern void ext4_block_group_set_used_dirs_count(ext4_block_group_t *,
    ext4_superblock_t *, uint32_t);
extern uint16_t ext4_block_group_get_flags(ext4_block_group_t *);
extern void ext4_block_group_set_flags(ext4_block_group_t *, uint16_t);
extern uint32_t ext4_block_group_get_itable_unused(ext4_block_group_t *,
    ext4_superblock_t *);
extern void ext4_block_group_set_itable_unused(ext4_block_group_t *,
    ext4_superblock_t *, uint32_t);
extern uint16_t ext4_block_group_get_checksum(ext4_block_group_t *);
extern void ext4_block_group_set_checksum(ext4_block_group_t *, uint16_t);

extern bool ext4_block_group_has_flag(ext4_block_group_t *, uint32_t);
extern void ext4_block_group_set_flag(ext4_block_group_t *, uint32_t);
extern void ext4_block_group_clear_flag(ext4_block_group_t *, uint32_t);

#endif

/**
 * @}
 */
