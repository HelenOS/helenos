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
 * Currently we can create 12/16/32-bit FAT.
 */

#include <stdio.h>
#include <stdlib.h>
#include <block.h>
#include <mem.h>
#include <loc.h>
#include <byteorder.h>
#include <sys/types.h>
#include <sys/typefmt.h>
#include <inttypes.h>
#include <errno.h>
#include "fat.h"

#define NAME	"mkfat"

/** Divide and round up. */
#define div_round_up(a, b) (((a) + (b) - 1) / (b))

/** Default file-system parameters */
enum {
	default_sector_size		= 512,
	default_sectors_per_cluster	= 4,
	default_fat_count		= 2,
	default_reserved_clusters	= 2,
	default_media_descriptor	= 0xF8 /**< fixed disk */
};

/** Configurable file-system parameters */
typedef struct fat_cfg {
	int fat_type; /* FAT12 = 12, FAT16 = 16, FAT32 = 32 */
	size_t sector_size;
	uint32_t total_sectors;
	uint16_t root_ent_max;
	uint32_t addt_res_sectors;
	uint8_t sectors_per_cluster;

	uint16_t reserved_sectors;
	uint32_t rootdir_sectors;
	uint32_t fat_sectors;
	uint32_t total_clusters;
	uint8_t fat_count;
} fat_cfg_t;

static void syntax_print(void);

static int fat_params_compute(struct fat_cfg *cfg);
static int fat_blocks_write(struct fat_cfg const *cfg, service_id_t service_id);
static void fat_bootsec_create(struct fat_cfg const *cfg, struct fat_bs *bs);

int main(int argc, char **argv)
{
	struct fat_cfg cfg;

	int rc;
	char *dev_path;
	service_id_t service_id;
	char *endptr;
	aoff64_t dev_nblocks;

	cfg.sector_size = default_sector_size;
	cfg.sectors_per_cluster = default_sectors_per_cluster;
	cfg.fat_count = default_fat_count;
	cfg.total_sectors = 0;
	cfg.addt_res_sectors = 0;
	cfg.root_ent_max = 128;
	cfg.fat_type = FAT16;

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

	if (str_cmp(*argv, "--type") == 0) {
		--argc; ++argv;
		if (*argv == NULL) {
			printf(NAME ": Error, argument missing.\n");
			syntax_print();
			return 1;
		}

		cfg.fat_type = strtol(*argv, &endptr, 10);
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
	printf("Device: %s\n", dev_path);

	rc = loc_service_get_id(dev_path, &service_id, 0);
	if (rc != EOK) {
		printf(NAME ": Error resolving device `%s'.\n", dev_path);
		return 2;
	}

	rc = block_init(EXCHANGE_SERIALIZE, service_id, 2048);
	if (rc != EOK)  {
		printf(NAME ": Error initializing libblock.\n");
		return 2;
	}

	rc = block_get_bsize(service_id, &cfg.sector_size);
	if (rc != EOK) {
		printf(NAME ": Error determining device block size.\n");
		return 2;
	}

	rc = block_get_nblocks(service_id, &dev_nblocks);
	if (rc != EOK) {
		printf(NAME ": Warning, failed to obtain block device size.\n");
	} else {
		printf(NAME ": Block device has %" PRIuOFF64 " blocks.\n",
		    dev_nblocks);
		if (!cfg.total_sectors || dev_nblocks < cfg.total_sectors)
			cfg.total_sectors = dev_nblocks;
	}

	if (cfg.total_sectors == 0) {
		printf(NAME ": Error. You must specify filesystem size.\n");
		return 1;
	}

	if (cfg.fat_type != FAT12 && cfg.fat_type != FAT16 && cfg.fat_type != FAT32) {
		printf(NAME ": Error. Unknown FAT type.\n");
		return 2;
	}

	printf(NAME ": Creating FAT%d filesystem on device %s.\n", cfg.fat_type, dev_path);

	rc = fat_params_compute(&cfg);
	if (rc != EOK) {
		printf(NAME ": Invalid file-system parameters.\n");
		return 2;
	}

	rc = fat_blocks_write(&cfg, service_id);
	if (rc != EOK) {
		printf(NAME ": Error writing device.\n");
		return 2;
	}

	block_fini(service_id);
	printf("Success.\n");

	return 0;
}

static void syntax_print(void)
{
	printf("syntax: mkfat [--size <sectors>] [--type 12|16|32] <device_name>\n");
}

/** Derive sizes of different filesystem structures.
 *
 * This function concentrates all the different computations of FAT
 * file system params.
 */
static int fat_params_compute(struct fat_cfg *cfg)
{
	uint32_t fat_bytes;
	uint32_t non_data_sectors_lb;

	/*
	 * Make a conservative guess on the FAT size needed for the file
	 * system. The optimum could be potentially smaller since we
	 * do not subtract size of the FAT itself when computing the
	 * size of the data region.
	 */

	cfg->reserved_sectors = 1 + cfg->addt_res_sectors;
	if (cfg->fat_type != FAT32) {
		cfg->rootdir_sectors = div_round_up(cfg->root_ent_max * DIRENT_SIZE,
			cfg->sector_size);
	} else
		cfg->rootdir_sectors = 0;
	non_data_sectors_lb = cfg->reserved_sectors + cfg->rootdir_sectors;

	cfg->total_clusters = div_round_up(cfg->total_sectors - non_data_sectors_lb,
	    cfg->sectors_per_cluster);

	if ((cfg->fat_type == FAT12 && cfg->total_clusters > FAT12_CLST_MAX) ||
	    (cfg->fat_type == FAT16 && (cfg->total_clusters <= FAT12_CLST_MAX ||
	    cfg->total_clusters > FAT16_CLST_MAX)) ||
	    (cfg->fat_type == FAT32 && cfg->total_clusters <= FAT16_CLST_MAX))
		return ENOSPC;

	fat_bytes = div_round_up((cfg->total_clusters + 2) *
	    FAT_CLUSTER_DOUBLE_SIZE(cfg->fat_type), 2);
	cfg->fat_sectors = div_round_up(fat_bytes, cfg->sector_size);

	return EOK;
}

/** Create file system with the given parameters. */
static int fat_blocks_write(struct fat_cfg const *cfg, service_id_t service_id)
{
	aoff64_t addr;
	uint8_t *buffer;
	int i;
	uint32_t j;
	int rc;
	struct fat_bs bs;

	fat_bootsec_create(cfg, &bs);

	rc = block_write_direct(service_id, BS_BLOCK, 1, &bs);
	if (rc != EOK)
		return EIO;

	addr = BS_BLOCK + 1;

	buffer = calloc(cfg->sector_size, 1);
	if (buffer == NULL)
		return ENOMEM;
	memset(buffer, 0, cfg->sector_size);

	/* Reserved sectors */
	for (i = 0; i < cfg->reserved_sectors - 1; ++i) {
		rc = block_write_direct(service_id, addr, 1, buffer);
		if (rc != EOK)
			return EIO;

		++addr;
	}

	/* File allocation tables */
	for (i = 0; i < cfg->fat_count; ++i) {
		printf("Writing allocation table %d.\n", i + 1);

		for (j = 0; j < cfg->fat_sectors; ++j) {
			memset(buffer, 0, cfg->sector_size);
			if (j == 0) {
				buffer[0] = default_media_descriptor;
				buffer[1] = 0xFF;
				buffer[2] = 0xFF;
				if (cfg->fat_type == FAT16) {
					buffer[3] = 0xFF;
				} else if (cfg->fat_type == FAT32) {
					buffer[3] = 0x0F;
					buffer[4] = 0xFF;
					buffer[5] = 0xFF;
					buffer[6] = 0xFF;
					buffer[7] = 0x0F;
					buffer[8] = 0xF8;
					buffer[9] = 0xFF;
					buffer[10] = 0xFF;
					buffer[11] = 0x0F;
				}
			}

			rc = block_write_direct(service_id, addr, 1, buffer);
			if (rc != EOK)
				return EIO;

			++addr;
		}
	}

	/* Root directory */
	printf("Writing root directory.\n");
	memset(buffer, 0, cfg->sector_size);
	if (cfg->fat_type != FAT32) {
		size_t idx;
		for (idx = 0; idx < cfg->rootdir_sectors; ++idx) {
			rc = block_write_direct(service_id, addr, 1, buffer);
			if (rc != EOK)
				return EIO;

			++addr;
		}
	} else {
		for (i = 0; i < cfg->sectors_per_cluster; i++) {
			rc = block_write_direct(service_id, addr, 1, buffer);
			if (rc != EOK)
				return EIO;

			++addr;
		}	
	}

	free(buffer);

	return EOK;
}

/** Construct boot sector with the given parameters. */
static void fat_bootsec_create(struct fat_cfg const *cfg, struct fat_bs *bs)
{
	memset(bs, 0, sizeof(*bs));

	bs->ji[0] = 0xEB;
	bs->ji[1] = 0x3C;
	bs->ji[2] = 0x90;

	memcpy(bs->oem_name, "HELENOS ", 8);

	/* BIOS Parameter Block */
	bs->bps = host2uint16_t_le(cfg->sector_size);
	bs->spc = cfg->sectors_per_cluster;
	bs->rscnt = host2uint16_t_le(cfg->reserved_sectors);
	bs->fatcnt = cfg->fat_count;
	bs->root_ent_max = host2uint16_t_le(cfg->root_ent_max);

	if (cfg->total_sectors < 0x10000) {
		bs->totsec16 = host2uint16_t_le(cfg->total_sectors);
		bs->totsec32 = 0;
	} else {
		bs->totsec16 = 0;
		bs->totsec32 = host2uint32_t_le(cfg->total_sectors);
	}

	bs->mdesc = default_media_descriptor;
	bs->sec_per_track = host2uint16_t_le(63);
	bs->signature = host2uint16_t_be(0x55AA);
	bs->headcnt = host2uint16_t_le(6);
	bs->hidden_sec = host2uint32_t_le(0);

	if (cfg->fat_type == FAT32) {
		bs->sec_per_fat = 0;
		bs->fat32.sectors_per_fat = host2uint32_t_le(cfg->fat_sectors);

		bs->fat32.pdn = 0x80;
		bs->fat32.ebs = 0x29;
		bs->fat32.id = host2uint32_t_be(0x12345678);
		bs->fat32.root_cluster = 2;

		memcpy(bs->fat32.label, "HELENOS_NEW", 11);
		memcpy(bs->fat32.type, "FAT32   ", 8);
	} else {
		bs->sec_per_fat = host2uint16_t_le(cfg->fat_sectors);
		bs->pdn = 0x80;
		bs->ebs = 0x29;
		bs->id = host2uint32_t_be(0x12345678);

		memcpy(bs->label, "HELENOS_NEW", 11);
		memcpy(bs->type, "FAT   ", 8);
	}
}

/**
 * @}
 */
