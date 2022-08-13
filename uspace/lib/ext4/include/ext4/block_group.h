/*
 * SPDX-FileCopyrightText: 2011 Martin Sucha
 * SPDX-FileCopyrightText: 2012 Frantisek Princ
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
