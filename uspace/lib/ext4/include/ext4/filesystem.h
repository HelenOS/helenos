/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 * SPDX-FileCopyrightText: 2011 Martin Sucha
 * SPDX-FileCopyrightText: 2012 Frantisek Princ
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libext4
 * @{
 */

#ifndef LIBEXT4_FILESYSTEM_H_
#define LIBEXT4_FILESYSTEM_H_

#include <block.h>
#include "ext4/cfg.h"
#include "ext4/fstypes.h"
#include "ext4/types.h"

extern errno_t ext4_filesystem_probe(service_id_t, ext4_fs_probe_info_t *);
extern errno_t ext4_filesystem_create(ext4_cfg_t *, service_id_t);
extern errno_t ext4_filesystem_open(ext4_instance_t *, service_id_t,
    enum cache_mode, aoff64_t *, ext4_filesystem_t **);
extern errno_t ext4_filesystem_close(ext4_filesystem_t *);
extern uint32_t ext4_filesystem_blockaddr2_index_in_group(ext4_superblock_t *,
    uint32_t);
extern uint32_t ext4_filesystem_index_in_group2blockaddr(ext4_superblock_t *,
    uint32_t, uint32_t);
extern uint32_t ext4_filesystem_blockaddr2group(ext4_superblock_t *, uint64_t);
extern errno_t ext4_filesystem_get_block_group_ref(ext4_filesystem_t *, uint32_t,
    ext4_block_group_ref_t **);
extern errno_t ext4_filesystem_put_block_group_ref(ext4_block_group_ref_t *);
extern errno_t ext4_filesystem_get_inode_ref(ext4_filesystem_t *, uint32_t,
    ext4_inode_ref_t **);
extern errno_t ext4_filesystem_put_inode_ref(ext4_inode_ref_t *);
extern errno_t ext4_filesystem_alloc_inode(ext4_filesystem_t *, ext4_inode_ref_t **,
    int);
extern errno_t ext4_filesystem_free_inode(ext4_inode_ref_t *);
extern errno_t ext4_filesystem_truncate_inode(ext4_inode_ref_t *, aoff64_t);
extern errno_t ext4_filesystem_get_inode_data_block_index(ext4_inode_ref_t *,
    aoff64_t iblock, uint32_t *);
extern errno_t ext4_filesystem_set_inode_data_block_index(ext4_inode_ref_t *,
    aoff64_t, uint32_t);
extern errno_t ext4_filesystem_release_inode_block(ext4_inode_ref_t *, uint32_t);
extern errno_t ext4_filesystem_append_inode_block(ext4_inode_ref_t *, uint32_t *,
    uint32_t *);
uint32_t ext4_filesystem_bg_get_backup_blocks(ext4_block_group_ref_t *bg);
uint32_t ext4_filesystem_bg_get_itable_size(ext4_superblock_t *sb,
    uint32_t);

#endif

/**
 * @}
 */
