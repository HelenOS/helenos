/*
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

#ifndef LIBEXT4_LIBEXT4_SUPERBLOCK_H_
#define LIBEXT4_LIBEXT4_SUPERBLOCK_H_

#include <libblock.h>
#include <sys/types.h>

#include "libext4_types.h"

extern uint32_t ext4_superblock_get_inodes_count(ext4_superblock_t *);
extern void ext4_superblock_set_inodes_count(ext4_superblock_t *, uint32_t);
extern uint64_t ext4_superblock_get_blocks_count(ext4_superblock_t *);
extern void ext4_superblock_set_blocks_count(ext4_superblock_t *, uint64_t);
extern uint64_t ext4_superblock_get_reserved_blocks_count(ext4_superblock_t *);
extern void ext4_superblock_set_reserved_blocks_count(ext4_superblock_t *, uint64_t);
extern uint64_t ext4_superblock_get_free_blocks_count(ext4_superblock_t *);
extern void ext4_superblock_set_free_blocks_count(ext4_superblock_t *, uint64_t);
extern uint32_t ext4_superblock_get_free_inodes_count(ext4_superblock_t *);
extern void ext4_superblock_set_free_inodes_count(ext4_superblock_t *, uint32_t);
extern uint32_t ext4_superblock_get_first_data_block(ext4_superblock_t *);
extern void ext4_superblock_set_first_data_block(ext4_superblock_t *, uint32_t);
extern uint32_t ext4_superblock_get_log_block_size(ext4_superblock_t *);
extern void ext4_superblock_set_log_block_size(ext4_superblock_t *, uint32_t);
extern uint32_t ext4_superblock_get_block_size(ext4_superblock_t *);
extern void ext4_superblock_set_block_size(ext4_superblock_t *, uint32_t);
extern uint32_t ext4_superblock_get_blocks_per_group(ext4_superblock_t *);
extern void ext4_superblock_set_blocks_per_group(ext4_superblock_t *, uint32_t);
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
extern void ext4_superblock_set_block_group_index(ext4_superblock_t *, uint16_t);
extern uint32_t	ext4_superblock_get_features_compatible(ext4_superblock_t *);
extern void	ext4_superblock_set_features_compatible(ext4_superblock_t *, uint32_t);
extern uint32_t	ext4_superblock_get_features_incompatible(ext4_superblock_t *);
extern void	ext4_superblock_set_features_incompatible(ext4_superblock_t *, uint32_t);
extern uint32_t	ext4_superblock_get_features_read_only(ext4_superblock_t *);
extern void	ext4_superblock_set_features_read_only(ext4_superblock_t *, uint32_t);

extern const uint8_t * ext4_superblock_get_uuid(ext4_superblock_t *);
extern void ext4_superblock_set_uuid(ext4_superblock_t *, const uint8_t *);
extern const char * ext4_superblock_get_volume_name(ext4_superblock_t *);
extern void ext4_superblock_set_volume_name(ext4_superblock_t *, const char *);
extern const char * ext4_superblock_get_last_mounted(ext4_superblock_t *);
extern void ext4_superblock_set_last_mounted(ext4_superblock_t *, const char *);

/*
uint32_t s_algorithm_usage_bitmap; // For compression
uint8_t s_prealloc_blocks; // Number of blocks to try to preallocate
uint8_t s_prealloc_dir_blocks; // Number to preallocate for dirs
uint16_t s_reserved_gdt_blocks; // Per group desc for online growth
uint8_t s_journal_uuid[16]; // UUID of journal superblock
uint32_t s_journal_inum; // Inode number of journal file
uint32_t s_journal_dev; // Device number of journal file
*/
extern uint32_t ext4_superblock_get_last_orphan(ext4_superblock_t *);
extern void ext4_superblock_set_last_orphan(ext4_superblock_t *, uint32_t);
extern const uint32_t * ext4_superblock_get_hash_seed(ext4_superblock_t *);
extern void ext4_superblock_set_hash_seed(ext4_superblock_t *, const uint32_t *);
extern uint8_t ext4_superblock_get_default_hash_version(ext4_superblock_t *);
extern void ext4_superblock_set_default_hash_version(ext4_superblock_t *, uint8_t);
/*
uint8_t s_jnl_backup_type;
*/

extern uint16_t ext4_superblock_get_desc_size(ext4_superblock_t *);
extern void ext4_superblock_set_desc_size(ext4_superblock_t *, uint16_t);

/*
uint32_t s_default_mount_opts; // Default mount options
uint32_t s_first_meta_bg; // First metablock block group
uint32_t s_mkfs_time; // When the filesystem was created
uint32_t s_jnl_blocks[17]; // Backup of the journal inode
uint16_t s_min_extra_isize; // All inodes have at least # bytes
uint16_t s_want_extra_isize; // New inodes should reserve # bytes
*/
extern uint32_t ext4_superblock_get_flags(ext4_superblock_t *);
extern void ext4_superblock_set_flags(ext4_superblock_t *, uint32_t);
/*
uint16_t s_raid_stride; // RAID stride
uint16_t s_mmp_interval; // # seconds to wait in MMP checking
uint64_t s_mmp_block; // Block for multi-mount protection
uint32_t s_raid_stripe_width; // blocks on all data disks (N*stride)
uint8_t s_log_groups_per_flex; // FLEX_BG group size
uint8_t s_reserved_char_pad;
uint16_t s_reserved_pad;
uint64_t s_kbytes_written; // Number of lifetime kilobytes written
uint32_t s_snapshot_inum; // Inode number of active snapshot
uint32_t s_snapshot_id; // Sequential ID of active snapshot
uint64_t s_snapshot_r_blocks_count; // reserved blocks for active snapshot's future use
uint32_t s_snapshot_list; // inode number of the head of the on-disk snapshot list
uint32_t s_error_count; // number of fs errors
uint32_t s_first_error_time; // First time an error happened
uint32_t s_first_error_ino; // Inode involved in first error
uint64_t s_first_error_block; // block involved of first error
uint8_t s_first_error_func[32]; // Function where the error happened
uint32_t s_first_error_line; // Line number where error happened
uint32_t s_last_error_time; // Most recent time of an error
uint32_t s_last_error_ino; // Inode involved in last error
uint32_t s_last_error_line; // Line number where error happened
uint64_t s_last_error_block;     // block involved of last error
uint8_t s_last_error_func[32];  // function where the error happened
uint8_t s_mount_opts[64];
*/

/* More complex superblock functions */
extern bool ext4_superblock_has_flag(ext4_superblock_t *, uint32_t);
extern bool ext4_superblock_has_feature_compatible(ext4_superblock_t *, uint32_t);
extern bool ext4_superblock_has_feature_incompatible(ext4_superblock_t *, uint32_t);
extern bool ext4_superblock_has_feature_read_only(ext4_superblock_t *, uint32_t);
extern int ext4_superblock_read_direct(service_id_t, ext4_superblock_t **);
extern int ext4_superblock_write_direct(service_id_t, ext4_superblock_t *);
extern int ext4_superblock_check_sanity(ext4_superblock_t *);

extern uint32_t ext4_superblock_get_block_group_count(ext4_superblock_t *);
extern uint32_t ext4_superblock_get_blocks_in_group(ext4_superblock_t *, uint32_t);
extern uint32_t ext4_superblock_get_inodes_in_group(ext4_superblock_t *, uint32_t);

#endif

/**
 * @}
 */
