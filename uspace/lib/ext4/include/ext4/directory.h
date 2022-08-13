/*
 * SPDX-FileCopyrightText: 2011 Martin Sucha
 * SPDX-FileCopyrightText: 2012 Frantisek Princ
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libext4
 * @{
 */

#ifndef LIBEXT4_DIRECTORY_H_
#define LIBEXT4_DIRECTORY_H_

#include "ext4/types.h"

extern uint32_t ext4_directory_entry_ll_get_inode(ext4_directory_entry_ll_t *);
extern void ext4_directory_entry_ll_set_inode(ext4_directory_entry_ll_t *,
    uint32_t);
extern uint16_t ext4_directory_entry_ll_get_entry_length(
    ext4_directory_entry_ll_t *);
extern void ext4_directory_entry_ll_set_entry_length(ext4_directory_entry_ll_t *,
    uint16_t);
extern uint16_t ext4_directory_entry_ll_get_name_length(ext4_superblock_t *,
    ext4_directory_entry_ll_t *);
extern void ext4_directory_entry_ll_set_name_length(ext4_superblock_t *,
    ext4_directory_entry_ll_t *, uint16_t);
extern uint8_t ext4_directory_entry_ll_get_inode_type(ext4_superblock_t *,
    ext4_directory_entry_ll_t *);
extern void ext4_directory_entry_ll_set_inode_type(ext4_superblock_t *,
    ext4_directory_entry_ll_t *, uint8_t);

extern errno_t ext4_directory_iterator_init(ext4_directory_iterator_t *,
    ext4_inode_ref_t *, aoff64_t);
extern errno_t ext4_directory_iterator_next(ext4_directory_iterator_t *);
extern errno_t ext4_directory_iterator_fini(ext4_directory_iterator_t *);

extern void ext4_directory_write_entry(ext4_superblock_t *,
    ext4_directory_entry_ll_t *, uint16_t, ext4_inode_ref_t *,
    const char *, size_t);
extern errno_t ext4_directory_add_entry(ext4_inode_ref_t *, const char *,
    ext4_inode_ref_t *);
extern errno_t ext4_directory_find_entry(ext4_directory_search_result_t *,
    ext4_inode_ref_t *, const char *);
extern errno_t ext4_directory_remove_entry(ext4_inode_ref_t *, const char *);

extern errno_t ext4_directory_try_insert_entry(ext4_superblock_t *, block_t *,
    ext4_inode_ref_t *, const char *, uint32_t);

extern errno_t ext4_directory_find_in_block(block_t *, ext4_superblock_t *, size_t,
    const char *, ext4_directory_entry_ll_t **);

extern errno_t ext4_directory_destroy_result(ext4_directory_search_result_t *);

#endif

/**
 * @}
 */
