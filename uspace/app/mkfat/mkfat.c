/*
 * Copyright (c) 2024 Jiri Svoboda
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

/** @addtogroup mkfat
 * @{
 */

/**
 * @file	mkfat.c
 * @brief	Tool for creating new FAT file systems.
 *
 * Currently we can create 12/16/32-bit FAT.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <block.h>
#include <mem.h>
#include <loc.h>
#include <byteorder.h>
#include <inttypes.h>
#include <errno.h>
#include <rndgen.h>
#include <str.h>
#include "fat.h"
#include "fat_dentry.h"

#define NAME	"mkfat"

#define LABEL_NONAME "NO NAME"

/** Divide and round up. */
#define div_round_up(a, b) (((a) + (b) - 1) / (b))

/** Default file-system parameters */
enum {
	default_sector_size		= 512,
	default_sectors_per_cluster	= 4,
	default_fat_count		= 2,
	default_reserved_clusters	= 2,
	default_media_descriptor	= 0xF8 /**< fixed disk */,
	fat32_root_cluster		= 2
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
	const char *label;
} fat_cfg_t;

static void syntax_print(void);

static errno_t fat_params_compute(struct fat_cfg *cfg);
static errno_t fat_blocks_write(struct fat_cfg const *cfg, service_id_t service_id);
static errno_t fat_bootsec_create(struct fat_cfg const *cfg, struct fat_bs *bs);

int main(int argc, char **argv)
{
	struct fat_cfg cfg;

	errno_t rc;
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
	cfg.fat_type = FATAUTO;
	cfg.label = NULL;

	if (argc < 2) {
		printf(NAME ": Error, argument missing.\n");
		syntax_print();
		return 1;
	}

	--argc;
	++argv;

	while (*argv[0] == '-') {
		if (str_cmp(*argv, "--size") == 0) {
			--argc;
			++argv;
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

			--argc;
			++argv;
		}

		if (str_cmp(*argv, "--type") == 0) {
			--argc;
			++argv;
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

			--argc;
			++argv;
		}

		if (str_cmp(*argv, "--label") == 0) {
			--argc;
			++argv;
			if (*argv == NULL) {
				printf(NAME ": Error, argument missing.\n");
				syntax_print();
				return 1;
			}

			cfg.label = *argv;

			--argc;
			++argv;
		}

		if (str_cmp(*argv, "-") == 0) {
			--argc;
			++argv;
			break;
		}
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

	rc = block_init(service_id);
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

	if (cfg.fat_type != FATAUTO && cfg.fat_type != FAT12 && cfg.fat_type != FAT16 &&
	    cfg.fat_type != FAT32) {
		printf(NAME ": Error. Unknown FAT type.\n");
		return 2;
	}

	printf(NAME ": Creating FAT filesystem on device %s.\n", dev_path);

	rc = fat_params_compute(&cfg);
	if (rc != EOK) {
		printf(NAME ": Invalid file-system parameters.\n");
		return 2;
	}

	printf(NAME ": Filesystem type FAT%d.\n", cfg.fat_type);

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
	printf("syntax: mkfat [<options>...] <device_name>\n");
	printf("options:\n"
	    "\t--size <sectors> Filesystem size, overrides device size\n"
	    "\t--type 12|16|32  FAT type (auto-detected by default)\n"
	    "\t--label <label>  Volume label\n");
}

static errno_t fat_label_encode(void *dest, const char *src)
{
	int i;
	const char *sp;
	uint8_t *dp;

	i = 0;
	sp = src;
	dp = (uint8_t *)dest;

	while (*sp != '\0' && i < FAT_VOLLABEL_LEN) {
		if (!ascii_check(*sp))
			return EINVAL;
		if (dp != NULL)
			dp[i] = toupper(*sp);
		++i;
		++sp;
	}

	while (i < FAT_VOLLABEL_LEN) {
		if (dp != NULL)
			dp[i] = FAT_PAD;
		++i;
	}

	return EOK;
}

/** Derive sizes of different filesystem structures.
 *
 * This function concentrates all the different computations of FAT
 * file system params.
 */
static errno_t fat_params_compute(struct fat_cfg *cfg)
{
	uint32_t fat_bytes;
	uint32_t non_data_sectors_lb_16;
	uint32_t non_data_sectors_lb;
	uint32_t rd_sectors;
	uint32_t tot_clust_16;

	/*
	 * Make a conservative guess on the FAT size needed for the file
	 * system. The optimum could be potentially smaller since we
	 * do not subtract size of the FAT itself when computing the
	 * size of the data region. Also the root dir area might not
	 * need FAT entries if we decide to make a FAT32.
	 */

	cfg->reserved_sectors = 1 + cfg->addt_res_sectors;

	/* Only correct for FAT12/16 (FAT32 has root dir stored in clusters) */
	rd_sectors = div_round_up(cfg->root_ent_max * DIRENT_SIZE,
	    cfg->sector_size);
	non_data_sectors_lb_16 = cfg->reserved_sectors + rd_sectors;

	/* Only correct for FAT12/16 */
	tot_clust_16 = div_round_up(cfg->total_sectors - non_data_sectors_lb_16,
	    cfg->sectors_per_cluster);

	/* Now detect FAT type */
	if (tot_clust_16 <= FAT12_CLST_MAX) {
		if (cfg->fat_type == FATAUTO)
			cfg->fat_type = FAT12;
		else if (cfg->fat_type != FAT12)
			return EINVAL;
	} else if (tot_clust_16 <= FAT16_CLST_MAX) {
		if (cfg->fat_type == FATAUTO)
			cfg->fat_type = FAT16;
		else if (cfg->fat_type != FAT16)
			return EINVAL;
	} else {
		if (cfg->fat_type == FATAUTO)
			cfg->fat_type = FAT32;
		else if (cfg->fat_type != FAT32)
			return EINVAL;
	}

	/* Actual root directory size, non-data sectors */
	if (cfg->fat_type != FAT32) {
		cfg->rootdir_sectors = div_round_up(cfg->root_ent_max * DIRENT_SIZE,
		    cfg->sector_size);
		non_data_sectors_lb = cfg->reserved_sectors + cfg->rootdir_sectors;

	} else {
		/* We create a single-cluster root dir */
		cfg->rootdir_sectors = cfg->sectors_per_cluster;
		non_data_sectors_lb = cfg->reserved_sectors;
	}

	/* Actual total number of clusters */
	cfg->total_clusters = div_round_up(cfg->total_sectors - non_data_sectors_lb,
	    cfg->sectors_per_cluster);

	fat_bytes = div_round_up((cfg->total_clusters + 2) *
	    FAT_CLUSTER_DOUBLE_SIZE(cfg->fat_type), 2);
	cfg->fat_sectors = div_round_up(fat_bytes, cfg->sector_size);

	if (cfg->label != NULL && fat_label_encode(NULL, cfg->label) != EOK)
		return EINVAL;

	return EOK;
}

/** Create file system with the given parameters. */
static errno_t fat_blocks_write(struct fat_cfg const *cfg, service_id_t service_id)
{
	aoff64_t addr;
	uint8_t *buffer;
	int i;
	uint32_t j;
	errno_t rc;
	struct fat_bs bs;
	fat_dentry_t *de;

	rc = fat_bootsec_create(cfg, &bs);
	if (rc != EOK)
		return rc;

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

	if (cfg->fat_type == FAT32) {
		/* Root dir is stored in cluster fat32_root_cluster */
		addr += fat32_root_cluster * cfg->sectors_per_cluster;
	}

	/* Root directory */
	printf("Writing root directory.\n");
	memset(buffer, 0, cfg->sector_size);
	de = (fat_dentry_t *)buffer;
	size_t idx;
	for (idx = 0; idx < cfg->rootdir_sectors; ++idx) {

		if (idx == 0 && cfg->label != NULL) {
			/* Set up volume label entry */
			(void) fat_label_encode(&de->name, cfg->label);
			de->attr = FAT_ATTR_VOLLABEL;
			de->mtime = 0x1234; // XXX Proper time
			de->mdate = 0x1234; // XXX Proper date
		} else if (idx == 1) {
			/* Clear volume label entry */
			memset(buffer, 0, cfg->sector_size);
		}

		rc = block_write_direct(service_id, addr, 1, buffer);
		if (rc != EOK)
			return EIO;

		++addr;
	}

	free(buffer);

	return EOK;
}

/** Construct boot sector with the given parameters. */
static errno_t fat_bootsec_create(struct fat_cfg const *cfg, struct fat_bs *bs)
{
	const char *bs_label;
	rndgen_t *rndgen;
	uint32_t vsn;
	errno_t rc;

	/* Generate a volume serial number */
	rc = rndgen_create(&rndgen);
	if (rc != EOK)
		return rc;

	rc = rndgen_uint32(rndgen, &vsn);
	if (rc != EOK) {
		rndgen_destroy(rndgen);
		return rc;
	}

	rndgen_destroy(rndgen);

	/*
	 * The boot sector must always contain a valid label. If there
	 * is no label, there should be 'NO NAME'
	 */
	if (cfg->label != NULL)
		bs_label = cfg->label;
	else
		bs_label = LABEL_NONAME;
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
		bs->fat32.id = host2uint32_t_be(vsn);
		bs->fat32.root_cluster = fat32_root_cluster;

		(void) fat_label_encode(&bs->fat32.label, bs_label);
		memcpy(bs->fat32.type, "FAT32   ", 8);
	} else {
		bs->sec_per_fat = host2uint16_t_le(cfg->fat_sectors);
		bs->pdn = 0x80;
		bs->ebs = 0x29;
		bs->id = host2uint32_t_be(vsn);

		(void) fat_label_encode(&bs->label, bs_label);
		if (cfg->fat_type == FAT12)
			memcpy(bs->type, "FAT12   ", 8);
		else
			memcpy(bs->type, "FAT16   ", 8);
	}

	return EOK;
}

/**
 * @}
 */
