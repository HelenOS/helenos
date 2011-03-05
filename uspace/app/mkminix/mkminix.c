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
#include <fs/minix.h>

#define NAME	"mkminix"

typedef enum {
	HELP_SHORT,
	HELP_LONG
} help_level_t;

typedef struct mfs_params {
	uint16_t fs_magic;
	uint32_t block_size;
	size_t devblock_size;
	unsigned long n_inodes;
	aoff64_t dev_nblocks;
	bool fs_longnames;
} mfs_params_t;

static void	help_cmd_mkminix(help_level_t level);
static int	num_of_set_bits(uint32_t n);
static void	prepare_superblock(struct mfs_superblock *sb, mfs_params_t *opt);
static void	prepare_superblock_v3(struct mfs3_superblock *sb, mfs_params_t *opt);

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
	devmap_handle_t handle;
	
	struct mfs_superblock *sb;
	struct mfs3_superblock *sb3;

	mfs_params_t opt;

	/*Default is MinixFS V3*/
	opt.fs_magic = MFS_MAGIC_V3;

	/*Default block size is 4Kb*/
	opt.block_size = 4096;
	opt.n_inodes = 0;
	opt.fs_longnames = false;

	if (argc == 1) {
		help_cmd_mkminix(HELP_SHORT);
		printf("Incorrect number of arguments, try `mkminix --help'\n");
		exit(0);
	}

	for (c = 0, optind = 0, opt_ind = 0; c != -1;) {
		c = getopt_long(argc, argv, "eh123b:i:", long_options, &opt_ind);
		switch (c) {
		case 'h':
			help_cmd_mkminix(HELP_LONG);
			exit(0);
		case '1':
			opt.fs_magic = MFS_MAGIC_V1;
			opt.block_size = MFS_MIN_BLOCK_SIZE;
			break;
		case '2':
			opt.fs_magic = MFS_MAGIC_V2;
			opt.block_size = MFS_MIN_BLOCK_SIZE;
			break;
		case '3':
			opt.fs_magic = MFS_MAGIC_V3;
			break;
		case 'b':
			opt.block_size = (uint32_t) strtol(optarg, NULL, 10);
			break;
		case 'i':
			opt.n_inodes = (unsigned long) strtol(optarg, NULL, 10);
			break;
		case 'l':
			opt.fs_longnames = true;
			break;
		}
	}

	if (opt.block_size < MFS_MIN_BLOCK_SIZE || 
				opt.block_size > MFS_MAX_BLOCK_SIZE) {
		printf(NAME ":Error! Invalid block size.\n");
		exit(0);
	} else if (num_of_set_bits(opt.block_size) != 1) {
		/*Block size must be a power of 2.*/
		printf(NAME ":Error! Invalid block size.\n");
		exit(0);
	} else if (opt.block_size > MFS_MIN_BLOCK_SIZE && 
			opt.fs_magic != MFS_MAGIC_V3) {
		printf(NAME ":Error! Block size > 1024 is supported by V3 filesystem only.\n");
		exit(0);
	} else if (opt.fs_magic == MFS_MAGIC_V3 && opt.fs_longnames) {
		printf(NAME ":Error! Long filenames are supported by V1/V2 filesystem only.\n");
		exit(0);
	}

	argv += optind;

	device_name = argv[0];

	if (!device_name) {
		help_cmd_mkminix(HELP_LONG);
		exit(0);
	}

	rc = devmap_device_get_handle(device_name, &handle, 0);
	if (rc != EOK) {
		printf(NAME ": Error resolving device `%s'.\n", device_name);
		return 2;
	}

	rc = block_init(handle, MFS_MIN_BLOCK_SIZE);
	if (rc != EOK)  {
		printf(NAME ": Error initializing libblock.\n");
		return 2;
	}

	rc = block_get_bsize(handle, &opt.devblock_size);
	if (rc != EOK) {
		printf(NAME ": Error determining device block size.\n");
		return 2;
	}

	rc = block_get_nblocks(handle, &opt.dev_nblocks);
	if (rc != EOK) {
		printf(NAME ": Warning, failed to obtain block device size.\n");
	} else {
		printf(NAME ": Block device has %" PRIuOFF64 " blocks.\n",
		    opt.dev_nblocks);
	}

	if (opt.devblock_size != 512) {
		printf(NAME ": Error. Device block size is not 512 bytes.\n");
		return 2;
	}

	printf("Creating Minix file system on device\n");

	/*Prepare superblock*/

	if (opt.fs_magic == MFS_MAGIC_V3) {
		sb3 = (struct mfs3_superblock *) malloc(sizeof(struct mfs3_superblock));
		if (!sb3) {
			printf(NAME ": Error, not enough memory");
			return 2;
		}
		prepare_superblock_v3(sb3, &opt);
	} else {
		sb = (struct mfs_superblock *) malloc(sizeof(struct mfs_superblock));
		if (!sb) {
			printf(NAME ": Error, not enough memory");
			return 2;
		}
		prepare_superblock(sb, &opt);
	}

	return 0;
}

static void prepare_superblock(struct mfs_superblock *sb, mfs_params_t *opt)
{
	if (opt->fs_longnames) {
		if (opt->fs_magic == MFS_MAGIC_V1)
			opt->fs_magic = MFS_MAGIC_V1L;
		else
			opt->fs_magic = MFS_MAGIC_V2L;
	}

	sb->s_magic = opt->fs_magic;

	/*Valid only for MFS V1*/
	sb->s_nzones = opt->dev_nblocks > UINT16_MAX ? 
			UINT16_MAX : opt->dev_nblocks;

	/*Valid only for MFS V2*/
	sb->s_nzones2 = opt->dev_nblocks > UINT32_MAX ?
			UINT32_MAX : opt->dev_nblocks;

	if (opt->n_inodes == 0) {
		aoff64_t tmp = opt->dev_nblocks / 3;		
		sb->s_ninodes = tmp > UINT16_MAX ? UINT16_MAX : tmp;
	} else {
		sb->s_ninodes = opt->n_inodes;
	}

	sb->s_log2_zone_size = 0;

}

static void prepare_superblock_v3(struct mfs3_superblock *sb, mfs_params_t *opt)
{
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
