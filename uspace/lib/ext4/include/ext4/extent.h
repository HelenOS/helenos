/*
 * SPDX-FileCopyrightText: 2012 Frantisek Princ
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libext4
 * @{
 */

#ifndef LIBEXT4_EXTENT_H_
#define LIBEXT4_EXTENT_H_

#include "ext4/types.h"

extern uint32_t ext4_extent_get_first_block(ext4_extent_t *);
extern void ext4_extent_set_first_block(ext4_extent_t *, uint32_t);
extern uint16_t ext4_extent_get_block_count(ext4_extent_t *);
extern void ext4_extent_set_block_count(ext4_extent_t *, uint16_t);
extern uint64_t ext4_extent_get_start(ext4_extent_t *);
extern void ext4_extent_set_start(ext4_extent_t *, uint64_t);

extern uint32_t ext4_extent_index_get_first_block(ext4_extent_index_t *);
extern void ext4_extent_index_set_first_block(ext4_extent_index_t *, uint32_t);
extern uint64_t ext4_extent_index_get_leaf(ext4_extent_index_t *);
extern void ext4_extent_index_set_leaf(ext4_extent_index_t *, uint64_t);

extern uint16_t ext4_extent_header_get_magic(ext4_extent_header_t *);
extern void ext4_extent_header_set_magic(ext4_extent_header_t *, uint16_t);
extern uint16_t ext4_extent_header_get_entries_count(ext4_extent_header_t *);
extern void ext4_extent_header_set_entries_count(ext4_extent_header_t *,
    uint16_t);
extern uint16_t ext4_extent_header_get_max_entries_count(ext4_extent_header_t *);
extern void ext4_extent_header_set_max_entries_count(ext4_extent_header_t *,
    uint16_t);
extern uint16_t ext4_extent_header_get_depth(ext4_extent_header_t *);
extern void ext4_extent_header_set_depth(ext4_extent_header_t *, uint16_t);
extern uint32_t ext4_extent_header_get_generation(ext4_extent_header_t *);
extern void ext4_extent_header_set_generation(ext4_extent_header_t *, uint32_t);

extern errno_t ext4_extent_find_block(ext4_inode_ref_t *, uint32_t, uint32_t *);
extern errno_t ext4_extent_release_blocks_from(ext4_inode_ref_t *, uint32_t);

extern errno_t ext4_extent_append_block(ext4_inode_ref_t *, uint32_t *, uint32_t *,
    bool);

#endif

/**
 * @}
 */
