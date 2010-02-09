/*
 * Copyright (c) 2010 Jiri Svoboda
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
 * @file	mkfat.c
 * @brief	Tool for creating new FAT file systems.
 *
 * Currently we can only create 16-bit FAT.
 */

#include <stdio.h>
#include <stdlib.h>
#include <libblock.h>
#include <mem.h>
#include <devmap.h>
#include <byteorder.h>
#include <sys/types.h>
#include <sys/typefmt.h>
#include <inttypes.h>
#include <errno.h>
#include "fat.h"

#define NAME	"mkfat"

/** Divide and round up. */
#define div_round_up(a, b) (((a) + (b) - 1) / (b))

/** Predefined file-system parameters */
enum {
	sector_size		= 512,
	sectors_per_cluster	= 8,
	fat_count		= 2,
	reserved_clusters	= 2,
	media_descriptor	= 0xF8 /**< fixed disk */
};

/** Configurable file-system parameters */
typedef struct fat_cfg {
	uint32_t total_sectors;
	uint16_t root_ent_max;
	uint16_t addt_res_sectors;
} fat_cfg_t;

/** Derived file-system parameters */
typedef struct fat_params {
	struct fat_cfg cfg;
	uint16_t reserved_sectors;
	uint16_t rootdir_sectors;
	uint32_t fat_sectors;
	uint16_t total_clusters;
} fat_params_t;

static void syntax_print(void);

static int fat_params_compute(struct fat_cfg const *cfg,
    struct fat_params *par);
static int fat_blocks_write(struct fat_params const *par,
    dev_handle_t handle);
static void fat_bootsec_create(struct fat_params const *par, struct fat_bs *bs);

int main(int argc, char **argv)
{
	struct fat_params par;
	struct fat_cfg cfg;

	int rc;
	char *dev_path;
	dev_handle_t handle;
	size_t block_size;
	char *endptr;
	bn_t dev_nblocks;

	cfg.total_sectors = 0;
	cfg.addt_res_sectors = 0;
	cfg.root_ent_max = 128;

	if (argc < 2) {
		printf(NAME ": Error, argument missing.\n");
		syntax_print();
		return 1;
	}

	--argc; ++argv;

	if (str_cmp(*argv, "--size") == 0) {
		--argc; ++argv;
		if (*argv == NULL) {
			printf(NAME ": Error, argument missing.\n");
			syntax_print();
			return 1;
		}

		cfg.total_sectors = strtol(*argv, &endptr, 10);
		if (*endptr != '\0') {
			printf(NAME ": Error, invalid argument.\n");
			syntax_print();
			return 1;
		}

		--argc; ++argv;
	}

	if (argc != 1) {
		printf(NAME ": Error, unexpected argument.\n");
		syntax_print();
		return 1;
	}

	dev_path = *argv;

	rc = devmap_device_get_handle(dev_path, &handle, 0);
	if (rc != EOK) {
		printf(NAME ": Error resolving device `%s'.\n", dev_path);
		return 2;
	}

	rc = block_init(handle, 2048);
	if (rc != EOK)  {
		printf(NAME ": Error initializing libblock.\n");
		return 2;
	}

	rc = block_get_bsize(handle, &block_size);
	if (rc != EOK) {
		printf(NAME ": Error determining device block size.\n");
		return 2;
	}

	rc = block_get_nblocks(handle, &dev_nblocks);
	if (rc != EOK) {
		printf(NAME ": Warning, failed to obtain block device size.\n");
	} else {
		printf(NAME ": Block device has %" PRIuBN " blocks.\n",
		    dev_nblocks);
		cfg.total_sectors = dev_nblocks;
	}

	if (block_size != 512) {
		printf(NAME ": Error. Device block size is not 512 bytes.\n");
		return 2;
	}

	if (cfg.total_sectors == 0) {
		printf(NAME ": Error. You must specify filesystem size.\n");
		return 1;
	}

	printf(NAME ": Creating FAT filesystem on device %s.\n", dev_path);

	rc = fat_params_compute(&cfg, &par);
	if (rc != EOK) {
		printf(NAME ": Invalid file-system parameters.\n");
		return 2;
	}

	rc = fat_blocks_write(&par, handle);
	if (rc != EOK) {
		printf(NAME ": Error writing device.\n");
		return 2;
	}

	block_fini(handle);
	printf("Success.\n");

	return 0;
}

static void syntax_print(void)
{
	printf("syntax: mkfat [--size <num_blocks>] <device_name>\n");
}

/** Derive sizes of different filesystem structures.
 *
 * This function concentrates all the different computations of FAT
 * file system params.
 */
static int fat_params_compute(struct fat_cfg const *cfg, struct fat_params *par)
{
	uint32_t fat_bytes;
	uint32_t non_data_sectors_lb;

	/*
         * Make a conservative guess on the FAT size needed for the file
         * system. The optimum could be potentially smaller since we
         * do not subtract size of the FAT itself when computing the
         * size of the data region.
         */

	par->reserved_sectors = 1 + cfg->addt_res_sectors;
	par->rootdir_sectors = div_round_up(cfg->root_ent_max * DIRENT_SIZE,
	    sector_size);
	non_data_sectors_lb = par->reserved_sectors + par->rootdir_sectors;

	par->total_clusters = div_round_up(cfg->total_sectors - non_data_sectors_lb,
	    sectors_per_cluster);

	fat_bytes = (par->total_clusters + 2) * 2;
	par->fat_sectors = div_round_up(fat_bytes, sector_size);

	par->cfg = *cfg;

	return EOK;
}

/** Create file system with the given parameters. */
static int fat_blocks_write(struct fat_params const *par, dev_handle_t handle)
{
	bn_t addr;
	uint8_t *buffer;
	int i;
	uint32_t j;
	int rc;
	struct fat_bs bs;

	fat_bootsec_create(par, &bs);

	rc = block_write_direct(handle, BS_BLOCK, 1, &bs);
	if (rc != EOK)
		return EIO;

	addr = BS_BLOCK + 1;

	buffer = calloc(sector_size, 1);
	if (buffer == NULL)
		return ENOMEM;

	/* Reserved sectors */
	for (i = 0; i < par->reserved_sectors - 1; ++i) {
		rc = block_write_direct(handle, addr, 1, buffer);
		if (rc != EOK)
			return EIO;

		++addr;
	}

	/* File allocation tables */
	for (i = 0; i < fat_count; ++i) {
		printf("Writing allocation table %d.\n", i + 1);

		for (j = 0; j < par->fat_sectors; ++j) {
			memset(buffer, 0, sector_size);
			if (j == 0) {
				buffer[0] = media_descriptor;
				buffer[1] = 0xFF;
				buffer[2] = 0xFF;
				buffer[3] = 0xFF;
			}

			rc = block_write_direct(handle, addr, 1, buffer);
			if (rc != EOK)
				return EIO;

			++addr;
		}
	}

	printf("Writing root directory.\n");

	memset(buffer, 0, sector_size);

	/* Root directory */
	for (i = 0; i < par->rootdir_sectors; ++i) {
		rc = block_write_direct(handle, addr, 1, buffer);
		if (rc != EOK)
			return EIO;

		++addr;
	}

	free(buffer);

	return EOK;
}

/** Construct boot sector with the given parameters. */
static void fat_bootsec_create(struct fat_params const *par, struct fat_bs *bs)
{
	memset(bs, 0, sizeof(*bs));

	bs->ji[0] = 0xEB;
	bs->ji[1] = 0x3C;
	bs->ji[2] = 0x90;

	memcpy(bs->oem_name, "HELENOS ", 8);

	/* BIOS Parameter Block */
	bs->bps = host2uint16_t_le(sector_size);
	bs->spc = sectors_per_cluster;
	bs->rscnt = host2uint16_t_le(par->reserved_sectors);
	bs->fatcnt = fat_count;
	bs->root_ent_max = host2uint16_t_le(par->cfg.root_ent_max);

	if (par->cfg.total_sectors < 0x10000)
		bs->totsec16 = host2uint16_t_le(par->cfg.total_sectors);
	else
		bs->totsec16 = host2uint16_t_le(0);

	bs->mdesc = media_descriptor;
	bs->sec_per_fat = host2uint16_t_le(par->fat_sectors);
	bs->sec_per_track = host2uint16_t_le(63);
	bs->headcnt = host2uint16_t_le(6);
	bs->hidden_sec = host2uint32_t_le(0);

	if (par->cfg.total_sectors >= 0x10000)
		bs->totsec32 = host2uint32_t_le(par->cfg.total_sectors);
	else
		bs->totsec32 = host2uint32_t_le(0);

	/* Extended BPB */
	bs->pdn = 0x80;
	bs->ebs = 0x29;
	bs->id = host2uint32_t_be(0x12345678);

	memcpy(bs->label, "HELENOS_NEW", 11);
	memcpy(bs->type, "FAT16   ", 8);
	bs->signature = host2uint16_t_be(0x55AA);
}

/**
 * @}
 */
