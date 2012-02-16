/*
 * Copyright (c) 2012 Maurizio Lombardi
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
 * @file	mkexfat.c
 * @brief	Tool for creating new exFAT file systems.
 *
 */

#include <stdio.h>
#include <libblock.h>

#define NAME    "mkexfat"

/** First sector of the FAT */
#define FAT_SECTOR_START 128

/** Divide and round up. */
#define div_round_up(a, b) (((a) + (b) - 1) / (b))

typedef struct exfat_cfg {
	uint64_t volume_start;
	uint64_t volume_count;
	uint32_t fat_sector_count;
	uint32_t data_start_sector;
	uint32_t data_cluster;
	uint32_t rootdir_cluster;
	unsigned bytes_per_sector;
	unsigned sec_per_cluster;
} exfat_cfg_t;

static void usage(void)
{
	printf("Usage: mkexfat <device>\n");
}

/** Initialize the exFAT params structure.
 *
 * @param cfg Pointer to the exFAT params structure to initialize.
 */
static void
cfg_params_initialize(exfat_cfg_t *cfg)
{
}

/** Initialize the Volume Boot Record fields.
 *
 * @param vbr Pointer to the Volume Boot Record structure.
 * @param cfg Pointer to the exFAT configuration structure.
 */
static void
vbr_initialize(exfat_bs_t *vbr, exfat_cfg_t *cfg)
{
	/* Fill the structure with zeroes */
	memset(vbr, 0, sizeof(exfat_bs_t));

	/* Init Jump Boot section */
	vbr->jump[0] = 0xEB;
	vbr->jump[1] = 0x76;
	vbr->jump[2] = 0x90;

	/* Set the filesystem name */
	memcpy(vbr->oem_name, "EXFAT   ", sizeof(vbr->oem_name));

	vbr->volume_start = host2uint64_t_le(cfg->volume_start);
	vbr->volume_count = host2uint64_t_le(cfg->volume_count);
	vbr->fat_sector_start = host2uint32_t_le(FAT_SECTOR_START);
	vbr->fat_sector_count = host2uint32_t_le(cfg->fat_sector_count);
	vbr->version.major = 1;
	vbr->version.minor = 0;
	vbr->volume_flags = host2uint16_t_le(0);
	vbr->bytes_per_sector = cfg->bytes_per_sectors;
	vbr->sec_per_cluster = cfg->sec_per_cluster;
	vbr->fat_count = 1;
	vbr->drive_no = 0x80;
	vbr->allocated_percent = 0;
	vbr->signature = host2uint16_t_le(0xAA55);
}

int main (int argc, char **argv)
{
	exfat_cfg_t cfg;
	aoff64_t dev_nblocks;
	char *dev_path;
	service_id_t service_id;
	size_t sector_size;
	int rc;

	if (argc < 2) {
		printf(NAME ": Error, argument missing\n");
		usage();
		return 1;
	}

	/* The fist sector of the partition is zero */
	cfg.volume_start = 0;

	/* TODO: Add parameters */

	++argv;
	dev_path = *argv;

	printf(NAME ": Device = %s\n", dev_path);

	rc = loc_service_get_id(dev_path, &service_id, 0);
	if (rc != EOK) {
		printf(NAME ": Error resolving device `%s'.\n");
		return 2;
	}

	rc = block_init(EXCHANGE_SERIALIZE, service_id, 2048);
	if (rc != EOK) {
		printf(NAME ": Error initializing libblock.\n");
		return 2;
	}

	rc = block_get_bsize(service_id, &sector_size);
	if (rc != EOK) {
		printf(NAME ": Error determining device block size.\n");
		return 2;
	}

	rc = block_get_nblocks(service_id, &dev_nblocks);
	if (rc != EOK) {
		printf(NAME ": Warning, failed to obtain device block size.\n");
		/* FIXME: the user should specify the filesystem size */
		return 1;
	} else {
		printf(NAME ": Block device has %" PRIuOFF64 " blocks.\n",
		    dev_nblocks);

		cfg.volume_count = dev_nblocks;
	}

	cfg_params_initialize(&cfg);


	return 0;
}

