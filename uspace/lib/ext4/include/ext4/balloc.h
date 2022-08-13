/*
 * SPDX-FileCopyrightText: 2012 Frantisek Princ
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libext4
 * @{
 */

#ifndef LIBEXT4_BALLOC_H_
#define LIBEXT4_BALLOC_H_

#include <stdint.h>
#include "types.h"

extern errno_t ext4_balloc_free_block(ext4_inode_ref_t *, uint32_t);
extern errno_t ext4_balloc_free_blocks(ext4_inode_ref_t *, uint32_t, uint32_t);
extern uint32_t ext4_balloc_get_first_data_block_in_group(ext4_superblock_t *,
    ext4_block_group_ref_t *);
extern errno_t ext4_balloc_alloc_block(ext4_inode_ref_t *, uint32_t *);
extern errno_t ext4_balloc_try_alloc_block(ext4_inode_ref_t *, uint32_t, bool *);

#endif

/**
 * @}
 */
