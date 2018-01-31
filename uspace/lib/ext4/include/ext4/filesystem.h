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

#ifndef LIBEXT4_FILESYSTEM_H_
#define LIBEXT4_FILESYSTEM_H_

#include <block.h>
#include "ext4/fstypes.h"
#include "ext4/types.h"

extern errno_t ext4_filesystem_probe(service_id_t);
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
    ext4_block_group_ref_t *bg_ref);

#endif

/**
 * @}
 */
