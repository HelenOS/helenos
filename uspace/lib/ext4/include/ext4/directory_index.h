/*
 * SPDX-FileCopyrightText: 2012 Frantisek Princ
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libext4
 * @{
 */

#ifndef LIBEXT4_DIRECTORY_INDEX_H_
#define LIBEXT4_DIRECTORY_INDEX_H_

#include "ext4/types.h"

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
extern void ext4_directory_dx_entry_set_block(ext4_directory_dx_entry_t *,
    uint32_t);

extern errno_t ext4_directory_dx_init(ext4_inode_ref_t *);
extern errno_t ext4_directory_dx_find_entry(ext4_directory_search_result_t *,
    ext4_inode_ref_t *, size_t, const char *);
extern errno_t ext4_directory_dx_add_entry(ext4_inode_ref_t *, ext4_inode_ref_t *,
    const char *);

#endif

/**
 * @}
 */
