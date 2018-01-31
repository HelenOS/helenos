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

#ifndef LIBEXT4_SUPERBLOCK_H_
#define LIBEXT4_SUPERBLOCK_H_

#include <block.h>
#include <stdint.h>
#include "ext4/types.h"

extern uint32_t ext4_superblock_get_inodes_count(ext4_superblock_t *);
extern void ext4_superblock_set_inodes_count(ext4_superblock_t *, uint32_t);
extern uint64_t ext4_superblock_get_blocks_count(ext4_superblock_t *);
extern void ext4_superblock_set_blocks_count(ext4_superblock_t *, uint64_t);
extern uint64_t ext4_superblock_get_reserved_blocks_count(ext4_superblock_t *);
extern void ext4_superblock_set_reserved_blocks_count(ext4_superblock_t *,
    uint64_t);
extern uint64_t ext4_superblock_get_free_blocks_count(ext4_superblock_t *);
extern void ext4_superblock_set_free_blocks_count(ext4_superblock_t *,
    uint64_t);
extern uint32_t ext4_superblock_get_free_inodes_count(ext4_superblock_t *);
extern void ext4_superblock_set_free_inodes_count(ext4_superblock_t *,
    uint32_t);
extern uint32_t ext4_superblock_get_first_data_block(ext4_superblock_t *);
extern void ext4_superblock_set_first_data_block(ext4_superblock_t *, uint32_t);
extern uint32_t ext4_superblock_get_log_block_size(ext4_superblock_t *);
extern void ext4_superblock_set_log_block_size(ext4_superblock_t *, uint32_t);
extern uint32_t ext4_superblock_get_block_size(ext4_superblock_t *);
extern void ext4_superblock_set_block_size(ext4_superblock_t *, uint32_t);
extern uint32_t ext4_superblock_get_log_frag_size(ext4_superblock_t *);
extern void ext4_superblock_set_log_frag_size(ext4_superblock_t *, uint32_t);
extern uint32_t ext4_superblock_get_frag_size(ext4_superblock_t *);
extern void ext4_superblock_set_frag_size(ext4_superblock_t *, uint32_t);
extern uint32_t ext4_superblock_get_blocks_per_group(ext4_superblock_t *);
extern void ext4_superblock_set_blocks_per_group(ext4_superblock_t *, uint32_t);
extern uint32_t ext4_superblock_get_frags_per_group(ext4_superblock_t *);
extern void ext4_superblock_set_frags_per_group(ext4_superblock_t *, uint32_t);
extern uint32_t ext4_superblock_get_inodes_per_group(ext4_superblock_t *);
extern void ext4_superblock_set_inodes_per_group(ext4_superblock_t *, uint32_t);
extern uint32_t ext4_superblock_get_mount_time(ext4_superblock_t *);
extern void ext4_superblock_set_mount_time(ext4_superblock_t *, uint32_t);
extern uint32_t ext4_superblock_get_write_time(ext4_superblock_t *);
extern void ext4_superblock_set_write_time(ext4_superblock_t *, uint32_t);
extern uint16_t ext4_superblock_get_mount_count(ext4_superblock_t *);
extern void ext4_superblock_set_mount_count(ext4_superblock_t *, uint16_t);
extern uint16_t ext4_superblock_get_max_mount_count(ext4_superblock_t *);
extern void ext4_superblock_set_max_mount_count(ext4_superblock_t *, uint16_t);
extern uint16_t ext4_superblock_get_magic(ext4_superblock_t *);
extern void ext4_superblock_set_magic(ext4_superblock_t *sb, uint16_t magic);
extern uint16_t ext4_superblock_get_state(ext4_superblock_t *);
extern void ext4_superblock_set_state(ext4_superblock_t *, uint16_t);
extern uint16_t ext4_superblock_get_errors(ext4_superblock_t *);
extern void ext4_superblock_set_errors(ext4_superblock_t *, uint16_t);
extern uint16_t ext4_superblock_get_minor_rev_level(ext4_superblock_t *);
extern void ext4_superblock_set_minor_rev_level(ext4_superblock_t *, uint16_t);
extern uint32_t ext4_superblock_get_last_check_time(ext4_superblock_t *);
extern void ext4_superblock_set_last_check_time(ext4_superblock_t *, uint32_t);
extern uint32_t ext4_superblock_get_check_interval(ext4_superblock_t *);
extern void ext4_superblock_set_check_interval(ext4_superblock_t *, uint32_t);
extern uint32_t ext4_superblock_get_creator_os(ext4_superblock_t *);
extern void ext4_superblock_set_creator_os(ext4_superblock_t *, uint32_t);
extern uint32_t ext4_superblock_get_rev_level(ext4_superblock_t *);
extern void ext4_superblock_set_rev_level(ext4_superblock_t *, uint32_t);
extern uint16_t ext4_superblock_get_def_resuid(ext4_superblock_t *);
extern void ext4_superblock_set_def_resuid(ext4_superblock_t *, uint16_t);
extern uint16_t ext4_superblock_get_def_resgid(ext4_superblock_t *);
extern void ext4_superblock_set_def_resgid(ext4_superblock_t *, uint16_t);
extern uint32_t ext4_superblock_get_first_inode(ext4_superblock_t *);
extern void ext4_superblock_set_first_inode(ext4_superblock_t *, uint32_t);
extern uint16_t ext4_superblock_get_inode_size(ext4_superblock_t *);
extern void ext4_superblock_set_inode_size(ext4_superblock_t *, uint16_t);
extern uint16_t ext4_superblock_get_block_group_index(ext4_superblock_t *);
extern void ext4_superblock_set_block_group_index(ext4_superblock_t *,
    uint16_t);
extern uint32_t ext4_superblock_get_features_compatible(ext4_superblock_t *);
extern void ext4_superblock_set_features_compatible(ext4_superblock_t *,
    uint32_t);
extern uint32_t ext4_superblock_get_features_incompatible(ext4_superblock_t *);
extern void ext4_superblock_set_features_incompatible(ext4_superblock_t *,
    uint32_t);
extern uint32_t ext4_superblock_get_features_read_only(ext4_superblock_t *);
extern void ext4_superblock_set_features_read_only(ext4_superblock_t *,
    uint32_t);

extern const uint8_t *ext4_superblock_get_uuid(ext4_superblock_t *);
extern void ext4_superblock_set_uuid(ext4_superblock_t *, const uint8_t *);
extern const char *ext4_superblock_get_volume_name(ext4_superblock_t *);
extern void ext4_superblock_set_volume_name(ext4_superblock_t *, const char *);
extern const char *ext4_superblock_get_last_mounted(ext4_superblock_t *);
extern void ext4_superblock_set_last_mounted(ext4_superblock_t *, const char *);

extern uint32_t ext4_superblock_get_last_orphan(ext4_superblock_t *);
extern void ext4_superblock_set_last_orphan(ext4_superblock_t *, uint32_t);
extern const uint32_t *ext4_superblock_get_hash_seed(ext4_superblock_t *);
extern void ext4_superblock_set_hash_seed(ext4_superblock_t *,
    const uint32_t *);
extern uint8_t ext4_superblock_get_default_hash_version(ext4_superblock_t *);
extern void ext4_superblock_set_default_hash_version(ext4_superblock_t *,
    uint8_t);

extern uint16_t ext4_superblock_get_desc_size(ext4_superblock_t *);
extern void ext4_superblock_set_desc_size(ext4_superblock_t *, uint16_t);

extern uint32_t ext4_superblock_get_flags(ext4_superblock_t *);
extern void ext4_superblock_set_flags(ext4_superblock_t *, uint32_t);

extern void ext4_superblock_get_backup_groups_sparse2(ext4_superblock_t *sb,
    uint32_t *g1, uint32_t *g2);
extern void ext4_superblock_set_backup_groups_sparse2(ext4_superblock_t *sb,
    uint32_t g1, uint32_t g2);

extern uint32_t ext4_superblock_get_reserved_gdt_blocks(ext4_superblock_t *sb);
extern void ext4_superblock_set_reserved_gdt_blocks(ext4_superblock_t *sb,
    uint32_t n);

/* More complex superblock functions */
extern bool ext4_superblock_has_flag(ext4_superblock_t *, uint32_t);
extern bool ext4_superblock_has_feature_compatible(ext4_superblock_t *,
    uint32_t);
extern bool ext4_superblock_has_feature_incompatible(ext4_superblock_t *,
    uint32_t);
extern bool ext4_superblock_has_feature_read_only(ext4_superblock_t *,
    uint32_t);
extern errno_t ext4_superblock_read_direct(service_id_t, ext4_superblock_t **);
extern errno_t ext4_superblock_write_direct(service_id_t, ext4_superblock_t *);
extern void ext4_superblock_release(ext4_superblock_t *);
extern errno_t ext4_superblock_check_sanity(ext4_superblock_t *);

extern uint32_t ext4_superblock_get_block_group_count(ext4_superblock_t *);
extern uint32_t ext4_superblock_get_blocks_in_group(ext4_superblock_t *,
    uint32_t);
extern uint32_t ext4_superblock_get_inodes_in_group(ext4_superblock_t *,
    uint32_t);

#endif

/**
 * @}
 */
