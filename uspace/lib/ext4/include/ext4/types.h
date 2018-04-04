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

#ifndef LIBEXT4_TYPES_H_
#define LIBEXT4_TYPES_H_

#include <block.h>

/*
 * Structure of the super block
 */
typedef struct ext4_superblock {
	uint32_t inodes_count;              /* I-nodes count */
	uint32_t blocks_count_lo;           /* Blocks count */
	uint32_t reserved_blocks_count_lo;  /* Reserved blocks count */
	uint32_t free_blocks_count_lo;      /* Free blocks count */
	uint32_t free_inodes_count;         /* Free inodes count */
	uint32_t first_data_block;          /* First Data Block */
	uint32_t log_block_size;            /* Block size */
	uint32_t log_frag_size;             /* Obsoleted fragment size */
	uint32_t blocks_per_group;          /* Number of blocks per group */
	uint32_t frags_per_group;           /* Obsoleted fragments per group */
	uint32_t inodes_per_group;          /* Number of inodes per group */
	uint32_t mount_time;                /* Mount time */
	uint32_t write_time;                /* Write time */
	uint16_t mount_count;               /* Mount count */
	uint16_t max_mount_count;           /* Maximal mount count */
	uint16_t magic;                     /* Magic signature */
	uint16_t state;                     /* File system state */
	uint16_t errors;                    /* Behaviour when detecting errors */
	uint16_t minor_rev_level;           /* Minor revision level */
	uint32_t last_check_time;           /* Time of last check */
	uint32_t check_interval;            /* Maximum time between checks */
	uint32_t creator_os;                /* Creator OS */
	uint32_t rev_level;                 /* Revision level */
	uint16_t def_resuid;                /* Default uid for reserved blocks */
	uint16_t def_resgid;                /* Default gid for reserved blocks */

	/* Fields for EXT4_DYNAMIC_REV superblocks only. */
	uint32_t first_inode;             /* First non-reserved inode */
	uint16_t inode_size;              /* Size of inode structure */
	uint16_t block_group_index;       /* Block group index of this superblock */
	uint32_t features_compatible;     /* Compatible feature set */
	uint32_t features_incompatible;   /* Incompatible feature set */
	uint32_t features_read_only;      /* Readonly-compatible feature set */
	uint8_t uuid[16];                 /* 128-bit uuid for volume */
	char volume_name[16];             /* Volume name */
	char last_mounted[64];            /* Directory where last mounted */
	uint32_t algorithm_usage_bitmap;  /* For compression */

	/*
	 * Performance hints. Directory preallocation should only
	 * happen if the EXT4_FEATURE_COMPAT_DIR_PREALLOC flag is on.
	 */
	uint8_t prealloc_blocks;        /* Number of blocks to try to preallocate */
	uint8_t prealloc_dir_blocks;    /* Number to preallocate for dirs */
	uint16_t reserved_gdt_blocks;   /* Per group desc for online growth */

	/*
	 * Journaling support valid if EXT4_FEATURE_COMPAT_HAS_JOURNAL set.
	 */
	uint8_t journal_uuid[16];       /* UUID of journal superblock */
	uint32_t journal_inode_number;  /* Inode number of journal file */
	uint32_t journal_dev;           /* Device number of journal file */
	uint32_t last_orphan;           /* Head of list of inodes to delete */
	uint32_t hash_seed[4];          /* HTREE hash seed */
	uint8_t default_hash_version;   /* Default hash version to use */
	uint8_t journal_backup_type;
	uint16_t desc_size;             /* Size of group descriptor */
	uint32_t default_mount_opts;    /* Default mount options */
	uint32_t first_meta_bg;         /* First metablock block group */
	uint32_t mkfs_time;             /* When the filesystem was created */
	uint32_t journal_blocks[17];    /* Backup of the journal inode */

	/* 64bit support valid if EXT4_FEATURE_COMPAT_64BIT */
	uint32_t blocks_count_hi;           /* Blocks count */
	uint32_t reserved_blocks_count_hi;  /* Reserved blocks count */
	uint32_t free_blocks_count_hi;      /* Free blocks count */
	uint16_t min_extra_isize;           /* All inodes have at least # bytes */
	uint16_t want_extra_isize;          /* New inodes should reserve # bytes */
	uint32_t flags;                     /* Miscellaneous flags */
	uint16_t raid_stride;               /* RAID stride */
	uint16_t mmp_interval;              /* # seconds to wait in MMP checking */
	uint64_t mmp_block;                 /* Block for multi-mount protection */
	uint32_t raid_stripe_width;         /* Blocks on all data disks (N * stride) */
	uint8_t log_groups_per_flex;        /* FLEX_BG group size */
	uint8_t reserved_char_pad;
	uint16_t reserved_pad;
	uint64_t kbytes_written;            /* Number of lifetime kilobytes written */
	uint32_t snapshot_inum;             /* I-node number of active snapshot */
	uint32_t snapshot_id;               /* Sequential ID of active snapshot */
	uint64_t snapshot_r_blocks_count;   /* Reserved blocks for active snapshot's future use */
	uint32_t snapshot_list;             /* I-node number of the head of the on-disk snapshot list */
	uint32_t error_count;               /* Number of file system errors */
	uint32_t first_error_time;          /* First time an error happened */
	uint32_t first_error_ino;           /* I-node involved in first error */
	uint64_t first_error_block;         /* Block involved of first error */
	uint8_t first_error_func[32];       /* Function where the error happened */
	uint32_t first_error_line;          /* Line number where error happened */
	uint32_t last_error_time;           /* Most recent time of an error */
	uint32_t last_error_ino;            /* I-node involved in last error */
	uint32_t last_error_line;           /* Line number where error happened */
	uint64_t last_error_block;          /* Block involved of last error */
	uint8_t last_error_func[32];        /* Function where the error happened */
	uint8_t mount_opts[64];             /* String containing the mount options */
	uint32_t usr_quota_inum;            /* Inode number of user quota file */
	uint32_t grp_quota_inum;            /* Inode number of group quota file */
	uint32_t overhead_blocks;           /* Overhead blocks/clusters */
	uint32_t backup_bgs[2];             /* Block groups containing superblock backups (if SPARSE_SUPER2) */
	uint32_t encrypt_algos;             /* Encrypt algorithm in use */
	uint32_t padding[105];              /* Padding to the end of the block */
} __attribute__((packed)) ext4_superblock_t;


#define EXT4_SUPERBLOCK_MAGIC   0xEF53
#define EXT4_SUPERBLOCK_SIZE    1024
#define EXT4_SUPERBLOCK_OFFSET  1024

#define EXT4_SUPERBLOCK_OS_LINUX  0
#define EXT4_SUPERBLOCK_OS_HURD   1

/*
 * Misc. filesystem flags
 */
#define EXT4_SUPERBLOCK_FLAGS_SIGNED_HASH    0x0001  /* Signed dirhash in use */
#define EXT4_SUPERBLOCK_FLAGS_UNSIGNED_HASH  0x0002  /* Unsigned dirhash in use */
#define EXT4_SUPERBLOCK_FLAGS_TEST_FILESYS   0x0004  /* to test development code */

/*
 * Filesystem states
 */
#define EXT4_SUPERBLOCK_STATE_VALID_FS   0x0001  /* Unmounted cleanly */
#define EXT4_SUPERBLOCK_STATE_ERROR_FS   0x0002  /* Errors detected */
#define EXT4_SUPERBLOCK_STATE_ORPHAN_FS  0x0004  /* Orphans being recovered */

/*
 * Behaviour when errors detected
 */
#define EXT4_SUPERBLOCK_ERRORS_CONTINUE  1  /* Continue execution */
#define EXT4_SUPERBLOCK_ERRORS_RO        2  /* Remount fs read-only */
#define EXT4_SUPERBLOCK_ERRORS_PANIC     3  /* Panic */
#define EXT4_SUPERBLOCK_ERRORS_DEFAULT   EXT4_ERRORS_CONTINUE

/*
 * Compatible features
 */
#define EXT4_FEATURE_COMPAT_DIR_PREALLOC   0x0001
#define EXT4_FEATURE_COMPAT_IMAGIC_INODES  0x0002
#define EXT4_FEATURE_COMPAT_HAS_JOURNAL    0x0004
#define EXT4_FEATURE_COMPAT_EXT_ATTR       0x0008
#define EXT4_FEATURE_COMPAT_RESIZE_INODE   0x0010
#define EXT4_FEATURE_COMPAT_DIR_INDEX      0x0020
#define EXT4_FEATURE_COMPAT_SPARSE_SUPER2  0x0200

/*
 * Read-only compatible features
 */
#define EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER  0x0001
#define EXT4_FEATURE_RO_COMPAT_LARGE_FILE    0x0002
#define EXT4_FEATURE_RO_COMPAT_BTREE_DIR     0x0004
#define EXT4_FEATURE_RO_COMPAT_HUGE_FILE     0x0008
#define EXT4_FEATURE_RO_COMPAT_GDT_CSUM      0x0010
#define EXT4_FEATURE_RO_COMPAT_DIR_NLINK     0x0020
#define EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE   0x0040

/*
 * Incompatible features
 */
#define EXT4_FEATURE_INCOMPAT_COMPRESSION  0x0001
#define EXT4_FEATURE_INCOMPAT_FILETYPE     0x0002
#define EXT4_FEATURE_INCOMPAT_RECOVER      0x0004  /* Needs recovery */
#define EXT4_FEATURE_INCOMPAT_JOURNAL_DEV  0x0008  /* Journal device */
#define EXT4_FEATURE_INCOMPAT_META_BG      0x0010
#define EXT4_FEATURE_INCOMPAT_EXTENTS      0x0040  /* extents support */
#define EXT4_FEATURE_INCOMPAT_64BIT        0x0080
#define EXT4_FEATURE_INCOMPAT_MMP          0x0100
#define EXT4_FEATURE_INCOMPAT_FLEX_BG      0x0200
#define EXT4_FEATURE_INCOMPAT_EA_INODE     0x0400  /* EA in inode */
#define EXT4_FEATURE_INCOMPAT_DIRDATA      0x1000  /* data in dirent */

#define EXT4_FEATURE_COMPAT_SUPP  (EXT4_FEATURE_COMPAT_DIR_INDEX)

#define EXT4_FEATURE_INCOMPAT_SUPP \
	(EXT4_FEATURE_INCOMPAT_FILETYPE | \
	EXT4_FEATURE_INCOMPAT_EXTENTS | \
	EXT4_FEATURE_INCOMPAT_64BIT | \
	EXT4_FEATURE_INCOMPAT_FLEX_BG)

#define EXT4_FEATURE_RO_COMPAT_SUPP \
	(EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER | \
	EXT4_FEATURE_RO_COMPAT_DIR_NLINK | \
	EXT4_FEATURE_RO_COMPAT_HUGE_FILE | \
	EXT4_FEATURE_RO_COMPAT_LARGE_FILE | \
	EXT4_FEATURE_RO_COMPAT_GDT_CSUM | \
	EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE)

typedef struct ext4_filesystem {
	service_id_t device;
	ext4_superblock_t *superblock;
	aoff64_t inode_block_limits[4];
	aoff64_t inode_blocks_per_level[4];
} ext4_filesystem_t;


#define EXT4_BLOCK_GROUP_INODE_UNINIT   0x0001  /* Inode table/bitmap not in use */
#define EXT4_BLOCK_GROUP_BLOCK_UNINIT   0x0002  /* Block bitmap not in use */
#define EXT4_BLOCK_GROUP_ITABLE_ZEROED  0x0004  /* On-disk itable initialized to zero */

/*
 * Structure of a blocks group descriptor
 */
typedef struct ext4_block_group {
	uint32_t block_bitmap_lo;             /* Blocks bitmap block */
	uint32_t inode_bitmap_lo;             /* Inodes bitmap block */
	uint32_t inode_table_first_block_lo;  /* Inodes table block */
	uint16_t free_blocks_count_lo;        /* Free blocks count */
	uint16_t free_inodes_count_lo;        /* Free inodes count */
	uint16_t used_dirs_count_lo;          /* Directories count */
	uint16_t flags;                       /* EXT4_BG_flags (INODE_UNINIT, etc) */
	uint32_t reserved[2];                 /* Likely block/inode bitmap checksum */
	uint16_t itable_unused_lo;            /* Unused inodes count */
	uint16_t checksum;                    /* crc16(sb_uuid+group+desc) */

	uint32_t block_bitmap_hi;             /* Blocks bitmap block MSB */
	uint32_t inode_bitmap_hi;             /* I-nodes bitmap block MSB */
	uint32_t inode_table_first_block_hi;  /* I-nodes table block MSB */
	uint16_t free_blocks_count_hi;        /* Free blocks count MSB */
	uint16_t free_inodes_count_hi;        /* Free i-nodes count MSB */
	uint16_t used_dirs_count_hi;          /* Directories count MSB */
	uint16_t itable_unused_hi;            /* Unused inodes count MSB */
	uint32_t reserved2[3];                /* Padding */
} ext4_block_group_t;

typedef struct ext4_block_group_ref {
	block_t *block;                   /* Reference to a block containing this block group descr */
	ext4_block_group_t *block_group;
	ext4_filesystem_t *fs;
	uint32_t index;
	bool dirty;
} ext4_block_group_ref_t;

#define EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE  32
#define EXT4_MAX_BLOCK_GROUP_DESCRIPTOR_SIZE  64

#define EXT4_MIN_BLOCK_SIZE   1024   /* 1 KiB */
#define EXT4_MAX_BLOCK_SIZE   65536  /* 64 KiB */
#define EXT4_REV0_INODE_SIZE  128

#define EXT4_INODE_BLOCK_SIZE  512

#define EXT4_INODE_DIRECT_BLOCK_COUNT      12
#define EXT4_INODE_INDIRECT_BLOCK          EXT4_INODE_DIRECT_BLOCK_COUNT
#define EXT4_INODE_DOUBLE_INDIRECT_BLOCK   (EXT4_INODE_INDIRECT_BLOCK + 1)
#define EXT4_INODE_TRIPPLE_INDIRECT_BLOCK  (EXT4_INODE_DOUBLE_INDIRECT_BLOCK + 1)
#define EXT4_INODE_BLOCKS                  (EXT4_INODE_TRIPPLE_INDIRECT_BLOCK + 1)
#define EXT4_INODE_INDIRECT_BLOCK_COUNT    (EXT4_INODE_BLOCKS - EXT4_INODE_DIRECT_BLOCK_COUNT)

/*
 * Structure of an inode on the disk
 */
typedef struct ext4_inode {
	uint16_t mode;                       /* File mode */
	uint16_t uid;                        /* Low 16 bits of owner uid */
	uint32_t size_lo;                    /* Size in bytes */
	uint32_t access_time;                /* Access time */
	uint32_t change_inode_time;          /* I-node change time */
	uint32_t modification_time;          /* Modification time */
	uint32_t deletion_time;              /* Deletion time */
	uint16_t gid;                        /* Low 16 bits of group id */
	uint16_t links_count;                /* Links count */
	uint32_t blocks_count_lo;            /* Blocks count */
	uint32_t flags;                      /* File flags */
	uint32_t unused_osd1;                /* OS dependent - not used in HelenOS */
	uint32_t blocks[EXT4_INODE_BLOCKS];  /* Pointers to blocks */
	uint32_t generation;                 /* File version (for NFS) */
	uint32_t file_acl_lo;                /* File ACL */
	uint32_t size_hi;
	uint32_t obso_faddr;                 /* Obsoleted fragment address */

	union {
		struct {
			uint16_t blocks_high;
			uint16_t file_acl_high;
			uint16_t uid_high;
			uint16_t gid_high;
			uint32_t reserved2;
		} linux2;
		struct {
			uint16_t reserved1;
			uint16_t mode_high;
			uint16_t uid_high;
			uint16_t gid_high;
			uint32_t author;
		} hurd2;
	} __attribute__((packed)) osd2;

	uint16_t extra_isize;
	uint16_t pad1;
	uint32_t ctime_extra;   /* Extra change time (nsec << 2 | epoch) */
	uint32_t mtime_extra;   /* Extra Modification time (nsec << 2 | epoch) */
	uint32_t atime_extra;   /* Extra Access time (nsec << 2 | epoch) */
	uint32_t crtime;        /* File creation time */
	uint32_t crtime_extra;  /* Extra file creation time (nsec << 2 | epoch) */
	uint32_t version_hi;    /* High 32 bits for 64-bit version */
} __attribute__((packed)) ext4_inode_t;

#define EXT4_INODE_MODE_FIFO       0x1000
#define EXT4_INODE_MODE_CHARDEV    0x2000
#define EXT4_INODE_MODE_DIRECTORY  0x4000
#define EXT4_INODE_MODE_BLOCKDEV   0x6000
#define EXT4_INODE_MODE_FILE       0x8000
#define EXT4_INODE_MODE_SOFTLINK   0xA000
#define EXT4_INODE_MODE_SOCKET     0xC000
#define EXT4_INODE_MODE_TYPE_MASK  0xF000

/*
 * Inode flags
 */
#define EXT4_INODE_FLAG_SECRM      0x00000001  /* Secure deletion */
#define EXT4_INODE_FLAG_UNRM       0x00000002  /* Undelete */
#define EXT4_INODE_FLAG_COMPR      0x00000004  /* Compress file */
#define EXT4_INODE_FLAG_SYNC       0x00000008  /* Synchronous updates */
#define EXT4_INODE_FLAG_IMMUTABLE  0x00000010  /* Immutable file */
#define EXT4_INODE_FLAG_APPEND     0x00000020  /* writes to file may only append */
#define EXT4_INODE_FLAG_NODUMP     0x00000040  /* do not dump file */
#define EXT4_INODE_FLAG_NOATIME    0x00000080  /* do not update atime */

/* Compression flags */
#define EXT4_INODE_FLAG_DIRTY     0x00000100
#define EXT4_INODE_FLAG_COMPRBLK  0x00000200  /* One or more compressed clusters */
#define EXT4_INODE_FLAG_NOCOMPR   0x00000400  /* Don't compress */
#define EXT4_INODE_FLAG_ECOMPR    0x00000800  /* Compression error */

#define EXT4_INODE_FLAG_INDEX         0x00001000  /* hash-indexed directory */
#define EXT4_INODE_FLAG_IMAGIC        0x00002000  /* AFS directory */
#define EXT4_INODE_FLAG_JOURNAL_DATA  0x00004000  /* File data should be journaled */
#define EXT4_INODE_FLAG_NOTAIL        0x00008000  /* File tail should not be merged */
#define EXT4_INODE_FLAG_DIRSYNC       0x00010000  /* Dirsync behaviour (directories only) */
#define EXT4_INODE_FLAG_TOPDIR        0x00020000  /* Top of directory hierarchies */
#define EXT4_INODE_FLAG_HUGE_FILE     0x00040000  /* Set to each huge file */
#define EXT4_INODE_FLAG_EXTENTS       0x00080000  /* Inode uses extents */
#define EXT4_INODE_FLAG_EA_INODE      0x00200000  /* Inode used for large EA */
#define EXT4_INODE_FLAG_EOFBLOCKS     0x00400000  /* Blocks allocated beyond EOF */
#define EXT4_INODE_FLAG_RESERVED      0x80000000  /* reserved for ext4 lib */

#define EXT4_INODE_ROOT_INDEX  2

typedef struct ext4_inode_ref {
	block_t *block;         /* Reference to a block containing this inode */
	ext4_inode_t *inode;
	ext4_filesystem_t *fs;
	uint32_t index;         /* Index number of this inode */
	bool dirty;
} ext4_inode_ref_t;


#define EXT4_DIRECTORY_FILENAME_LEN  255

#define EXT4_DIRECTORY_FILETYPE_UNKNOWN   0
#define EXT4_DIRECTORY_FILETYPE_REG_FILE  1
#define EXT4_DIRECTORY_FILETYPE_DIR       2
#define EXT4_DIRECTORY_FILETYPE_CHRDEV    3
#define EXT4_DIRECTORY_FILETYPE_BLKDEV    4
#define EXT4_DIRECTORY_FILETYPE_FIFO      5
#define EXT4_DIRECTORY_FILETYPE_SOCK      6
#define EXT4_DIRECTORY_FILETYPE_SYMLINK   7

/**
 * Linked list directory entry structure
 */
typedef struct ext4_directory_entry_ll {
	uint32_t inode;         /* I-node for the entry */
	uint16_t entry_length;  /* Distance to the next directory entry */
	uint8_t name_length;    /* Lower 8 bits of name length */

	union {
		uint8_t name_length_high;  /* Higher 8 bits of name length */
		uint8_t inode_type;        /* Type of referenced inode (in rev >= 0.5) */
	} __attribute__((packed));

	uint8_t name[EXT4_DIRECTORY_FILENAME_LEN];  /* Entry name */
} __attribute__((packed)) ext4_directory_entry_ll_t;

typedef struct ext4_directory_iterator {
	ext4_inode_ref_t *inode_ref;
	block_t *current_block;
	aoff64_t current_offset;
	ext4_directory_entry_ll_t *current;
} ext4_directory_iterator_t;

typedef struct ext4_directory_search_result {
	block_t *block;
	ext4_directory_entry_ll_t *dentry;
} ext4_directory_search_result_t;

/* Structures for indexed directory */

typedef struct ext4_directory_dx_countlimit {
	uint16_t limit;
	uint16_t count;
} ext4_directory_dx_countlimit_t;

typedef struct ext4_directory_dx_dot_entry {
	uint32_t inode;
	uint16_t entry_length;
	uint8_t name_length;
	uint8_t inode_type;
	uint8_t name[4];
} ext4_directory_dx_dot_entry_t;

typedef struct ext4_directory_dx_root_info {
	uint32_t reserved_zero;
	uint8_t hash_version;
	uint8_t info_length;
	uint8_t indirect_levels;
	uint8_t unused_flags;
} ext4_directory_dx_root_info_t;

typedef struct ext4_directory_dx_entry {
	uint32_t hash;
	uint32_t block;
} ext4_directory_dx_entry_t;

typedef struct ext4_directory_dx_root {
	ext4_directory_dx_dot_entry_t dots[2];
	ext4_directory_dx_root_info_t info;
	ext4_directory_dx_entry_t entries[0];
} ext4_directory_dx_root_t;

typedef struct ext4_fake_directory_entry {
	uint32_t inode;
	uint16_t entry_length;
	uint8_t name_length;
	uint8_t inode_type;
} ext4_fake_directory_entry_t;

typedef struct ext4_directory_dx_node {
	ext4_fake_directory_entry_t fake;
	ext4_directory_dx_entry_t entries[0];
} ext4_directory_dx_node_t;

typedef struct ext4_directory_dx_block {
	block_t *block;
	ext4_directory_dx_entry_t *entries;
	ext4_directory_dx_entry_t *position;
} ext4_directory_dx_block_t;

#define EXT4_DIRECTORY_HTREE_EOF  UINT32_C(0x7fffffff)

/*
 * This is the extent on-disk structure.
 * It's used at the bottom of the tree.
 */
typedef struct ext4_extent {
	uint32_t first_block;  /* First logical block extent covers */
	uint16_t block_count;  /* Number of blocks covered by extent */
	uint16_t start_hi;     /* High 16 bits of physical block */
	uint32_t start_lo;     /* Low 32 bits of physical block */
} ext4_extent_t;

/*
 * This is index on-disk structure.
 * It's used at all the levels except the bottom.
 */
typedef struct ext4_extent_index {
	uint32_t first_block;  /* Index covers logical blocks from 'block' */

	/**
	 * Pointer to the physical block of the next
	 * level. leaf or next index could be there
	 * high 16 bits of physical block
	 */
	uint32_t leaf_lo;
	uint16_t leaf_hi;
	uint16_t padding;
} ext4_extent_index_t;

/*
 * Each block (leaves and indexes), even inode-stored has header.
 */
typedef struct ext4_extent_header {
	uint16_t magic;
	uint16_t entries_count;      /* Number of valid entries */
	uint16_t max_entries_count;  /* Capacity of store in entries */
	uint16_t depth;              /* Has tree real underlying blocks? */
	uint32_t generation;         /* generation of the tree */
} ext4_extent_header_t;

typedef struct ext4_extent_path {
	block_t *block;
	uint16_t depth;
	ext4_extent_header_t *header;
	ext4_extent_index_t *index;
	ext4_extent_t *extent;
} ext4_extent_path_t;

#define EXT4_EXTENT_MAGIC  0xF30A

#define	EXT4_EXTENT_FIRST(header) \
	((ext4_extent_t *) (((void *) (header)) + sizeof(ext4_extent_header_t)))

#define	EXT4_EXTENT_FIRST_INDEX(header) \
	((ext4_extent_index_t *) (((void *) (header)) + sizeof(ext4_extent_header_t)))

#define EXT4_HASH_VERSION_LEGACY             0
#define EXT4_HASH_VERSION_HALF_MD4           1
#define EXT4_HASH_VERSION_TEA                2
#define EXT4_HASH_VERSION_LEGACY_UNSIGNED    3
#define EXT4_HASH_VERSION_HALF_MD4_UNSIGNED  4
#define EXT4_HASH_VERSION_TEA_UNSIGNED       5

typedef struct ext4_hash_info {
	uint32_t hash;
	uint32_t minor_hash;
	uint32_t hash_version;
	const uint32_t *seed;
} ext4_hash_info_t;

#endif

/**
 * @}
 */
