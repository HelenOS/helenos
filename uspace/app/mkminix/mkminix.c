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
#include "mfs.h"

#define NAME	"mkminix"

typedef enum {
	HELP_SHORT,
	HELP_LONG
} help_level_t;

static void help_cmd_mkminix(help_level_t level);
static int num_of_set_bits(uint32_t n);

static struct option const long_options[] = {
	{ "help", no_argument, 0, 'h' },
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
	aoff64_t dev_nblocks;

	/*Default is MinixFS version 3*/
	mfs_version_t fs_version = MFS_VERSION_V3;

	/*Default block size is 4096 bytes*/
	uint32_t block_size = MFS_MAX_BLOCK_SIZE;
	size_t devblock_size;
	unsigned long n_inodes = 0;

	struct mfs_superblock *sb;
	sb = (struct mfs_superblock *) malloc(sizeof(struct mfs_superblock));

	if (argc == 1) {
		help_cmd_mkminix(HELP_SHORT);
		printf("Incorrect number of arguments, try `mkminix --help'\n");
		exit(0);
	}

	for (c = 0, optind = 0, opt_ind = 0; c != -1;) {
		c = getopt_long(argc, argv, "h123b:i:", long_options, &opt_ind);
		switch (c) {
		case 'h':
			help_cmd_mkminix(HELP_LONG);
			exit(0);
		case '1':
			fs_version = MFS_VERSION_V1;
			block_size = MFS_MIN_BLOCK_SIZE;
			break;
		case '2':
			fs_version = MFS_VERSION_V2;
			block_size = MFS_MIN_BLOCK_SIZE;
			break;
		case '3':
			fs_version = MFS_VERSION_V3;
			break;
		case 'b':
			block_size = (uint32_t) strtol(optarg, NULL, 10);
			break;
		case 'i':
			n_inodes = (unsigned long) strtol(optarg, NULL, 10);
			break;
		}
	}

	if (block_size < MFS_MIN_BLOCK_SIZE || block_size > MFS_MAX_BLOCK_SIZE) {
		printf(NAME ":Error! Invalid block size.\n");
		exit(0);
	} else if (num_of_set_bits(block_size) != 1) {
		/*Block size must be a power of 2.*/
		printf(NAME ":Error! Invalid block size.\n");
		exit(0);
	} else if (block_size > MFS_MIN_BLOCK_SIZE && 
			fs_version != MFS_VERSION_V3) {
		printf(NAME ":Error! Block size > 1024 is supported from V3 filesystem only.\n");
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

	rc = block_get_bsize(handle, &devblock_size);
	if (rc != EOK) {
		printf(NAME ": Error determining device block size.\n");
		return 2;
	}

	rc = block_get_nblocks(handle, &dev_nblocks);
	if (rc != EOK) {
		printf(NAME ": Warning, failed to obtain block device size.\n");
	} else {
		printf(NAME ": Block device has %" PRIuOFF64 " blocks.\n",
		    dev_nblocks);
	}

	if (devblock_size != 512) {
		printf(NAME ": Error. Device block size is not 512 bytes.\n");
		return 2;
	}

	printf("Creating Minix file system on device\n");

	/*Prepare superblock*/

	return 0;
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
		"-i ##      Specify the number of inodes for the filesystem\n");
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
