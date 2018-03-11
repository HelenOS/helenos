/*
 * Copyright (c) 2011 Maurizio Lombardi
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

/** @addtogroup fs
 * @{
 */

/**
 * @file	mkmfs.c
 * @brief	Tool for creating new Minix file systems.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <block.h>
#include <errno.h>
#include <inttypes.h>
#include <getopt.h>
#include <mem.h>
#include <str.h>
#include <time.h>
#include <minix.h>

#define NAME	"mkmfs"

#define FREE	0
#define USED	1

#define UPPER(n, size) 		(((n) / (size)) + (((n) % (size)) != 0))
#define NEXT_DENTRY(p, dirsize)	(p += (dirsize))

typedef enum {
	HELP_SHORT,
	HELP_LONG
} help_level_t;

/* Generic MFS superblock */
struct mfs_sb_info {
	uint64_t n_inodes;
	uint64_t n_zones;
	aoff64_t dev_nblocks;
	unsigned long ibmap_blocks;
	unsigned long zbmap_blocks;
	unsigned long first_data_zone;
	unsigned long itable_size;
	int log2_zone_size;
	int ino_per_block;
	int dirsize;
	uint32_t max_file_size;
	uint16_t magic;
	uint32_t block_size;
	int fs_version;
	bool longnames;
};

static void	help_cmd_mkmfs(help_level_t level);
static bool	is_power_of_two(uint32_t n);
static errno_t	init_superblock(struct mfs_sb_info *sb);
static errno_t	write_superblock(const struct mfs_sb_info *sbi);
static errno_t	write_superblock3(const struct mfs_sb_info *sbi);
static errno_t	init_bitmaps(const struct mfs_sb_info *sb);
static errno_t	init_inode_table(const struct mfs_sb_info *sb);
static errno_t	make_root_ino(const struct mfs_sb_info *sb);
static errno_t	make_root_ino2(const struct mfs_sb_info *sb);
static void	mark_bmap(uint32_t *bmap, int idx, int v);
static errno_t	insert_dentries(const struct mfs_sb_info *sb);

static inline errno_t write_block(aoff64_t off, size_t size, const void *data);

static service_id_t service_id;
static int shift;

static struct option const long_options[] = {
		{ "help", no_argument, 0, 'h' },
		{ "long-names", no_argument, 0, 'l' },
		{ "block-size", required_argument, 0, 'b' },
		{ "inodes", required_argument, 0, 'i' },
		{ NULL, no_argument, 0, '1' },
		{ NULL, no_argument, 0, '2' },
		{ 0, 0, 0, 0 }
};

int main (int argc, char **argv)
{
	errno_t rc;
	int c, opt_ind;
	char *device_name;
	size_t devblock_size;

	struct mfs_sb_info sb;

	/* Default is MinixFS V3 */
	sb.magic = MFS_MAGIC_V3;
	sb.fs_version = 3;

	/* Default block size is 4Kb */
	sb.block_size = MFS_MAX_BLOCKSIZE;
	sb.dirsize = MFS3_DIRSIZE;
	sb.n_inodes = 0;
	sb.longnames = false;
	sb.ino_per_block = V3_INODES_PER_BLOCK(MFS_MAX_BLOCKSIZE);

	if (argc == 1) {
		help_cmd_mkmfs(HELP_SHORT);
		printf("Incorrect number of arguments, try `mkmfs --help'\n");
		exit(0);
	}

	c = 0;
	optind = 0;
	opt_ind = 0;
	while (c != -1) {
		c = getopt_long(argc, argv, "lh12b:i:",
		    long_options, &opt_ind);
		switch (c) {
		case 'h':
			help_cmd_mkmfs(HELP_LONG);
			exit(0);
		case '1':
			sb.magic = MFS_MAGIC_V1;
			sb.block_size = MFS_BLOCKSIZE;
			sb.fs_version = 1;
			sb.ino_per_block = V1_INODES_PER_BLOCK;
			sb.dirsize = MFS_DIRSIZE;
			break;
		case '2':
			sb.magic = MFS_MAGIC_V2;
			sb.block_size = MFS_BLOCKSIZE;
			sb.fs_version = 2;
			sb.ino_per_block = V2_INODES_PER_BLOCK;
			sb.dirsize = MFS_DIRSIZE;
			break;
		case 'b':
			sb.block_size = (uint32_t) strtol(optarg, NULL, 10);
			break;
		case 'i':
			sb.n_inodes = (uint64_t) strtol(optarg, NULL, 10);
			break;
		case 'l':
			sb.longnames = true;
			sb.dirsize = MFSL_DIRSIZE;
			break;
		}
	}

	if (sb.block_size < MFS_MIN_BLOCKSIZE ||
	    sb.block_size > MFS_MAX_BLOCKSIZE) {
		printf(NAME ":Error! Invalid block size.\n");
		exit(0);
	} else if (!is_power_of_two(sb.block_size)) {
		/* Block size must be a power of 2. */
		printf(NAME ":Error! Invalid block size.\n");
		exit(0);
	} else if (sb.block_size > MFS_BLOCKSIZE &&
	    sb.fs_version != 3) {
		printf(NAME ":Error! Block size > 1024 is "
		    "supported by V3 filesystem only.\n");
		exit(0);
	} else if (sb.fs_version == 3 && sb.longnames) {
		printf(NAME ":Error! Long filenames are supported "
		    "by V1/V2 filesystem only.\n");
		exit(0);
	}

	if (sb.block_size == MFS_MIN_BLOCKSIZE)
		shift = 1;
	else if (sb.block_size == MFS_MAX_BLOCKSIZE)
		shift = 3;
	else
		shift = 2;

	argv += optind;

	device_name = argv[0];

	if (!device_name) {
		help_cmd_mkmfs(HELP_LONG);
		exit(0);
	}

	rc = loc_service_get_id(device_name, &service_id, 0);
	if (rc != EOK) {
		printf(NAME ": Error resolving device `%s'.\n", device_name);
		return 2;
	}

	rc = block_init(service_id, 2048);
	if (rc != EOK)  {
		printf(NAME ": Error initializing libblock.\n");
		return 2;
	}

	rc = block_get_bsize(service_id, &devblock_size);
	if (rc != EOK) {
		printf(NAME ": Error determining device block size.\n");
		return 2;
	}

	rc = block_get_nblocks(service_id, &sb.dev_nblocks);
	if (rc != EOK) {
		printf(NAME ": Warning, failed to obtain "
		    "block device size.\n");
	} else {
		printf(NAME ": Block device has %" PRIuOFF64 " blocks.\n",
		    sb.dev_nblocks);
	}

	if (devblock_size != 512) {
		printf(NAME ": Error. Device block size is not 512 bytes.\n");
		return 2;
	}

	/* Minimum block size is 1 Kb */
	sb.dev_nblocks /= 2;

	printf(NAME ": Creating Minix file system on device\n");
	printf(NAME ": Writing superblock\n");

	/* Initialize superblock */
	if (init_superblock(&sb) != EOK) {
		printf(NAME ": Error. Superblock initialization failed\n");
		return 2;
	}

	printf(NAME ": Initializing bitmaps\n");

	/* Initialize bitmaps */
	if (init_bitmaps(&sb) != EOK) {
		printf(NAME ": Error. Bitmaps initialization failed\n");
		return 2;
	}

	printf(NAME ": Initializing the inode table\n");

	/* Init inode table */
	if (init_inode_table(&sb) != EOK) {
		printf(NAME ": Error. Inode table initialization failed\n");
		return 2;
	}

	printf(NAME ": Creating the root directory inode\n");

	/* Make the root inode */
	if (sb.fs_version == 1)
		rc = make_root_ino(&sb);
	else
		rc = make_root_ino2(&sb);

	if (rc != EOK) {
		printf(NAME ": Error. Root inode initialization failed\n");
		return 2;
	}

	/* Insert directory entries . and .. */
	if (insert_dentries(&sb) != EOK) {
		printf(NAME ": Error. Root directory initialization failed\n");
		return 2;
	}

	block_fini(service_id);

	return 0;
}

/**Inserts the '.' and '..' directory entries in the root directory.
 *
 * @param sb		Pointer to the superblock structure.
 *
 * @return		EOK on success or an error code.
 */
static errno_t insert_dentries(const struct mfs_sb_info *sb)
{
	void *root_block;
	uint8_t *dentry_ptr;
	errno_t rc;
	const long root_dblock = sb->first_data_zone;

	root_block = malloc(sb->block_size);
	memset(root_block, 0x00, sb->block_size);

	if (!root_block)
		return ENOMEM;

	dentry_ptr = root_block;

	if (sb->fs_version != 3) {
		/* Directory entries for V1/V2 filesystem */
		struct mfs_dentry *dentry = root_block;

		dentry->d_inum = MFS_ROOT_INO;
		memcpy(dentry->d_name, ".\0", 2);

		dentry = (struct mfs_dentry *) NEXT_DENTRY(dentry_ptr,
		    sb->dirsize);

		dentry->d_inum = MFS_ROOT_INO;
		memcpy(dentry->d_name, "..\0", 3);
	} else {
		/* Directory entries for V3 filesystem */
		struct mfs3_dentry *dentry = root_block;

		dentry->d_inum = MFS_ROOT_INO;
		memcpy(dentry->d_name, ".\0", 2);

		dentry = (struct mfs3_dentry *) NEXT_DENTRY(dentry_ptr,
		    sb->dirsize);

		dentry->d_inum = MFS_ROOT_INO;
		memcpy(dentry->d_name, "..\0", 3);
	}

	rc = write_block(root_dblock, 1, root_block);

	free(root_block);
	return rc;
}

/**Initialize the inode table.
 *
 * @param sb		Pointer to the superblock structure.
 *
 * @return		EOK on success or an error code.
 */
static errno_t init_inode_table(const struct mfs_sb_info *sb)
{
	unsigned int i;
	uint8_t *itable_buf;
	errno_t rc = EOK;

	long itable_off = sb->zbmap_blocks + sb->ibmap_blocks + 2;
	unsigned long itable_size = sb->itable_size;

	itable_buf = malloc(sb->block_size);

	if (!itable_buf)
		return ENOMEM;

	memset(itable_buf, 0x00, sb->block_size);

	for (i = 0; i < itable_size; ++i, ++itable_off) {
		rc = write_block(itable_off, 1, itable_buf);

		if (rc != EOK)
			break;
	}

	free(itable_buf);
	return rc;
}

/**Initialize a V1 root inode.
 *
 * @param sb		Ponter to the superblock structure.
 *
 * @return		EOK on success or an error code.
 */
static errno_t make_root_ino(const struct mfs_sb_info *sb)
{
	struct mfs_inode *ino_buf;
	errno_t rc;

	const long itable_off = sb->zbmap_blocks + sb->ibmap_blocks + 2;

	const time_t sec = time(NULL);

	ino_buf = malloc(MFS_BLOCKSIZE);

	if (!ino_buf)
		return ENOMEM;

	memset(ino_buf, 0x00, MFS_BLOCKSIZE);

	ino_buf[MFS_ROOT_INO - 1].i_mode = S_IFDIR;
	ino_buf[MFS_ROOT_INO - 1].i_uid = 0;
	ino_buf[MFS_ROOT_INO - 1].i_gid = 0;
	ino_buf[MFS_ROOT_INO - 1].i_size = (sb->longnames ? MFSL_DIRSIZE :
	    MFS_DIRSIZE) * 2;
	ino_buf[MFS_ROOT_INO - 1].i_mtime = sec;
	ino_buf[MFS_ROOT_INO - 1].i_nlinks = 2;
	ino_buf[MFS_ROOT_INO - 1].i_dzone[0] = sb->first_data_zone;

	rc = write_block(itable_off, 1, ino_buf);

	free(ino_buf);
	return rc;
}

/**Initialize a Minix V2 root inode on disk, also valid for V3 filesystem.
 *
 * @param sb		Pointer to the superblock structure.
 *
 * @return		EOK on success or an error code.
 */
static errno_t make_root_ino2(const struct mfs_sb_info *sb)
{
	struct mfs2_inode *ino_buf;
	errno_t rc;

	/* Compute offset of the first inode table block */
	const long itable_off = sb->zbmap_blocks + sb->ibmap_blocks + 2;

	const time_t sec = time(NULL);

	ino_buf = malloc(sb->block_size);

	if (!ino_buf)
		return ENOMEM;

	memset(ino_buf, 0x00, sb->block_size);

	ino_buf[MFS_ROOT_INO - 1].i_mode = S_IFDIR;
	ino_buf[MFS_ROOT_INO - 1].i_uid = 0;
	ino_buf[MFS_ROOT_INO - 1].i_gid = 0;
	ino_buf[MFS_ROOT_INO - 1].i_size = MFS3_DIRSIZE * 2;
	ino_buf[MFS_ROOT_INO - 1].i_mtime = sec;
	ino_buf[MFS_ROOT_INO - 1].i_atime = sec;
	ino_buf[MFS_ROOT_INO - 1].i_ctime = sec;
	ino_buf[MFS_ROOT_INO - 1].i_nlinks = 2;
	ino_buf[MFS_ROOT_INO - 1].i_dzone[0] = sb->first_data_zone;

	rc = write_block(itable_off, 1, ino_buf);

	free(ino_buf);
	return rc;
}

/**Initialize the superblock structure on disk.
 *
 * @param sb		Pointer to the superblock structure.
 *
 * @return		EOK on success or an error code.
 */
static errno_t init_superblock(struct mfs_sb_info *sb)
{
	aoff64_t inodes;
	unsigned long ind;
	unsigned long ind2;
	unsigned long zones;
	errno_t rc;

	if (sb->longnames)
		sb->magic = sb->fs_version == 1 ? MFS_MAGIC_V1L :
		    MFS_MAGIC_V2L;

	/* Compute the number of zones on disk */

	if (sb->fs_version == 1) {
		/* Valid only for MFS V1 */
		sb->n_zones = sb->dev_nblocks > UINT16_MAX ?
		    UINT16_MAX : sb->dev_nblocks;
		ind = MFS_BLOCKSIZE / sizeof(uint16_t);
		ind2 = ind * ind;
		sb->max_file_size = (V1_NR_DIRECT_ZONES + ind + ind2) *
		    MFS_BLOCKSIZE;
	} else {
		/*Valid for MFS V2/V3*/
		size_t ptrsize;
		if (sb->fs_version == 2)
			ptrsize = sizeof(uint16_t);
		else
			ptrsize = sizeof(uint32_t);

		ind = sb->block_size / ptrsize;
		ind2 = ind * ind;
		zones = V2_NR_DIRECT_ZONES + ind + ind2;
		sb->max_file_size = zones * sb->block_size;
		sb->n_zones = sb->dev_nblocks > UINT32_MAX ?
		    UINT32_MAX : sb->dev_nblocks;

		if (sb->fs_version == 3) {
			if(INT32_MAX / sb->block_size < zones)
				sb->max_file_size = INT32_MAX;
			sb->ino_per_block = V3_INODES_PER_BLOCK(sb->block_size);
			sb->n_zones /= (sb->block_size / MFS_MIN_BLOCKSIZE);
		}
	}

	/* Round up the number of inodes to fill block size */
	if (sb->n_inodes == 0)
		inodes = sb->dev_nblocks / 3;
	else
		inodes = sb->n_inodes;

	if (inodes % sb->ino_per_block)
		inodes = ((inodes / sb->ino_per_block) + 1) *
		    sb->ino_per_block;

	if (sb->fs_version < 3)
		sb->n_inodes = inodes > UINT16_MAX ? UINT16_MAX : inodes;
	else
		sb->n_inodes = inodes > UINT32_MAX ? UINT32_MAX : inodes;

	/* Compute inode bitmap size in blocks */
	sb->ibmap_blocks = UPPER(sb->n_inodes, sb->block_size * 8);

	/* Compute inode table size */
	sb->itable_size = sb->n_inodes / sb->ino_per_block;

	/* Compute zone bitmap size in blocks */
	sb->zbmap_blocks = UPPER(sb->n_zones, sb->block_size * 8);

	/* Compute first data zone position */
	sb->first_data_zone = 2 + sb->itable_size +
	    sb->zbmap_blocks + sb->ibmap_blocks;

	/* Set log2 of zone to block ratio to zero */
	sb->log2_zone_size = 0;

	/* Check for errors */
	if (sb->first_data_zone >= sb->n_zones) {
		printf(NAME ": Error! Insufficient disk space");
		return ENOMEM;
	}

	/* Superblock is now ready to be written on disk */
	printf(NAME ": %d block size\n", sb->block_size);
	printf(NAME ": %d inodes\n", (uint32_t) sb->n_inodes);
	printf(NAME ": %d zones\n", (uint32_t) sb->n_zones);
	printf(NAME ": inode table blocks = %ld\n", sb->itable_size);
	printf(NAME ": inode bitmap blocks = %ld\n", sb->ibmap_blocks);
	printf(NAME ": zone bitmap blocks = %ld\n", sb->zbmap_blocks);
	printf(NAME ": first data zone = %d\n", (uint32_t)sb->first_data_zone);
	printf(NAME ": max file size = %u\n", sb->max_file_size);
	printf(NAME ": long fnames = %s\n", sb->longnames ? "Yes" : "No");

	if (sb->fs_version == 3)
		rc = write_superblock3(sb);
	else
		rc = write_superblock(sb);

	return rc;
}

/**Write the V1/V2 superblock on disk.
 *
 * @param sbi		Pointer to the superblock structure to write on disk.
 *
 * @return		EOK on success or an error code.
 */
static errno_t write_superblock(const struct mfs_sb_info *sbi)
{
	struct mfs_superblock *sb;
	errno_t rc;

	sb = malloc(MFS_SUPERBLOCK_SIZE);

	if (!sb)
		return ENOMEM;

	sb->s_ninodes = (uint16_t) sbi->n_inodes;
	sb->s_nzones = (uint16_t) sbi->n_zones;
	sb->s_nzones2 = (uint32_t) sbi->n_zones;
	sb->s_ibmap_blocks = (uint16_t) sbi->ibmap_blocks;
	sb->s_zbmap_blocks = (uint16_t) sbi->zbmap_blocks;
	sb->s_first_data_zone = (uint16_t) sbi->first_data_zone;
	sb->s_log2_zone_size = sbi->log2_zone_size;
	sb->s_max_file_size = sbi->max_file_size;
	sb->s_magic = sbi->magic;
	sb->s_state = MFS_VALID_FS;

	rc = write_block(MFS_SUPERBLOCK, 1, sb);
	free(sb);

	return rc;
}

/**Write the V3s superblock on disk.
 *
 * @param sbi		Pointer to the superblock structure to write on disk.
 *
 * @return		EOK on success or an error code.
 */
static errno_t write_superblock3(const struct mfs_sb_info *sbi)
{
	struct mfs3_superblock *sb;
	errno_t rc;

	sb = malloc(MFS_SUPERBLOCK_SIZE);

	if (!sb)
		return ENOMEM;

	sb->s_ninodes = (uint32_t) sbi->n_inodes;
	sb->s_nzones = (uint32_t) sbi->n_zones;
	sb->s_ibmap_blocks = (uint16_t) sbi->ibmap_blocks;
	sb->s_zbmap_blocks = (uint16_t) sbi->zbmap_blocks;
	sb->s_first_data_zone = (uint16_t) sbi->first_data_zone;
	sb->s_log2_zone_size = sbi->log2_zone_size;
	sb->s_max_file_size = sbi->max_file_size;
	sb->s_magic = sbi->magic;
	sb->s_block_size = sbi->block_size;
	sb->s_disk_version = 3;

	rc = block_write_direct(service_id, MFS_SUPERBLOCK << 1, 1 << 1, sb);
	free(sb);

	return rc;
}

/**Initialize the inode and block bitmaps on disk.
 *
 * @param sb		Pointer to the superblock structure.
 *
 * @return		EOK on success or an error code.
 */
static errno_t init_bitmaps(const struct mfs_sb_info *sb)
{
	uint32_t *ibmap_buf, *zbmap_buf;
	uint8_t *ibmap_buf8, *zbmap_buf8;
	const unsigned int ibmap_nblocks = sb->ibmap_blocks;
	const unsigned int zbmap_nblocks = sb->zbmap_blocks;
	unsigned int i;
	errno_t rc = EOK;

	ibmap_buf = malloc(ibmap_nblocks * sb->block_size);
	zbmap_buf = malloc(zbmap_nblocks * sb->block_size);

	if (!ibmap_buf || !zbmap_buf) {
		rc = ENOMEM;
		goto exit;
	}

	memset(ibmap_buf, 0xFF, ibmap_nblocks * sb->block_size);
	memset(zbmap_buf, 0xFF, zbmap_nblocks * sb->block_size);

	for (i = 2; i < sb->n_inodes + 1; ++i)
		mark_bmap(ibmap_buf, i, FREE);

	for (i = 2; i < sb->n_zones - sb->first_data_zone; ++i)
		mark_bmap(zbmap_buf, i, FREE);

	ibmap_buf8 = (uint8_t *) ibmap_buf;
	zbmap_buf8 = (uint8_t *) zbmap_buf;

	int start_block = 2;

	for (i = 0; i < ibmap_nblocks; ++i) {
		if ((rc = write_block(start_block + i,
		    1, (ibmap_buf8 + i * sb->block_size))) != EOK)
			goto exit;
	}

	start_block = 2 + ibmap_nblocks;

	for (i = 0; i < zbmap_nblocks; ++i) {
		if ((rc = write_block(start_block + i,
		    1, (zbmap_buf8 + i * sb->block_size))) != EOK)
			goto exit;
	}

exit:
	free(ibmap_buf);
	free(zbmap_buf);

	return rc;
}

/**Mark a bitmap entry as used or free.
 *
 * @param bmap		32-bit pointer to the bitmap in memory.
 * @param idx		The index in the bitmap of the bit to set at 1 or 0.
 * @param v		FREE to clear the bit, USED to set the bit.
 */
static void mark_bmap(uint32_t *bmap, int idx, int v)
{
	if (v == FREE)
		bmap[idx / 32] &= ~(1 << (idx % 32));
	else
		bmap[idx / 32] |= 1 << (idx % 32);
}

/**Write a block on disk.
 *
 * @param off		64-bit block offset on disk.
 * @param size		size of the block.
 * @param data		Pointer to the block content.
 *
 * @return		EOK on success or a error number.
 */
static inline errno_t write_block(aoff64_t off, size_t size, const void *data)
{
	if (shift == 3) {
		errno_t rc;
		aoff64_t tmp_off = off << 1;
		uint8_t *data_ptr = (uint8_t *) data;

		rc = block_write_direct(service_id, tmp_off << 2,
		    size << 2, data_ptr);

		if (rc != EOK)
			return rc;

		data_ptr += 2048;
		tmp_off++;

		return block_write_direct(service_id, tmp_off << 2,
		    size << 2, data_ptr);
	}
	return block_write_direct(service_id, off << shift,
	    size << shift, data);
}

static void help_cmd_mkmfs(help_level_t level)
{
	if (level == HELP_SHORT) {
		printf(NAME": tool to create new Minix file systems\n");
	} else {
		printf("Usage: [options] device\n"
		    "-1         Make a Minix version 1 filesystem\n"
		    "-2         Make a Minix version 2 filesystem\n"
		    "-b ##      Specify the block size in bytes (V3 only),\n"
		    "           valid block size values are 1024, 2048 and"
				" 4096 bytes per block\n"
		    "-i ##      Specify the number of inodes"
				" for the filesystem\n"
		    "-l         Use 30-char long filenames (V1/V2 only)\n");
	}
}

/** Check if a given number is a power of two.
 *
 * @param n	The number to check.
 *
 * @return	true if it is a power of two, false otherwise.
 */
static bool is_power_of_two(uint32_t n)
{
	if (n == 0)
		return false;

	return (n & (n - 1)) == 0;
}


/**
 * @}
 */
