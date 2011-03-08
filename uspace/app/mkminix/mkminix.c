/*
 * Copyright (c) 2010 Jiri Svoboda
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
 * @file	mkminix.c
 * @brief	Tool for creating new Minix file systems.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <libblock.h>
#include <unistd.h>
#include <errno.h>
#include <sys/typefmt.h>
#include <inttypes.h>
#include <str.h>
#include <getopt.h>
#include <mem.h>
#include <minix.h>

#define NAME	"mkminix"

#define FREE	0
#define USED	1

#define UPPER(n, size) (((n) / (size)) + (((n) % (size)) != 0))

typedef enum {
	HELP_SHORT,
	HELP_LONG
} help_level_t;

/*Generic MFS superblock*/
struct mfs_sb_info {
	devmap_handle_t handle;
	uint64_t n_inodes;
	uint64_t n_zones;
	aoff64_t dev_nblocks;
	aoff64_t devblock_size;
	unsigned long ibmap_blocks;
	unsigned long zbmap_blocks;
	unsigned long first_data_zone;
	unsigned long itable_size;
	int log2_zone_size;
	int ino_per_block;
	uint32_t max_file_size;
	uint16_t magic;
	uint32_t block_size;
	int fs_version;
	bool longnames;
};

static void	help_cmd_mkminix(help_level_t level);
static int	num_of_set_bits(uint32_t n);
static void	init_superblock(struct mfs_sb_info *sb);
static void	write_superblock(struct mfs_sb_info *sbi);
static void	write_superblock3(struct mfs_sb_info *sbi);
static void	init_bitmaps(struct mfs_sb_info *sb);
static void	init_inode_table(struct mfs_sb_info *sb);
static void	make_root_ino(struct mfs_sb_info *sb);
static void	make_root_ino3(struct mfs_sb_info *sb);
static void	mark_bmap(uint32_t *bmap, int idx, int v);

static struct option const long_options[] = {
	{ "help", no_argument, 0, 'h' },
	{ "long-names", no_argument, 0, 'l' },
	{ "blocks", required_argument, 0, 'b' },
	{ "inodes", required_argument, 0, 'i' },
	{ NULL, no_argument, 0, '1' },
	{ NULL, no_argument, 0, '2' },
	{ NULL, no_argument, 0, '3' },
	{ 0, 0, 0, 0 }
};

int main (int argc, char **argv)
{
	int rc, c, opt_ind;
	char *device_name;

	struct mfs_sb_info sb;

	/*Default is MinixFS V3*/
	sb.magic = MFS_MAGIC_V3;

	/*Default block size is 4Kb*/
	sb.block_size = MFS_MAX_BLOCKSIZE;
	sb.n_inodes = 0;
	sb.longnames = false;
	sb.ino_per_block = V3_INODES_PER_BLOCK(MFS_MAX_BLOCKSIZE);

	if (argc == 1) {
		help_cmd_mkminix(HELP_SHORT);
		printf("Incorrect number of arguments, try `mkminix --help'\n");
		exit(0);
	}

	for (c = 0, optind = 0, opt_ind = 0; c != -1;) {
		c = getopt_long(argc, argv, "lh123b:i:", long_options, &opt_ind);
		switch (c) {
		case 'h':
			help_cmd_mkminix(HELP_LONG);
			exit(0);
		case '1':
			sb.magic = MFS_MAGIC_V1;
			sb.block_size = MFS_BLOCKSIZE;
			sb.fs_version = 1;
			sb.ino_per_block = V1_INODES_PER_BLOCK;
			break;
		case '2':
			sb.magic = MFS_MAGIC_V2;
			sb.block_size = MFS_BLOCKSIZE;
			sb.fs_version = 2;
			sb.ino_per_block = V2_INODES_PER_BLOCK;
			break;
		case '3':
			sb.magic = MFS_MAGIC_V3;
			sb.fs_version = 3;
			sb.block_size = MFS_MAX_BLOCKSIZE;
			break;
		case 'b':
			sb.block_size = (uint32_t) strtol(optarg, NULL, 10);
			break;
		case 'i':
			sb.n_inodes = (unsigned long) strtol(optarg, NULL, 10);
			break;
		case 'l':
			sb.longnames = true;
			break;
		}
	}

	if (sb.block_size < MFS_MIN_BLOCKSIZE || 
				sb.block_size > MFS_MAX_BLOCKSIZE) {
		printf(NAME ":Error! Invalid block size.\n");
		exit(0);
	} else if (num_of_set_bits(sb.block_size) != 1) {
		/*Block size must be a power of 2.*/
		printf(NAME ":Error! Invalid block size.\n");
		exit(0);
	} else if (sb.block_size > MFS_BLOCKSIZE && 
			sb.fs_version != 3) {
		printf(NAME ":Error! Block size > 1024 is supported by V3 filesystem only.\n");
		exit(0);
	} else if (sb.fs_version == 3 && sb.longnames) {
		printf(NAME ":Error! Long filenames are supported by V1/V2 filesystem only.\n");
		exit(0);
	}

	argv += optind;

	device_name = argv[0];

	if (!device_name) {
		help_cmd_mkminix(HELP_LONG);
		exit(0);
	}

	rc = devmap_device_get_handle(device_name, &sb.handle, 0);
	if (rc != EOK) {
		printf(NAME ": Error resolving device `%s'.\n", device_name);
		return 2;
	}

	rc = block_init(sb.handle, MFS_MIN_BLOCKSIZE);
	if (rc != EOK)  {
		printf(NAME ": Error initializing libblock.\n");
		return 2;
	}

	rc = block_get_bsize(sb.handle, &sb.devblock_size);
	if (rc != EOK) {
		printf(NAME ": Error determining device block size.\n");
		return 2;
	}

	rc = block_get_nblocks(sb.handle, &sb.dev_nblocks);
	if (rc != EOK) {
		printf(NAME ": Warning, failed to obtain block device size.\n");
	} else {
		printf(NAME ": Block device has %" PRIuOFF64 " blocks.\n",
		    sb.dev_nblocks);
	}

	if (sb.devblock_size != 512) {
		printf(NAME ": Error. Device block size is not 512 bytes.\n");
		return 2;
	}

	/*Minimum block size is 1 Kb*/
	sb.dev_nblocks /= 2;

	printf(NAME ": Creating Minix file system on device\n");

	/*Initialize superblock*/
	init_superblock(&sb);

	/*Initialize bitmaps*/
	init_bitmaps(&sb);

	/*Init inode table*/
	init_inode_table(&sb);

	return 0;
}

static void init_inode_table(struct mfs_sb_info *sb)
{
	unsigned int i;
	uint8_t *itable_buf;
	long itable_pos = 2 + sb->zbmap_blocks + sb->ibmap_blocks;

	itable_buf = malloc(sb->block_size);
	memset(itable_buf, 0x00, sb->block_size);

	const int chunks = sb->block_size / MFS_BLOCKSIZE;

	for (i = 0; i < sb->itable_size; ++i, ++itable_pos)
		block_write_direct(sb->handle, itable_pos, chunks, itable_buf);

	free(itable_buf);

	/*Make the root inode*/
	if (sb->fs_version != 3)
		make_root_ino(sb);		
	else
		make_root_ino3(sb);
}

static void make_root_ino(struct mfs_sb_info *sb)
{
	struct mfs_inode *ino_buf;
	const size_t bufsize = MFS_BLOCKSIZE;
	const long itable_pos = 2 + sb->zbmap_blocks + sb->ibmap_blocks;

	ino_buf = (struct mfs_inode *) malloc(bufsize);
	memset(ino_buf, 0x00, bufsize);

	ino_buf[MFS_ROOT_INO].i_mode = S_IFDIR;
	ino_buf[MFS_ROOT_INO].i_uid = 0;
	ino_buf[MFS_ROOT_INO].i_gid = 0;
	ino_buf[MFS_ROOT_INO].i_size = (sb->longnames ? MFSL_DIRSIZE : MFS_DIRSIZE) * 2;
	ino_buf[MFS_ROOT_INO].i_mtime = 0;
	ino_buf[MFS_ROOT_INO].i_nlinks = 2;
	ino_buf[MFS_ROOT_INO].i_dzone[0] = sb->first_data_zone;

	block_write_direct(sb->handle, itable_pos, 1, ino_buf);

	free(ino_buf);
}

static void make_root_ino3(struct mfs_sb_info *sb)
{
	struct mfs2_inode *ino_buf;
	const size_t bufsize = MFS_MIN_BLOCKSIZE;
	const long itable_pos = 2 + sb->zbmap_blocks + sb->ibmap_blocks;

	ino_buf = (struct mfs2_inode *) malloc(bufsize);
	memset(ino_buf, 0x00, bufsize);

	ino_buf[MFS_ROOT_INO].i_mode = S_IFDIR;
	ino_buf[MFS_ROOT_INO].i_uid = 0;
	ino_buf[MFS_ROOT_INO].i_gid = 0;
	ino_buf[MFS_ROOT_INO].i_size = MFS3_DIRSIZE * 2;
	ino_buf[MFS_ROOT_INO].i_mtime = 0;
	ino_buf[MFS_ROOT_INO].i_atime = 0;
	ino_buf[MFS_ROOT_INO].i_ctime = 0;
	ino_buf[MFS_ROOT_INO].i_nlinks = 2;
	ino_buf[MFS_ROOT_INO].i_dzone[0] = sb->first_data_zone;

	block_write_direct(sb->handle, itable_pos * (sb->block_size / MFS_MIN_BLOCKSIZE), 1, ino_buf);

	free(ino_buf);
}

static void init_superblock(struct mfs_sb_info *sb)
{
	aoff64_t inodes;

	if (sb->longnames)
		sb->magic = sb->fs_version == 1 ? MFS_MAGIC_V1L : MFS_MAGIC_V2L;

	/*Compute the number of zones on disk*/

	if (sb->fs_version == 1) {
		/*Valid only for MFS V1*/
		sb->n_zones = sb->dev_nblocks > UINT16_MAX ? 
			UINT16_MAX : sb->dev_nblocks;
	} else {
		/*Valid for MFS V2/V3*/
		sb->n_zones = sb->dev_nblocks > UINT32_MAX ?
			UINT32_MAX : sb->dev_nblocks;

		if (sb->fs_version == 3) {
			sb->ino_per_block = V3_INODES_PER_BLOCK(sb->block_size);
			sb->n_zones /= (sb->block_size / MFS_MIN_BLOCKSIZE);
		}
	}

	/*Round up the number of inodes to fill block size*/
	if (sb->n_inodes == 0)
		inodes = sb->dev_nblocks / 3;

	if (inodes % sb->ino_per_block)
		inodes = ((inodes / sb->ino_per_block) + 1) * sb->ino_per_block;
	
	if (sb->fs_version < 3)
		sb->n_inodes = inodes > UINT16_MAX ? UINT16_MAX : inodes;
	else
		sb->n_inodes = inodes > UINT32_MAX ? UINT32_MAX : inodes;

	/*Compute inode bitmap size in blocks*/
	sb->ibmap_blocks = UPPER(sb->n_inodes, sb->block_size * 8);

	/*Compute zone bitmap size in blocks*/
	sb->zbmap_blocks = UPPER(sb->n_zones, sb->block_size * 8);

	/*Compute inode table size*/
	sb->itable_size = sb->n_inodes / sb->ino_per_block;

	/*Compute first data zone position*/
	sb->first_data_zone = 2 + sb->itable_size + 
				sb->zbmap_blocks + sb->ibmap_blocks;

	/*Set log2 of zone to block ratio to zero*/
	sb->log2_zone_size = 0;

	/*Superblock is now ready to be written on disk*/
	printf(NAME ": %d inodes\n", (uint32_t) sb->n_inodes);
	printf(NAME ": %d zones\n", (uint32_t) sb->n_zones);
	printf(NAME ": inode table blocks = %ld\n", sb->itable_size);
	printf(NAME ": inode bitmap blocks = %ld\n", sb->ibmap_blocks);
	printf(NAME ": zone bitmap blocks = %ld\n", sb->zbmap_blocks);
	printf(NAME ": first data zone = %d\n", (uint32_t) sb->first_data_zone);
	printf(NAME ": long fnames = %s\n", sb->longnames ? "Yes" : "No");

	if (sb->fs_version == 3)
		write_superblock3(sb);
	else
		write_superblock(sb);
}

static void write_superblock(struct mfs_sb_info *sbi)
{
	struct mfs_superblock *sb;
	uint8_t *superblock_buf;

	superblock_buf = malloc(1024);

	sb = (struct mfs_superblock *) superblock_buf;

	sb->s_ninodes = (uint16_t) sbi->n_inodes;
	sb->s_nzones = (uint16_t) sbi->n_zones;
	sb->s_nzones2 = (uint32_t) sbi->n_zones;
	sb->s_ibmap_blocks = (uint16_t) sbi->ibmap_blocks;
	sb->s_zbmap_blocks = (uint16_t) sbi->zbmap_blocks;
	sb->s_first_data_zone = (uint16_t) sbi->first_data_zone;
	sb->s_log2_zone_size = sbi->log2_zone_size;
	sb->s_max_file_size = UINT32_MAX;
	sb->s_magic = sbi->magic;
	sb->s_state = MFS_VALID_FS;

	block_write_direct(sbi->handle, MFS_SUPERBLOCK, 1, sb);
	free(superblock_buf);
}

static void write_superblock3(struct mfs_sb_info *sbi)
{
	struct mfs3_superblock *sb;

	sb = (struct mfs3_superblock *) malloc(MFS_SUPERBLOCK_SIZE);

	sb->s_ninodes = (uint32_t) sbi->n_inodes;
	sb->s_nzones = (uint32_t) sbi->n_zones;
	sb->s_ibmap_blocks = (uint16_t) sbi->ibmap_blocks;
	sb->s_zbmap_blocks = (uint16_t) sbi->zbmap_blocks;
	sb->s_first_data_zone = (uint16_t) sbi->first_data_zone;
	sb->s_log2_zone_size = sbi->log2_zone_size;
	sb->s_max_file_size = UINT32_MAX;
	sb->s_magic = sbi->magic;
	sb->s_block_size = sbi->block_size;
	sb->s_disk_version = 3;

	block_write_direct(sbi->handle, MFS_SUPERBLOCK, 1, sb);
	free(sb);
}

static void init_bitmaps(struct mfs_sb_info *sb)
{
	uint32_t *ibmap_buf, *zbmap_buf;
	int ibmap_nblocks = 1 + (sb->n_inodes / 8) / sb->block_size;
	int zbmap_nblocks = 1 + (sb->n_zones / 8) / sb->block_size;
	unsigned int i;

	ibmap_buf = (uint32_t *) malloc(ibmap_nblocks * sb->block_size);
	zbmap_buf = (uint32_t *) malloc(zbmap_nblocks * sb->block_size);

	memset(ibmap_buf, 0xFF, ibmap_nblocks * sb->block_size);
	memset(zbmap_buf, 0xFF, zbmap_nblocks * sb->block_size);

	for (i = 2; i < sb->n_inodes; ++i)
		mark_bmap(ibmap_buf, i, FREE);

	for (i = 2; i < sb->n_zones; ++i)
		mark_bmap(zbmap_buf, i, FREE);

	ibmap_nblocks *= sb->block_size / MFS_BLOCKSIZE;
	zbmap_nblocks *= sb->block_size / MFS_BLOCKSIZE;

	block_write_direct(sb->handle, 2, ibmap_nblocks, ibmap_buf);
	block_write_direct(sb->handle, 2 + ibmap_nblocks, zbmap_nblocks, zbmap_buf);
}

static void mark_bmap(uint32_t *bmap, int idx, int v)
{
	if (v == FREE)
		bmap[idx / 32] &= ~(1 << (idx % 32));
	else
		bmap[idx / 32] |= 1 << (idx % 32);
}

static void help_cmd_mkminix(help_level_t level)
{
	if (level == HELP_SHORT) {
		printf(NAME": tool to create new Minix file systems\n");
	} else {
	printf("Usage: [options] device\n"
		"-1         Make a Minix version 1 filesystem\n"
		"-2         Make a Minix version 2 filesystem\n"
		"-3         Make a Minix version 3 filesystem\n"
		"-b ##      Specify the block size in bytes (V3 only),\n"
		"           valid block size values are 1024, 2048 and 4096 bytes per block\n"
		"-i ##      Specify the number of inodes for the filesystem\n"
		"-l         Use 30-char long filenames (V1/V2 only)\n");
	}
}

static int num_of_set_bits(uint32_t n)
{
	n = n - ((n >> 1) & 0x55555555);
	n = (n & 0x33333333) + ((n >> 2) & 0x33333333);
	return (((n + (n >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;
}


/**
 * @}
 */
