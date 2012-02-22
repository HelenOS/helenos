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
#include <malloc.h>
#include <byteorder.h>
#include <align.h>
#include <sys/types.h>
#include <sys/typefmt.h>
#include "exfat.h"

#define NAME    "mkexfat"

/** First sector of the FAT */
#define FAT_SECTOR_START 128

/** First sector of the Main Extended Boot Region */
#define EBS_SECTOR_START 1

/** First sector of the Main Extended Boot Region Backup */
#define EBS_BACKUP_SECTOR_START 13

/** First sector of the Main Boot Sector */
#define MBS_SECTOR 0

/** First sector of the Main Boot Sector Backup */
#define MBS_BACKUP_SECTOR 12

/** Size of the Main Extended Boot Region */
#define EBS_SIZE 8

/** Divide and round up. */
#define div_round_up(a, b) (((a) + (b) - 1) / (b))

/** The default size of each cluster is 4096 byte */
#define DEFAULT_CLUSTER_SIZE 4096

typedef struct exfat_cfg {
	aoff64_t volume_start;
	aoff64_t volume_count;
	unsigned long fat_sector_count;
	unsigned long data_start_sector;
	unsigned long rootdir_cluster;
	unsigned long total_clusters;
	unsigned long allocated_clusters;
	size_t   bitmap_size;
	size_t   sector_size;
	size_t   cluster_size;
} exfat_cfg_t;


static unsigned log2(unsigned n);

static uint32_t
vbr_checksum_start(void const *octets, size_t nbytes);

static void
vbr_checksum_update(void const *octets, size_t nbytes, uint32_t *checksum);

static int
ebs_write(service_id_t service_id, exfat_cfg_t *cfg,
    int base, uint32_t *chksum);

static int
bitmap_write(service_id_t service_id, exfat_cfg_t *cfg);

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

	aoff64_t n_req_clusters = volume_bytes / DEFAULT_CLUSTER_SIZE;
	cfg->cluster_size = DEFAULT_CLUSTER_SIZE;

	/* Compute the required cluster size to index
	 * the entire storage device and to keep the FAT
	 * size less or equal to 64 Mb.
	 */
	while (n_req_clusters > 16000000ULL &&
	    (cfg->cluster_size < 32 * 1024 * 1024)) {

		cfg->cluster_size <<= 1;
		n_req_clusters = volume_bytes / cfg->cluster_size;
	}

	/* The first two clusters are reserved */
	cfg->total_clusters = n_req_clusters + 2;

	/* Compute the FAT size in sectors */
	fat_bytes = (cfg->total_clusters + 1) * 4;
	cfg->fat_sector_count = div_round_up(fat_bytes, cfg->sector_size);

	/* Compute the number of the first data sector */
	cfg->data_start_sector = ROUND_UP(FAT_SECTOR_START +
	    cfg->fat_sector_count, cfg->cluster_size / cfg->sector_size);

	/* Compute the bitmap size */
	cfg->bitmap_size = n_req_clusters / 8;

	cfg->allocated_clusters = div_round_up(cfg->bitmap_size,
	    cfg->cluster_size);

	/* This account for the root directory */
	cfg->allocated_clusters++;
	/* FIXME: add upcase table clusters to allocated_clusters */

	/* FIXME: set the real rootdir cluster */
	cfg->rootdir_cluster = 0;

	/* The first sector of the partition is zero */
	cfg->volume_start = 0;
}

/** Prints the exFAT structure values
 *
 * @param cfg Pointer to the exfat_cfg_t structure.
 */
static void
cfg_print_info(exfat_cfg_t *cfg)
{
	printf(NAME ": Sector size:           %lu\n", cfg->sector_size);
	printf(NAME ": Cluster size:          %lu\n", cfg->cluster_size);
	printf(NAME ": FAT size in sectors:   %lu\n", cfg->fat_sector_count);
	printf(NAME ": Data start sector:     %lu\n", cfg->data_start_sector);
	printf(NAME ": Total num of clusters: %lu\n", cfg->total_clusters);
}

/** Initialize the Main Boot Sector fields.
 *
 * @param mbs Pointer to the Main Boot Sector structure.
 * @param cfg Pointer to the exFAT configuration structure.
 * @return    Initial checksum value.
 */
static uint32_t
vbr_initialize(exfat_bs_t *mbs, exfat_cfg_t *cfg)
{
	/* Fill the structure with zeroes */
	memset(mbs, 0, sizeof(exfat_bs_t));

	/* Init Jump Boot section */
	mbs->jump[0] = 0xEB;
	mbs->jump[1] = 0x76;
	mbs->jump[2] = 0x90;

	/* Set the filesystem name */
	memcpy(mbs->oem_name, "EXFAT   ", sizeof(mbs->oem_name));

	mbs->volume_start = host2uint64_t_le(cfg->volume_start);
	mbs->volume_count = host2uint64_t_le(cfg->volume_count);
	mbs->fat_sector_start = host2uint32_t_le(FAT_SECTOR_START);
	mbs->fat_sector_count = host2uint32_t_le(cfg->fat_sector_count);
	mbs->data_start_sector = host2uint32_t_le(cfg->data_start_sector);

	mbs->data_clusters = host2uint32_t_le(cfg->total_clusters - 
	    div_round_up(cfg->data_start_sector, cfg->cluster_size));

	mbs->rootdir_cluster = 0;
	mbs->volume_serial = 0;
	mbs->version.major = 1;
	mbs->version.minor = 0;
	mbs->volume_flags = host2uint16_t_le(0);
	mbs->bytes_per_sector = log2(cfg->sector_size);
	mbs->sec_per_cluster = log2(cfg->cluster_size / cfg->sector_size);

	/* Maximum cluster size is 32 Mb */
	assert((mbs->bytes_per_sector + mbs->sec_per_cluster) <= 25);

	mbs->fat_count = 1;
	mbs->drive_no = 0x80;
	mbs->allocated_percent = 0;
	mbs->signature = host2uint16_t_le(0xAA55);

	return vbr_checksum_start(mbs, sizeof(exfat_bs_t));
}

static int
bootsec_write(service_id_t service_id, exfat_cfg_t *cfg)
{
	exfat_bs_t mbs;
	uint32_t vbr_checksum;
	uint32_t initial_checksum;
	int rc;

	vbr_checksum = vbr_initialize(&mbs, cfg);
	initial_checksum = vbr_checksum;

	/* Write the Main Boot Sector to disk */
	rc = block_write_direct(service_id, MBS_SECTOR, 1, &mbs);
	if (rc != EOK)
		return rc;

	/* Write the Main extended boot sectors to disk */
	rc = ebs_write(service_id, cfg, EBS_SECTOR_START, &vbr_checksum);
	if (rc != EOK)
		return rc;

	/* Write the Main Boot Sector backup to disk */
	rc = block_write_direct(service_id, MBS_BACKUP_SECTOR, 1, &mbs);
	if (rc != EOK)
		return rc;

	/* Restore the checksum to its initial value */
	vbr_checksum = initial_checksum;

	/* Write the Main extended boot sectors backup to disk */ 
	return ebs_write(service_id, cfg,
	    EBS_BACKUP_SECTOR_START, &vbr_checksum);
}

/** Write the Main Extended Boot Sector to disk
 *
 * @param service_id  The service id.
 * @param cfg  Pointer to the exFAT configuration structure.
 * @param base Base sector of the EBS.
 * @return  EOK on success or a negative error code.
 */
static int
ebs_write(service_id_t service_id, exfat_cfg_t *cfg, int base, uint32_t *chksum)
{
	uint32_t *ebs = calloc(cfg->sector_size, sizeof(uint8_t));
	int i, rc;
	unsigned idx;

	if (!ebs)
		return ENOMEM;

	ebs[cfg->sector_size / 4 - 1] = host2uint32_t_le(0xAA550000);

	for (i = 0; i < EBS_SIZE; ++i) {
		vbr_checksum_update(ebs, cfg->sector_size, chksum);

		rc = block_write_direct(service_id,
		    i + EBS_SECTOR_START + base, 1, ebs);

		if (rc != EOK)
			goto exit;
	}

	/* The OEM record is not yet used
	 * by the official exFAT implementation, we'll fill
	 * it with zeroes.
	 */

	memset(ebs, 0, cfg->sector_size);
	vbr_checksum_update(ebs, cfg->sector_size, chksum);

	rc = block_write_direct(service_id, i++ + base, 1, ebs);
	if (rc != EOK)
		goto exit;

	/* The next sector is reserved, fill it with zeroes too */
	vbr_checksum_update(ebs, cfg->sector_size, chksum);
	rc = block_write_direct(service_id, i++ + base, 1, ebs);
	if (rc != EOK)
		goto exit;

	/* Write the checksum sector */
	for (idx = 0; idx < cfg->sector_size / sizeof(uint32_t); ++idx)
		ebs[idx] = host2uint32_t_le(*chksum);

	rc = block_write_direct(service_id, i + base, 1, ebs);

exit:
	free(ebs);
	return rc;
}

/** Writes the FAT on disk.
 *
 * @param service_id  The service id.
 * @param cfg Pointer to the exfat_cfg structure.
 * @return EOK on success or a negative error code.
 */
static int
fat_write(service_id_t service_id, exfat_cfg_t *cfg)
{
	unsigned long i;
	uint32_t *pfat;
	int rc;

	pfat = calloc(cfg->sector_size, 1);
	if (!pfat)
		return ENOMEM;

	pfat[0] = host2uint32_t_le(0xFFFFFFF8);
	pfat[1] = host2uint32_t_le(0xFFFFFFFF);

	/* Allocate clusters for the bitmap, upcase table
	 * and the root directory.
	 */
	pfat[2] = host2uint32_t_le(0xFFFFFFFF);
	pfat[3] = host2uint32_t_le(0xFFFFFFFF);
	pfat[4] = host2uint32_t_le(0xFFFFFFFF);

	rc = block_write_direct(service_id, FAT_SECTOR_START, 1, pfat);
	if (rc != EOK)
		goto error;

	memset(pfat, 0, 5 * sizeof(uint32_t));

	for (i = 1; i < cfg->fat_sector_count; ++i) {
		rc = block_write_direct(service_id,
		    FAT_SECTOR_START + i, 1, pfat);
		if (rc != EOK)
			goto error;
	}

error:
	free(pfat);
	return rc;
}

/** Initialize the allocation bitmap.
 *
 * @param service_id   The service id.
 * @param cfg  Pointer to the exfat configuration structure.
 * @return  EOK on success or a negative error code.
 */
static int
bitmap_write(service_id_t service_id, exfat_cfg_t *cfg)
{
	unsigned long i, sec;
	unsigned long allocated_cls;
	int rc = EOK;

	/* Bitmap size in sectors */
	size_t const bss = div_round_up(cfg->bitmap_size, cfg->sector_size);

	uint8_t *bitmap = malloc(cfg->sector_size);
	if (!bitmap)
		return ENOMEM;

	allocated_cls = cfg->allocated_clusters;

	for (sec = 0; sec < bss; ++sec) {
		memset(bitmap, 0, cfg->sector_size);
		if (allocated_cls > 0) {
			for (i = 0; i < allocated_cls; ++i) {
				unsigned byte_idx = i / 8;
				unsigned bit_idx = i % 8;

				if (byte_idx == cfg->sector_size)
					break;
				bitmap[byte_idx] |= 1 << bit_idx;
			}

			allocated_cls -= i * 8;
		}

		rc = block_write_direct(service_id,
		    cfg->data_start_sector + sec, 1, bitmap);
		if (rc != EOK)
			goto exit;
	}

exit:
	free(bitmap);
	return rc;
}

/** Given a number (n), returns the result of log2(n).
 *
 * It works only if n is a power of two.
 */
static unsigned
log2(unsigned n)
{
	unsigned r;

	for (r = 0;n >> r != 1; ++r);

	return r;
}

/** Initialize the VBR checksum calculation */
static uint32_t
vbr_checksum_start(void const *data, size_t nbytes)
{
	uint32_t checksum = 0;
	size_t index;
	uint8_t const *octets = (uint8_t *) data;

	for (index = 0; index < nbytes; ++index) {
		if (index == 106 || index == 107 || index == 112) {
			/* Skip volume_flags and allocated_percent fields */
			continue;
		}

		checksum = ((checksum << 31) | (checksum >> 1)) + octets[index];
	}

	return checksum;
}

/** Update the VBR checksum */
static void
vbr_checksum_update(void const *data, size_t nbytes, uint32_t *checksum)
{
	size_t index;
	uint8_t const *octets = (uint8_t *) data;

	for (index = 0; index < nbytes; ++index)
		*checksum = ((*checksum << 31) | (*checksum >> 1)) + octets[index];
}

int main (int argc, char **argv)
{
	exfat_cfg_t cfg;
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
		printf(NAME ": Error, sector size can't be greater" \
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
	cfg_print_info(&cfg);

	rc = bootsec_write(service_id, &cfg);
	if (rc != EOK) {
		printf(NAME ": Error, failed to write the VBR to disk\n");
		return 2;
	}

	rc = fat_write(service_id, &cfg);
	if (rc != EOK) {
		printf(NAME ": Error, failed to write the FAT to disk\n");
		return 2;
	}

	rc = bitmap_write(service_id, &cfg);
	if (rc != EOK) {
		printf(NAME ": Error, failed to write the allocation" \
		    " bitmap to disk.\n");
		return 2;
	}

	return 0;
}

