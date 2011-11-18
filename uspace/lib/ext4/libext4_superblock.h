/*
 * Copyright (c) 2011 Frantisek Princ
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

/*
 * Structure of the super block
 */
typedef struct ext4_superblock {
	uint32_t inodes_count; // Inodes count
	uint32_t blocks_count_lo; // Blocks count
	uint32_t reserved_blocks_count_lo; // Reserved blocks count
	uint32_t free_blocks_count_lo; // Free blocks count
	uint32_t free_inodes_count; // Free inodes count
	uint32_t first_data_block; // First Data Block
	uint32_t log_block_size; // Block size
	uint32_t obso_log_frag_size; // Obsoleted fragment size
	uint32_t blocks_per_group; // Number of blocks per group
	uint32_t obso_frags_per_group; // Obsoleted fragments per group
	uint32_t inodes_per_group; // Number of inodes per group
	uint32_t mount_time; // Mount time
	uint32_t write_time; // Write time
	uint16_t mount_count; // Mount count
	uint16_t max_mount_count; // Maximal mount count
	uint16_t magic; // Magic signature
	uint16_t state; // File system state
	uint16_t errors; // Behaviour when detecting errors
	uint16_t minor_rev_level; // Minor revision level
	uint32_t last_check_time; // Time of last check
	uint32_t check_interval; // Maximum time between checks
	uint32_t creator_os; // Creator OS
	uint32_t rev_level; // Revision level
	uint16_t def_resuid; // Default uid for reserved blocks
	uint16_t def_resgid; // Default gid for reserved blocks

	// Fields for EXT4_DYNAMIC_REV superblocks only.
	uint32_t first_inode; // First non-reserved inode
	uint16_t inode_size; // Size of inode structure
	uint16_t block_group_number; // Block group number of this superblock
	uint32_t features_compatible; // Compatible feature set
	uint32_t features_incompatible; // Incompatible feature set
	uint32_t features_read_only; // Readonly-compatible feature set
	uint8_t uuid[16]; // 128-bit uuid for volume
	char volume_name[16]; // Volume name
	char last_mounted[64]; // Directory where last mounted
	uint32_t algorithm_usage_bitmap; // For compression

	/*
	 * Performance hints. Directory preallocation should only
	 * happen if the EXT4_FEATURE_COMPAT_DIR_PREALLOC flag is on.
	 */
	uint8_t s_prealloc_blocks; // Number of blocks to try to preallocate
	uint8_t s_prealloc_dir_blocks; // Number to preallocate for dirs
	uint16_t s_reserved_gdt_blocks; // Per group desc for online growth

	/*
	 * Journaling support valid if EXT4_FEATURE_COMPAT_HAS_JOURNAL set.
	 */
	uint8_t journal_uuid[16]; // UUID of journal superblock
	uint32_t journal_inode_number; // Inode number of journal file
	uint32_t journal_dev; // Device number of journal file
	uint32_t last_orphan; // Head of list of inodes to delete
	uint32_t hash_seed[4]; // HTREE hash seed
	uint8_t default_hash_version; // Default hash version to use
	uint8_t journal_backup_type;
	uint16_t desc_size; // Size of group descriptor
	uint32_t default_mount_opts; // Default mount options
	uint32_t first_meta_bg; // First metablock block group
	uint32_t mkfs_time; // When the filesystem was created
	uint32_t journal_blocks[17]; // Backup of the journal inode

	/* 64bit support valid if EXT4_FEATURE_COMPAT_64BIT */
	uint32_t blocks_count_hi; // Blocks count
	uint32_t reserved_blocks_count_hi; // Reserved blocks count
	uint32_t free_blocks_count_hi; // Free blocks count
	uint16_t min_extra_isize; // All inodes have at least # bytes
	uint16_t want_extra_isize; // New inodes should reserve # bytes
	uint32_t flags; // Miscellaneous flags
	uint16_t raid_stride; // RAID stride
	uint16_t mmp_interval; // # seconds to wait in MMP checking
	uint64_t mmp_block; // Block for multi-mount protection
	uint32_t raid_stripe_width; // blocks on all data disks (N*stride)
	uint8_t log_groups_per_flex; // FLEX_BG group size
	uint8_t reserved_char_pad;
	uint16_t reserved_pad;
	uint64_t kbytes_written; // Number of lifetime kilobytes written
	uint32_t snapshot_inum; // Inode number of active snapshot
	uint32_t snapshot_id; // Sequential ID of active snapshot
	uint64_t snapshot_r_blocks_count; /* reserved blocks for active snapshot's future use */
	uint32_t snapshot_list; // inode number of the head of the on-disk snapshot list
	uint32_t error_count; // number of fs errors
	uint32_t first_error_time; // First time an error happened
	uint32_t first_error_ino; // Inode involved in first error
	uint64_t first_error_block; // block involved of first error
	uint8_t first_error_func[32]; // Function where the error happened
	uint32_t first_error_line; // Line number where error happened
	uint32_t last_error_time; // Most recent time of an error
	uint32_t last_error_ino; // Inode involved in last error
	uint32_t last_error_line; // Line number where error happened
	uint64_t last_error_block;     // Block involved of last error
	uint8_t last_error_func[32];  // Function where the error happened
	uint8_t mount_opts[64];
	uint32_t padding[112]; // Padding to the end of the block
} __attribute__((packed)) ext4_superblock_t;

#define EXT4_SUPERBLOCK_MAGIC		0xEF53
#define EXT4_SUPERBLOCK_SIZE		1024
#define EXT4_SUPERBLOCK_OFFSET		1024

#define EXT4_SUPERBLOCK_OS_LINUX	0
#define EXT4_SUPERBLOCK_OS_HURD		1

/*
 * Misc. filesystem flags
 */
#define EXT4_SUPERBLOCK_FLAGS_SIGNED_HASH	0x0001  /* Signed dirhash in use */
#define EXT4_SUPERBLOCK_FLAGS_UNSIGNED_HASH	0x0002  /* Unsigned dirhash in use */
#define EXT4_SUPERBLOCK_FLAGS_TEST_FILESYS	0x0004  /* to test development code */

/* Compatible features */
#define EXT4_FEATURE_COMPAT_DIR_PREALLOC        0x0001
#define EXT4_FEATURE_COMPAT_IMAGIC_INODES       0x0002
#define EXT4_FEATURE_COMPAT_HAS_JOURNAL         0x0004
#define EXT4_FEATURE_COMPAT_EXT_ATTR            0x0008
#define EXT4_FEATURE_COMPAT_RESIZE_INODE        0x0010
#define EXT4_FEATURE_COMPAT_DIR_INDEX           0x0020

/* Read-only compatible features */
#define EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER     0x0001
#define EXT4_FEATURE_RO_COMPAT_LARGE_FILE       0x0002
#define EXT4_FEATURE_RO_COMPAT_BTREE_DIR        0x0004
#define EXT4_FEATURE_RO_COMPAT_HUGE_FILE        0x0008
#define EXT4_FEATURE_RO_COMPAT_GDT_CSUM         0x0010
#define EXT4_FEATURE_RO_COMPAT_DIR_NLINK        0x0020
#define EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE      0x0040

/* Incompatible features */
#define EXT4_FEATURE_INCOMPAT_COMPRESSION       0x0001
#define EXT4_FEATURE_INCOMPAT_FILETYPE          0x0002
#define EXT4_FEATURE_INCOMPAT_RECOVER           0x0004 /* Needs recovery */
#define EXT4_FEATURE_INCOMPAT_JOURNAL_DEV       0x0008 /* Journal device */
#define EXT4_FEATURE_INCOMPAT_META_BG           0x0010
#define EXT4_FEATURE_INCOMPAT_EXTENTS           0x0040 /* extents support */
#define EXT4_FEATURE_INCOMPAT_64BIT             0x0080
#define EXT4_FEATURE_INCOMPAT_MMP               0x0100
#define EXT4_FEATURE_INCOMPAT_FLEX_BG           0x0200
#define EXT4_FEATURE_INCOMPAT_EA_INODE          0x0400 /* EA in inode */
#define EXT4_FEATURE_INCOMPAT_DIRDATA           0x1000 /* data in dirent */

// TODO MODIFY features corresponding with implementation
#define EXT4_FEATURE_COMPAT_SUPP EXT4_FEATURE_COMPAT_EXT_ATTR

#define EXT4_FEATURE_INCOMPAT_SUPP      (EXT4_FEATURE_INCOMPAT_FILETYPE| \
                                         EXT4_FEATURE_INCOMPAT_RECOVER| \
                                         EXT4_FEATURE_INCOMPAT_META_BG| \
                                         EXT4_FEATURE_INCOMPAT_EXTENTS| \
                                         EXT4_FEATURE_INCOMPAT_64BIT| \
                                         EXT4_FEATURE_INCOMPAT_FLEX_BG)

#define EXT4_FEATURE_RO_COMPAT_SUPP     (EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER| \
                                         EXT4_FEATURE_RO_COMPAT_LARGE_FILE| \
                                         EXT4_FEATURE_RO_COMPAT_GDT_CSUM| \
                                         EXT4_FEATURE_RO_COMPAT_DIR_NLINK | \
                                         EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE | \
                                         EXT4_FEATURE_RO_COMPAT_BTREE_DIR |\
                                         EXT4_FEATURE_RO_COMPAT_HUGE_FILE)



extern uint32_t ext4_superblock_get_inodes_count(ext4_superblock_t *);
extern uint64_t ext4_superblock_get_blocks_count(ext4_superblock_t *);
extern uint64_t ext4_superblock_get_reserved_blocks_count(ext4_superblock_t *);
extern uint64_t ext4_superblock_get_free_blocks_count(ext4_superblock_t *);
extern uint32_t ext4_superblock_get_free_inodes_count(ext4_superblock_t *);
extern uint32_t ext4_superblock_get_first_data_block(ext4_superblock_t *);
extern uint32_t ext4_superblock_get_log_block_size(ext4_superblock_t *);
extern uint32_t ext4_superblock_get_block_size(ext4_superblock_t *);
extern uint32_t ext4_superblock_get_blocks_per_group(ext4_superblock_t *);
extern uint32_t ext4_superblock_get_inodes_per_group(ext4_superblock_t *);
extern uint32_t ext4_superblock_get_mount_time(ext4_superblock_t *);
extern uint32_t ext4_superblock_get_write_time(ext4_superblock_t *);
extern uint16_t ext4_superblock_get_mount_count(ext4_superblock_t *);
extern uint16_t ext4_superblock_get_max_mount_count(ext4_superblock_t *);
extern uint16_t ext4_superblock_get_magic(ext4_superblock_t *);
extern uint16_t ext4_superblock_get_state(ext4_superblock_t *);
extern uint16_t ext4_superblock_get_errors(ext4_superblock_t *);
extern uint16_t ext4_superblock_get_minor_rev_level(ext4_superblock_t *);
extern uint32_t ext4_superblock_get_last_check_time(ext4_superblock_t *);
extern uint32_t ext4_superblock_get_check_interval(ext4_superblock_t *);
extern uint32_t ext4_superblock_get_creator_os(ext4_superblock_t *);
extern uint32_t ext4_superblock_get_rev_level(ext4_superblock_t *);

/*
uint16_t s_def_resuid; // Default uid for reserved blocks
uint16_t s_def_resgid; // Default gid for reserved blocks
*/

extern uint32_t ext4_superblock_get_first_inode(ext4_superblock_t *);
extern uint16_t ext4_superblock_get_inode_size(ext4_superblock_t *);
extern uint16_t ext4_superblock_get_block_group_number(ext4_superblock_t *);
extern uint32_t	ext4_superblock_get_features_compatible(ext4_superblock_t *);
extern uint32_t	ext4_superblock_get_features_incompatible(ext4_superblock_t *);
extern uint32_t	ext4_superblock_get_features_read_only(ext4_superblock_t *);

/*
uint8_t s_uuid[16]; // 128-bit uuid for volume
char volume_name[16]; // Volume name
char last_mounted[64]; // Directory where last mounted
uint32_t s_algorithm_usage_bitmap; // For compression
uint8_t s_prealloc_blocks; // Number of blocks to try to preallocate
uint8_t s_prealloc_dir_blocks; // Number to preallocate for dirs
uint16_t s_reserved_gdt_blocks; // Per group desc for online growth
uint8_t s_journal_uuid[16]; // UUID of journal superblock
uint32_t s_journal_inum; // Inode number of journal file
uint32_t s_journal_dev; // Device number of journal file
uint32_t s_last_orphan; // Head of list of inodes to delete
*/
extern uint32_t* ext4_superblock_get_hash_seed(ext4_superblock_t *);

/*
uint8_t s_def_hash_version; // Default hash version to use
uint8_t s_jnl_backup_type;
*/

extern uint16_t ext4_superblock_get_desc_size(ext4_superblock_t *);

/*
uint32_t s_default_mount_opts; // Default mount options
uint32_t s_first_meta_bg; // First metablock block group
uint32_t s_mkfs_time; // When the filesystem was created
uint32_t s_jnl_blocks[17]; // Backup of the journal inode
uint16_t s_min_extra_isize; // All inodes have at least # bytes
uint16_t s_want_extra_isize; // New inodes should reserve # bytes
*/
extern uint32_t ext4_superblock_get_flags(ext4_superblock_t *);
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
extern int ext4_superblock_check_sanity(ext4_superblock_t *);

extern uint32_t ext4_superblock_get_block_group_count(ext4_superblock_t *);
extern uint32_t ext4_superblock_get_blocks_in_group(ext4_superblock_t *, uint32_t);
extern uint32_t ext4_superblock_get_inodes_in_group(ext4_superblock_t *, uint32_t);

#endif

/**
 * @}
 */
