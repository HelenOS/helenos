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
#include <assert.h>
#include <errno.h>
#include <byteorder.h>
#include <sys/types.h>
#include <sys/typefmt.h>
#include "exfat.h"

#define NAME    "mkexfat"

/** First sector of the FAT */
#define FAT_SECTOR_START 128

/** Divide and round up. */
#define div_round_up(a, b) (((a) + (b) - 1) / (b))

/** The default size of each cluster is 4096 byte */
#define DEFAULT_CLUSTER_SIZE 4096

static unsigned log2(unsigned n);

typedef struct exfat_cfg {
	aoff64_t volume_start;
	aoff64_t volume_count;
	uint32_t fat_sector_count;
	uint32_t data_start_sector;
	uint32_t data_cluster;
	uint32_t rootdir_cluster;
	size_t   sector_size;
	size_t   cluster_size;
	uint32_t total_clusters;
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
	unsigned long fat_bytes;
	aoff64_t const volume_bytes = (cfg->volume_count - FAT_SECTOR_START) *
	    cfg->sector_size;

	/** Number of clusters required to index the entire device, it must
	 * be less then UINT32_MAX.
	 */
	aoff64_t n_req_clusters = volume_bytes / DEFAULT_CLUSTER_SIZE;
	cfg->cluster_size = DEFAULT_CLUSTER_SIZE;

	/* Compute the required cluster size to index
	 * the entire storage device.
	 */
	while (n_req_clusters > UINT32_MAX &&
	    (cfg->cluster_size < 32 * 1024 * 1024)) {

		cfg->cluster_size <<= 1;
		n_req_clusters = volume_bytes / cfg->cluster_size;
	}

	cfg->total_clusters = n_req_clusters;

	/* Compute the FAT size in sectors */
	fat_bytes = (cfg->total_clusters + 1) * 4;
	cfg->fat_sector_count = div_round_up(fat_bytes, cfg->sector_size);

	/* The first sector of the partition is zero */
	cfg->volume_start = 0;
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
	vbr->bytes_per_sector = log2(cfg->sector_size);
	vbr->sec_per_cluster = log2(cfg->cluster_size / cfg->sector_size);

	/* Maximum cluster size is 32 Mb */
	assert((vbr->bytes_per_sector + vbr->sec_per_cluster) <= 25);

	vbr->fat_count = 1;
	vbr->drive_no = 0x80;
	vbr->allocated_percent = 0;
	vbr->signature = host2uint16_t_le(0xAA55);
}

/** Given a power-of-two number (n), returns the result of log2(n).
 *
 * It works only if n is a power of two.
 */
static unsigned log2(unsigned n)
{
	unsigned r;

	for (r = 0;n >> r != 1; ++r);

	return r;
}

int main (int argc, char **argv)
{
	exfat_cfg_t cfg;
	exfat_bs_t  vbr;
	char *dev_path;
	service_id_t service_id;
	int rc;

	if (argc < 2) {
		printf(NAME ": Error, argument missing\n");
		usage();
		return 1;
	}

	/* TODO: Add parameters */

	++argv;
	dev_path = *argv;

	printf(NAME ": Device = %s\n", dev_path);

	rc = loc_service_get_id(dev_path, &service_id, 0);
	if (rc != EOK) {
		printf(NAME ": Error resolving device `%s'.\n", dev_path);
		return 2;
	}

	rc = block_init(EXCHANGE_SERIALIZE, service_id, 2048);
	if (rc != EOK) {
		printf(NAME ": Error initializing libblock.\n");
		return 2;
	}

	rc = block_get_bsize(service_id, &cfg.sector_size);
	if (rc != EOK) {
		printf(NAME ": Error determining device block size.\n");
		return 2;
	}

	if (cfg.sector_size > 4096) {
		printf(NAME ":Error, sector size can't be greater" \
		    " than 4096 bytes.\n");
		return 2;
	}

	rc = block_get_nblocks(service_id, &cfg.volume_count);
	if (rc != EOK) {
		printf(NAME ": Warning, failed to obtain" \
		    " device block size.\n");
		/* FIXME: the user should be able to specify the filesystem size */
		return 1;
	} else {
		printf(NAME ": Block device has %" PRIuOFF64 " blocks.\n",
		    cfg.volume_count);
	}

	cfg_params_initialize(&cfg);
	vbr_initialize(&vbr, &cfg);


	return 0;
}

