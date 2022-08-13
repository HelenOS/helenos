/*
 * SPDX-FileCopyrightText: 2008 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup liblabel
 * @{
 */

#ifndef FAT_FAT_H_
#define FAT_FAT_H_

#include <stdint.h>

#define BS_BLOCK		0
#define BS_SIZE			512
#define DIRENT_SIZE		32

#define FAT12_CLST_MAX    4085
#define FAT16_CLST_MAX    65525

#define FATAUTO	0
#define FAT12	12
#define FAT16	16
#define FAT32	32

#define FAT_CLUSTER_DOUBLE_SIZE(a) ((a) / 4)

typedef struct fat_bs {
	/** Jump instruction */
	uint8_t		ji[3];
	uint8_t		oem_name[8];

	/* BIOS Parameter Block */

	/** Bytes per sector */
	uint16_t	bps;
	/** Sectors per cluster */
	uint8_t		spc;
	/** Reserved sector count */
	uint16_t	rscnt;
	/** Number of FATs */
	uint8_t		fatcnt;
	/** Maximum number of root directory entries */
	uint16_t	root_ent_max;
	/** Total sectors. 16-bit version */
	uint16_t	totsec16;
	/** Media descriptor */
	uint8_t		mdesc;
	/** Sectors per FAT12/FAT16 */
	uint16_t	sec_per_fat;
	/** Sectors per track */
	uint16_t	sec_per_track;
	/** Number of heads */
	uint16_t	headcnt;
	/** Hidden sectors */
	uint32_t	hidden_sec;
	/** Total sectors. 32-bit version */
	uint32_t	totsec32;

	union {
		struct {
			/* FAT12/FAT16 only: Extended BIOS Parameter Block */
			/** Physical drive number. */
			uint8_t		pdn;
			uint8_t		reserved;
			/** Extended boot signature. */
			uint8_t		ebs;
			/** Serial number. */
			uint32_t	id;
			/** Volume label. */
			uint8_t		label[11];
			/** FAT type. */
			uint8_t		type[8];
			/** Boot code. */
			uint8_t		boot_code[448];
			/** Boot sector signature. */
			uint16_t	signature;
		} __attribute__((packed));
		struct {
			/* FAT32 only */
			/** Sectors per FAT. */
			uint32_t	sectors_per_fat;
			/** FAT flags. */
			uint16_t	flags;
			/** Version. */
			uint16_t	version;
			/** Cluster number of root directory. */
			uint32_t	root_cluster;
			/** Sector number of file system information sector. */
			uint16_t	fsinfo_sec;
			/** Sector number of boot sector copy. */
			uint16_t	bscopy_sec;
			uint8_t		reserved1[12];
			/** Physical drive number. */
			uint8_t		pdn;
			uint8_t		reserved2;
			/** Extended boot signature. */
			uint8_t		ebs;
			/** Serial number. */
			uint32_t	id;
			/** Volume label. */
			uint8_t		label[11];
			/** FAT type. */
			uint8_t		type[8];
			/** Boot code. */
			uint8_t		boot_code[420];
			/** Signature. */
			uint16_t	signature;
		} __attribute__((packed)) fat32;
	};
} __attribute__((packed)) fat_bs_t;

#endif

/**
 * @}
 */
