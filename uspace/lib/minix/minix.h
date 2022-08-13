/*
 * SPDX-FileCopyrightText: 2011 Maurizio Lombardi
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libminix
 * @{
 */

#ifndef _MINIX_FS_H_
#define _MINIX_FS_H_

#include <stdint.h>

#define MFS_BLOCKSIZE		1024
#define S_ISDIR(m)		(((m) & S_IFMT) == S_IFDIR)
#define S_ISREG(m)		(((m) & S_IFMT) == S_IFREG)
#define S_IFDIR			0040000		/* Directory */
#define S_IFREG			0100000		/* Regular file */
#define S_IFMT			00170000

/* The following block sizes are valid only on V3 filesystem */
#define MFS_MIN_BLOCKSIZE	1024
#define MFS_MAX_BLOCKSIZE	4096

#define MFS_ROOT_INO		1
#define MFS_SUPERBLOCK		1
#define MFS_SUPERBLOCK_SIZE	1024
#define MFS_BOOTBLOCK_SIZE	1024

#define V2_NR_DIRECT_ZONES	7
#define V2_NR_INDIRECT_ZONES	3

#define V1_NR_DIRECT_ZONES	7
#define V1_NR_INDIRECT_ZONES	2

#define V1_INODES_PER_BLOCK	(MFS_BLOCKSIZE / sizeof(struct mfs_inode))
#define V2_INODES_PER_BLOCK	(MFS_BLOCKSIZE / sizeof(struct mfs2_inode))
#define V3_INODES_PER_BLOCK(bs)	((bs) / sizeof(struct mfs2_inode))

#define MFS_DIRSIZE		16
#define MFSL_DIRSIZE		32
#define MFS3_DIRSIZE		64

#define MFS_MAX_NAME_LEN	14
#define MFS_L_MAX_NAME_LEN	30
#define MFS3_MAX_NAME_LEN	60

#define MFS_MAGIC_V1		0x137F
#define MFS_MAGIC_V1R		0x7F13

#define MFS_MAGIC_V1L		0x138F
#define MFS_MAGIC_V1LR		0x8F13

#define MFS_MAGIC_V2		0x2468
#define MFS_MAGIC_V2R		0x6824

#define MFS_MAGIC_V2L		0x2478
#define MFS_MAGIC_V2LR		0x7824

#define MFS_MAGIC_V3		0x4D5A
#define MFS_MAGIC_V3R		0x5A4D

#define MFS_VALID_FS		0x0001
#define MFS_ERROR_FS		0x0002

/* MFS V1/V2 superblock data on disk */
struct mfs_superblock {
	/* Total number of inodes on the device */
	uint16_t	s_ninodes;
	/* Total number of zones on the device */
	uint16_t	s_nzones;
	/* Number of inode bitmap blocks */
	uint16_t	s_ibmap_blocks;
	/* Number of zone bitmap blocks */
	uint16_t	s_zbmap_blocks;
	/* First data zone on device */
	uint16_t	s_first_data_zone;
	/* Base 2 logarithm of the zone to block ratio */
	uint16_t	s_log2_zone_size;
	/* Maximum file size expressed in bytes */
	uint32_t	s_max_file_size;
	/*
	 * Magic number used to recognize MinixFS
	 * and to detect on-disk endianness
	 */
	uint16_t	s_magic;
	/* Flag used to detect FS errors */
	uint16_t	s_state;
	/* Total number of zones on the device (V2 only) */
	uint32_t	s_nzones2;
} __attribute__((packed));

/* MFS V3 superblock data on disk */
struct mfs3_superblock {
	/* Total number of inodes on the device */
	uint32_t	s_ninodes;
	uint16_t	s_pad0;
	/* Number of inode bitmap blocks */
	int16_t		s_ibmap_blocks;
	/* Number of zone bitmap blocks */
	int16_t		s_zbmap_blocks;
	/* First data zone on device */
	uint16_t	s_first_data_zone;
	/* Base 2 logarithm of the zone to block ratio */
	int16_t		s_log2_zone_size;
	int16_t		s_pad1;
	/* Maximum file size expressed in bytes */
	uint32_t	s_max_file_size;
	/* Total number of zones on the device */
	uint32_t	s_nzones;
	/*
	 * Magic number used to recognize MinixFS
	 * and to detect on-disk endianness
	 */
	int16_t		s_magic;
	int16_t		s_pad2;
	/* Filesystem block size expressed in bytes */
	uint16_t	s_block_size;
	/* Filesystem disk format version */
	int8_t		s_disk_version;
} __attribute__((packed));

/* MinixFS V1 inode structure as it is on disk */
struct mfs_inode {
	uint16_t	i_mode;
	int16_t		i_uid;
	int32_t		i_size;
	int32_t		i_mtime;
	uint8_t		i_gid;
	uint8_t		i_nlinks;
	/* Block numbers for direct zones */
	uint16_t	i_dzone[V1_NR_DIRECT_ZONES];
	/* Block numbers for indirect zones */
	uint16_t	i_izone[V1_NR_INDIRECT_ZONES];
} __attribute__((packed));

/* MinixFS V2 inode structure as it is on disk (also valid for V3). */
struct mfs2_inode {
	uint16_t 	i_mode;
	uint16_t 	i_nlinks;
	int16_t 	i_uid;
	uint16_t 	i_gid;
	int32_t 	i_size;
	int32_t		i_atime;
	int32_t		i_mtime;
	int32_t		i_ctime;
	/* Block numbers for direct zones */
	uint32_t	i_dzone[V2_NR_DIRECT_ZONES];
	/* Block numbers for indirect zones */
	uint32_t	i_izone[V2_NR_INDIRECT_ZONES];
} __attribute__((packed));

/* MinixFS V1/V2 directory entry on-disk structure */
struct mfs_dentry {
	uint16_t d_inum;
	char d_name[0];
};

/* MinixFS V3 directory entry on-disk structure */
struct mfs3_dentry {
	uint32_t d_inum;
	char d_name[0];
};

#endif

/**
 * @}
 */
