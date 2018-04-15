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
#include <stdbool.h>
#include <stdint.h>
#include <block.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <byteorder.h>
#include <align.h>
#include <str.h>
#include <getopt.h>
#include <macros.h>
#include "exfat.h"
#include "upcase.h"

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

/** VBR Checksum sector */
#define VBR_CHECKSUM_SECTOR 11

/** VBR Backup Checksum sector */
#define VBR_BACKUP_CHECKSUM_SECTOR 23

/** Size of the Main Extended Boot Region */
#define EBS_SIZE 8

/** Divide and round up. */
#define div_round_up(a, b) (((a) + (b) - 1) / (b))

/** The default size of each cluster is 4096 byte */
#define DEFAULT_CLUSTER_SIZE 4096

/** Index of the first free cluster on the device */
#define FIRST_FREE_CLUSTER   2


typedef struct exfat_cfg {
	aoff64_t volume_start;
	aoff64_t volume_count;
	unsigned long fat_sector_count;
	unsigned long data_start_sector;
	unsigned long rootdir_cluster;
	unsigned long upcase_table_cluster;
	unsigned long bitmap_cluster;
	unsigned long total_clusters;
	unsigned long allocated_clusters;
	size_t   bitmap_size;
	size_t   sector_size;
	size_t   cluster_size;
	const char *label;
} exfat_cfg_t;

static unsigned log2i(unsigned n);

static uint32_t
vbr_checksum_start(void const *octets, size_t nbytes);

static void
vbr_checksum_update(void const *octets, size_t nbytes, uint32_t *checksum);

static errno_t
ebs_write(service_id_t service_id, exfat_cfg_t *cfg,
    int base, uint32_t *chksum);

static errno_t
bitmap_write(service_id_t service_id, exfat_cfg_t *cfg);

static uint32_t
upcase_table_checksum(void const *data, size_t nbytes);

static struct option const long_options[] = {
	{ "help", no_argument, 0, 'h' },
	{ "cluster-size", required_argument, 0, 'c' },
	{ "fs-size", required_argument, 0, 's' },
	{ "label", required_argument, 0, 'L' },
};

static void usage(void)
{
	printf("Usage: mkexfat [options] <device>\n"
	    "-c, --cluster-size ## Specify the cluster size (Kb)\n"
	    "-s, --fs-size ##      Specify the filesystem size (sectors)\n"
	    "    --label ##        Volume label\n");
}

/** Initialize the exFAT params structure.
 *
 * @param cfg Pointer to the exFAT params structure to initialize.
 */
static void
cfg_params_initialize(exfat_cfg_t *cfg)
{
	unsigned long fat_bytes;
	unsigned long fat_cls;
	aoff64_t const volume_bytes = (cfg->volume_count - FAT_SECTOR_START) *
	    cfg->sector_size;

	if (cfg->cluster_size != 0) {
		/* The user already choose the cluster size he wants */
		cfg->total_clusters = volume_bytes / cfg->cluster_size;
		goto skip_cluster_size_set;
	}

	cfg->total_clusters = volume_bytes / DEFAULT_CLUSTER_SIZE;
	cfg->cluster_size = DEFAULT_CLUSTER_SIZE;

	/* Compute the required cluster size to index
	 * the entire storage device and to keep the FAT
	 * size less or equal to 64 Mb.
	 */
	while (cfg->total_clusters > 16000000ULL &&
	    (cfg->cluster_size < 32 * 1024 * 1024)) {

		cfg->cluster_size <<= 1;
		cfg->total_clusters = volume_bytes / cfg->cluster_size;
	}

skip_cluster_size_set:

	/* Compute the FAT size in sectors */
	fat_bytes = (cfg->total_clusters + 3) * sizeof(uint32_t);
	cfg->fat_sector_count = div_round_up(fat_bytes, cfg->sector_size);

	/* Compute the number of the first data sector */
	cfg->data_start_sector = ROUND_UP(FAT_SECTOR_START +
	    cfg->fat_sector_count, cfg->cluster_size / cfg->sector_size);

	/* Subtract the FAT space from the total
	 * number of available clusters.
	 */
	fat_cls = div_round_up((cfg->data_start_sector -
	    FAT_SECTOR_START) * cfg->sector_size,
	    cfg->cluster_size);
	if (fat_cls >= cfg->total_clusters) {
		/* Insufficient disk space on device */
		cfg->total_clusters = 0;
		return;
	}
	cfg->total_clusters -= fat_cls;

	/* Compute the bitmap size */
	cfg->bitmap_size = div_round_up(cfg->total_clusters, 8);

	/* Compute the number of clusters reserved to the bitmap */
	cfg->allocated_clusters = div_round_up(cfg->bitmap_size,
	    cfg->cluster_size);

	/* This account for the root directory */
	cfg->allocated_clusters++;

	/* Compute the number of clusters reserved to the upcase table */
	cfg->allocated_clusters += div_round_up(sizeof(upcase_table),
	    cfg->cluster_size);

	/* Will be set later */
	cfg->rootdir_cluster = 0;

	/* Bitmap always starts at the first free cluster */
	cfg->bitmap_cluster = FIRST_FREE_CLUSTER;

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
	printf("Sector size:           %lu\n",
	    (unsigned long) cfg->sector_size);
	printf("Cluster size:          %lu\n",
	    (unsigned long) cfg->cluster_size);
	printf("FAT size in sectors:   %lu\n", cfg->fat_sector_count);
	printf("Data start sector:     %lu\n", cfg->data_start_sector);
	printf("Total num of clusters: %lu\n", cfg->total_clusters);
	printf("Total used clusters:   %lu\n", cfg->allocated_clusters);
	printf("Bitmap size:           %lu\n", (unsigned long)
	    div_round_up(cfg->bitmap_size, cfg->cluster_size));
	printf("Upcase table size:     %lu\n", (unsigned long)
	    div_round_up(sizeof(upcase_table), cfg->cluster_size));
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

	mbs->data_clusters = host2uint32_t_le(cfg->total_clusters);

	mbs->rootdir_cluster = host2uint32_t_le(cfg->rootdir_cluster);
	mbs->volume_serial = host2uint32_t_le(0xe1028172);
	mbs->version.major = 1;
	mbs->version.minor = 0;
	mbs->volume_flags = host2uint16_t_le(0);
	mbs->bytes_per_sector = log2i(cfg->sector_size);
	mbs->sec_per_cluster = log2i(cfg->cluster_size / cfg->sector_size);

	/* Maximum cluster size is 32 Mb */
	assert((mbs->bytes_per_sector + mbs->sec_per_cluster) <= 25);

	mbs->fat_count = 1;
	mbs->drive_no = 0x80;
	mbs->allocated_percent = 0;
	mbs->signature = host2uint16_t_le(0xAA55);

	return vbr_checksum_start(mbs, sizeof(exfat_bs_t));
}

static errno_t
bootsec_write(service_id_t service_id, exfat_cfg_t *cfg)
{
	exfat_bs_t mbs;
	uint32_t vbr_checksum;
	uint32_t *chksum_sector;
	errno_t rc;
	unsigned idx;

	chksum_sector = calloc(cfg->sector_size, sizeof(uint8_t));
	if (!chksum_sector)
		return ENOMEM;

	vbr_checksum = vbr_initialize(&mbs, cfg);

	/* Write the Main Boot Sector to disk */
	rc = block_write_direct(service_id, MBS_SECTOR, 1, &mbs);
	if (rc != EOK)
		goto exit;

	/* Write the Main extended boot sectors to disk */
	rc = ebs_write(service_id, cfg, EBS_SECTOR_START, &vbr_checksum);
	if (rc != EOK)
		goto exit;

	/* Write the Main Boot Sector backup to disk */
	rc = block_write_direct(service_id, MBS_BACKUP_SECTOR, 1, &mbs);
	if (rc != EOK)
		goto exit;

	/* Initialize the checksum sectors */
	for (idx = 0; idx < cfg->sector_size / sizeof(uint32_t); ++idx)
		chksum_sector[idx] = host2uint32_t_le(vbr_checksum);

	/* Write the main checksum sector to disk */
	rc = block_write_direct(service_id,
	    VBR_CHECKSUM_SECTOR, 1, chksum_sector);
	if (rc != EOK)
		goto exit;

	/* Write the backup checksum sector to disk */
	rc = block_write_direct(service_id,
	    VBR_BACKUP_CHECKSUM_SECTOR, 1, chksum_sector);
	if (rc != EOK)
		goto exit;

	/* Write the Main extended boot sectors backup to disk */
	rc = ebs_write(service_id, cfg,
	    EBS_BACKUP_SECTOR_START, &vbr_checksum);

exit:
	free(chksum_sector);
	return rc;
}

/** Write the Main Extended Boot Sector to disk
 *
 * @param service_id  The service id.
 * @param cfg  Pointer to the exFAT configuration structure.
 * @param base Base sector of the EBS.
 * @return  EOK on success or an error code.
 */
static errno_t
ebs_write(service_id_t service_id, exfat_cfg_t *cfg, int base,
    uint32_t *chksum)
{
	uint32_t *ebs = calloc(cfg->sector_size, sizeof(uint8_t));
	int i;
	errno_t rc;

	if (!ebs)
		return ENOMEM;

	ebs[cfg->sector_size / 4 - 1] = host2uint32_t_le(0xAA550000);

	for (i = 0; i < EBS_SIZE; ++i) {
		vbr_checksum_update(ebs, cfg->sector_size, chksum);

		rc = block_write_direct(service_id,
		    i + base, 1, ebs);

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

	rc = block_write_direct(service_id, i + base, 1, ebs);
	if (rc != EOK)
		goto exit;

exit:
	free(ebs);
	return rc;
}

/** Initialize the FAT table.
 *
 * @param service_id  The service id.
 * @param cfg Pointer to the exfat_cfg structure.
 * @return EOK on success or an error code.
 */
static errno_t
fat_initialize(service_id_t service_id, exfat_cfg_t *cfg)
{
	unsigned long i;
	uint32_t *pfat;
	errno_t rc;

	pfat = calloc(cfg->sector_size, 1);
	if (!pfat)
		return ENOMEM;

	pfat[0] = host2uint32_t_le(0xFFFFFFF8);
	pfat[1] = host2uint32_t_le(0xFFFFFFFF);

	rc = block_write_direct(service_id, FAT_SECTOR_START, 1, pfat);
	if (rc != EOK)
		goto error;

	pfat[0] = pfat[1] = 0;

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

/** Allocate a given number of clusters and create a cluster chain.
 *
 * @param service_id  The service id.
 * @param cfg  Pointer to the exfat configuration structure.
 * @param cur_cls  Cluster index from where to start the allocation.
 * @param ncls  Number of clusters to allocate.
 * @return EOK on success or an error code.
 */
static errno_t
fat_allocate_clusters(service_id_t service_id, exfat_cfg_t *cfg,
    uint32_t cur_cls, unsigned long ncls)
{
	errno_t rc;
	unsigned const fat_entries = cfg->sector_size / sizeof(uint32_t);
	aoff64_t fat_sec = cur_cls / fat_entries + FAT_SECTOR_START;
	uint32_t *fat;
	uint32_t next_cls = cur_cls;

	cur_cls %= fat_entries;

	fat = malloc(cfg->sector_size);
	if (!fat)
		return ENOMEM;

loop:
	rc = block_read_direct(service_id, fat_sec, 1, fat);
	if (rc != EOK)
		goto exit;

	assert(fat[cur_cls] == 0);
	assert(ncls > 0);

	for (; cur_cls < fat_entries && ncls > 1; ++cur_cls, --ncls)
		fat[cur_cls] = host2uint32_t_le(++next_cls);

	if (cur_cls == fat_entries) {
		/* This sector is full, there are no more free entries,
		 * commit the changes to disk and restart from the next
		 * sector.
		 */
		rc = block_write_direct(service_id, fat_sec++, 1, fat);
		if (rc != EOK)
			goto exit;
		cur_cls = 0;
		goto loop;
	} else if (ncls == 1) {
		/* This is the last cluster of this chain, mark it
		 * with EOF.
		 */
		fat[cur_cls] = host2uint32_t_le(0xFFFFFFFF);
	}

	rc = block_write_direct(service_id, fat_sec, 1, fat);

exit:
	free(fat);
	return rc;
}

/** Initialize the allocation bitmap.
 *
 * @param service_id   The service id.
 * @param cfg  Pointer to the exfat configuration structure.
 * @return  EOK on success or an error code.
 */
static errno_t
bitmap_write(service_id_t service_id, exfat_cfg_t *cfg)
{
	unsigned long i, sec;
	unsigned long allocated_cls;
	errno_t rc = EOK;
	bool need_reset = true;

	/* Bitmap size in sectors */
	size_t const bss = div_round_up(cfg->bitmap_size, cfg->sector_size);

	uint8_t *bitmap = malloc(cfg->sector_size);
	if (!bitmap)
		return ENOMEM;

	allocated_cls = cfg->allocated_clusters;

	for (sec = 0; sec < bss; ++sec) {
		if (need_reset) {
			need_reset = false;
			memset(bitmap, 0, cfg->sector_size);
		}
		if (allocated_cls > 0) {
			for (i = 0; i < allocated_cls; ++i) {
				unsigned byte_idx = i / 8;
				unsigned bit_idx = i % 8;

				if (byte_idx == cfg->sector_size)
					break;
				bitmap[byte_idx] |= 1 << bit_idx;
			}

			allocated_cls -= i;
			need_reset = true;
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

/** Write the upcase table to disk. */
static errno_t
upcase_table_write(service_id_t service_id, exfat_cfg_t *cfg)
{
	errno_t rc = EOK;
	aoff64_t start_sec, nsecs, i;
	uint8_t *table_ptr;
	uint8_t *buf;
	size_t table_size = sizeof(upcase_table);

	buf = malloc(cfg->sector_size);
	if (!buf)
		return ENOENT;

	/* Compute the start sector of the upcase table */
	start_sec = cfg->data_start_sector;
	start_sec += ((cfg->upcase_table_cluster - 2) * cfg->cluster_size) /
	    cfg->sector_size;

	/* Compute the number of sectors needed to store the table on disk */
	nsecs = div_round_up(sizeof(upcase_table), cfg->sector_size);
	table_ptr = (uint8_t *) upcase_table;

	for (i = 0; i < nsecs; ++i,
	    table_ptr += min(table_size, cfg->sector_size),
	    table_size -= cfg->sector_size) {

		if (table_size < cfg->sector_size) {
			/* Reset the content of the unused part
			 * of the last sector.
			 */
			memset(buf, 0, cfg->sector_size);
		}
		memcpy(buf, table_ptr, min(table_size, cfg->sector_size));

		rc = block_write_direct(service_id,
		    start_sec + i, 1, buf);
		if (rc != EOK)
			goto exit;
	}

exit:
	free(buf);
	return rc;
}

/** Initialize and write the root directory entries to disk.
 *
 * @param service_id   The service id.
 * @param cfg   Pointer to the exFAT configuration structure.
 * @return   EOK on success or an error code.
 */
static errno_t
root_dentries_write(service_id_t service_id, exfat_cfg_t *cfg)
{
	exfat_dentry_t *d;
	aoff64_t rootdir_sec;
	uint16_t wlabel[EXFAT_VOLLABEL_LEN + 1];
	errno_t rc;
	uint8_t *data;
	unsigned long i;

	data = calloc(cfg->sector_size, 1);
	if (!data)
		return ENOMEM;

	d = (exfat_dentry_t *) data;

	/* Initialize the volume label dentry */

	if (cfg->label != NULL) {
		memset(wlabel, 0, (EXFAT_VOLLABEL_LEN + 1) * sizeof(uint16_t));
		rc = str_to_utf16(wlabel, EXFAT_VOLLABEL_LEN + 1, cfg->label);
		if (rc != EOK) {
			rc = EINVAL;
			goto exit;
		}

		d->type = EXFAT_TYPE_VOLLABEL;
		memcpy(d->vollabel.label, wlabel, EXFAT_VOLLABEL_LEN * 2);
		d->vollabel.size = utf16_wsize(wlabel);
		assert(d->vollabel.size <= EXFAT_VOLLABEL_LEN);

		d++;
	} else {
		d->type = EXFAT_TYPE_VOLLABEL & ~EXFAT_TYPE_USED;
	}

	/* Initialize the allocation bitmap dentry */
	d->type = EXFAT_TYPE_BITMAP;
	d->bitmap.flags = 0; /* First FAT */
	d->bitmap.firstc = host2uint32_t_le(cfg->bitmap_cluster);
	d->bitmap.size = host2uint64_t_le(cfg->bitmap_size);

	d++;

	/* Initialize the upcase table dentry */
	d->type = EXFAT_TYPE_UCTABLE;
	d->uctable.checksum = host2uint32_t_le(upcase_table_checksum(
	    upcase_table, sizeof(upcase_table)));
	d->uctable.firstc = host2uint32_t_le(cfg->upcase_table_cluster);
	d->uctable.size = host2uint64_t_le(sizeof(upcase_table));

	/* Compute the number of the sector where the rootdir resides */

	rootdir_sec = cfg->data_start_sector;
	rootdir_sec += ((cfg->rootdir_cluster - 2) * cfg->cluster_size) /
	    cfg->sector_size;

	rc = block_write_direct(service_id, rootdir_sec, 1, data);
	if (rc != EOK)
		goto exit;

	/* Fill the content of the sectors not used by the
	 * root directory with zeroes.
	 */
	memset(data, 0, cfg->sector_size);
	for (i = 1; i < cfg->cluster_size / cfg->sector_size; ++i) {
		rc = block_write_direct(service_id, rootdir_sec + i, 1, data);
		if (rc != EOK)
			goto exit;
	}

exit:
	free(data);
	return rc;
}

/** Given a number (n), returns the result of log2(n).
 *
 * It works only if n is a power of two.
 */
static unsigned
log2i(unsigned n)
{
	unsigned r;

	r = 0;
	while (n >> r != 1)
		++r;

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

		checksum = ((checksum << 31) | (checksum >> 1)) +
		    octets[index];
	}

	return checksum;
}

/** Update the VBR checksum */
static void
vbr_checksum_update(void const *data, size_t nbytes, uint32_t *checksum)
{
	size_t index;
	uint8_t const *octets = (uint8_t *) data;

	for (index = 0; index < nbytes; ++index) {
		*checksum = ((*checksum << 31) | (*checksum >> 1)) +
		    octets[index];
	}
}

/** Compute the checksum of the upcase table.
 *
 * @param data   Pointer to the upcase table.
 * @param nbytes size of the upcase table in bytes.
 * @return   Checksum value.
 */
static uint32_t
upcase_table_checksum(void const *data, size_t nbytes)
{
	size_t index;
	uint32_t chksum = 0;
	uint8_t const *octets = (uint8_t *) data;

	for (index = 0; index < nbytes; ++index)
		chksum = ((chksum << 31) | (chksum >> 1)) + octets[index];

	return chksum;
}

/** Check if a given number is a power of two.
 *
 * @param n   The number to check.
 * @return    true if n is a power of two, false otherwise.
 */
static bool
is_power_of_two(unsigned long n)
{
	if (n == 0)
		return false;

	return (n & (n - 1)) == 0;
}

int main (int argc, char **argv)
{
	exfat_cfg_t cfg;
	uint32_t next_cls;
	char *dev_path;
	service_id_t service_id;
	errno_t rc;
	int c, opt_ind;
	aoff64_t user_fs_size = 0;

	if (argc < 2) {
		printf(NAME ": Error, argument missing\n");
		usage();
		return 1;
	}

	cfg.cluster_size = 0;
	cfg.label = NULL;

	c = 0;
	optind = 0;
	opt_ind = 0;
	while (c != -1) {
		c = getopt_long(argc, argv, "hs:c:L:",
		    long_options, &opt_ind);
		switch (c) {
		case 'h':
			usage();
			return 0;
		case 's':
			user_fs_size = (aoff64_t) strtol(optarg, NULL, 10);
			break;

		case 'c':
			cfg.cluster_size = strtol(optarg, NULL, 10) * 1024;
			if (cfg.cluster_size < 4096) {
				printf(NAME ": Error, cluster size can't"
				    " be less than 4096 byte.\n");
				return 1;
			} else if (cfg.cluster_size > 32 * 1024 * 1024) {
				printf(NAME ": Error, cluster size can't"
				    " be greater than 32 Mb");
				return 1;
			}

			if (!is_power_of_two(cfg.cluster_size)) {
				printf(NAME ": Error, the size of the cluster"
				    " must be a power of two.\n");
				return 1;
			}
			break;
		case 'L':
			cfg.label = optarg;
			break;
		}
	}

	argv += optind;
	dev_path = *argv;

	if (!dev_path) {
		printf(NAME ": Error, you must specify a valid block"
		    " device.\n");
		usage();
		return 1;
	}

	printf("Device = %s\n", dev_path);

	rc = loc_service_get_id(dev_path, &service_id, 0);
	if (rc != EOK) {
		printf(NAME ": Error resolving device `%s'.\n", dev_path);
		return 2;
	}

	rc = block_init(service_id, 2048);
	if (rc != EOK) {
		printf(NAME ": Error initializing libblock.\n");
		return 2;
	}

	rc = block_get_bsize(service_id, &cfg.sector_size);
	if (rc != EOK) {
		printf(NAME ": Error determining device sector size.\n");
		return 2;
	}

	user_fs_size *= cfg.sector_size;
	if (user_fs_size > 0 && user_fs_size < 1024 * 1024) {
		printf(NAME ": Error, fs size can't be less"
		    " than 1 Mb.\n");
		return 1;
	}


	if (cfg.sector_size > 4096) {
		printf(NAME ": Error, sector size can't be greater"
		    " than 4096 bytes.\n");
		return 2;
	}

	rc = block_get_nblocks(service_id, &cfg.volume_count);
	if (rc != EOK) {
		printf(NAME ": Warning, failed to obtain"
		    " block device size.\n");

		if (user_fs_size == 0) {
			printf(NAME ": You must specify the"
			    " filesystem size.\n");
			return 1;
		}
	} else {
		printf("Block device has %" PRIuOFF64 " blocks.\n",
		    cfg.volume_count);
	}

	if (user_fs_size != 0) {
		if (user_fs_size > cfg.volume_count * cfg.sector_size) {
			printf(NAME ": Error, the device is not big enough"
			    " to create a filesystem of"
			    " the specified size.\n");
			return 1;
		}

		cfg.volume_count = user_fs_size / cfg.sector_size;
	}

	cfg_params_initialize(&cfg);
	cfg_print_info(&cfg);

	if (cfg.total_clusters <= cfg.allocated_clusters + 2) {
		printf(NAME ": Error, insufficient disk space on device.\n");
		return 2;
	}

	printf("Writing the allocation table.\n");

	/* Initialize the FAT table */
	rc = fat_initialize(service_id, &cfg);
	if (rc != EOK) {
		printf(NAME ": Error, failed to write the FAT to disk\n");
		return 2;
	}

	/* Allocate clusters for the bitmap */
	rc = fat_allocate_clusters(service_id, &cfg, cfg.bitmap_cluster,
	    div_round_up(cfg.bitmap_size, cfg.cluster_size));
	if (rc != EOK) {
		printf(NAME ": Error, failed to allocate"
		    " clusters for bitmap.\n");
		return 2;
	}

	next_cls = cfg.bitmap_cluster +
	    div_round_up(cfg.bitmap_size, cfg.cluster_size);
	cfg.upcase_table_cluster = next_cls;

	/* Allocate clusters for the upcase table */
	rc = fat_allocate_clusters(service_id, &cfg, next_cls,
	    div_round_up(sizeof(upcase_table), cfg.cluster_size));
	if (rc != EOK) {
		printf(NAME ":Error, failed to allocate clusters"
		    " for the upcase table.\n");
		return 2;
	}

	next_cls += div_round_up(sizeof(upcase_table), cfg.cluster_size);
	cfg.rootdir_cluster = next_cls;

	/* Allocate a cluster for the root directory entry */
	rc = fat_allocate_clusters(service_id, &cfg, next_cls, 1);
	if (rc != EOK) {
		printf(NAME ": Error, failed to allocate cluster"
		    " for the root dentry.\n");
		return 2;
	}

	printf("Writing the allocation bitmap.\n");

	/* Write the allocation bitmap to disk */
	rc = bitmap_write(service_id, &cfg);
	if (rc != EOK) {
		printf(NAME ": Error, failed to write the allocation"
		    " bitmap to disk.\n");
		return 2;
	}

	printf("Writing the upcase table.\n");

	/* Write the upcase table to disk */
	rc = upcase_table_write(service_id, &cfg);
	if (rc != EOK) {
		printf(NAME ": Error, failed to write the"
		    " upcase table to disk.\n");
		return 2;
	}

	printf("Writing the root directory.\n");

	rc = root_dentries_write(service_id, &cfg);
	if (rc != EOK) {
		printf(NAME ": Error, failed to write the root directory"
		    " entries to disk.\n");
		return 2;
	}

	printf("Writing the boot sectors.\n");

	rc = bootsec_write(service_id, &cfg);
	if (rc != EOK) {
		printf(NAME ": Error, failed to write the VBR to disk\n");
		return 2;
	}

	printf("Success.\n");

	return 0;
}

/**
 * @}
 */

