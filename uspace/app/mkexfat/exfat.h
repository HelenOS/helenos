/*
 * Copyright (c) 2008 Jakub Jermar
 * Copyright (c) 2011 Oleg Romanenko
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

/** @addtogroup mkexfat
 * @{
 */

#include <stdint.h>
#include <stdbool.h>

#define EXFAT_FILENAME_LEN	255
#define EXFAT_NAME_PART_LEN	15
#define EXFAT_VOLLABEL_LEN	11

#define EXFAT_TYPE_UNUSED	0x00
#define EXFAT_TYPE_USED		0x80
#define EXFAT_TYPE_VOLLABEL	0x83
#define EXFAT_TYPE_BITMAP	0x81
#define EXFAT_TYPE_UCTABLE	0x82
#define EXFAT_TYPE_GUID		0xA0
#define EXFAT_TYPE_FILE		0x85
#define EXFAT_TYPE_STREAM	0xC0
#define EXFAT_TYPE_NAME		0xC1

#define EXFAT_ATTR_RDONLY	0x01
#define EXFAT_ATTR_HIDDEN	0x02
#define EXFAT_ATTR_SYSTEM	0x04
#define EXFAT_ATTR_SUBDIR	0x10
#define EXFAT_ATTR_ARCHIVE	0x20

/* All dentry structs should have 31 byte size */
typedef struct {
	uint8_t 	size;
	uint16_t 	label[11];
	uint8_t 	_reserved[8];
} __attribute__((packed)) exfat_vollabel_dentry_t;

typedef struct {
	uint8_t 	flags;
	uint8_t 	_reserved[18];
	uint32_t 	firstc;
	uint64_t 	size;
} __attribute__((packed)) exfat_bitmap_dentry_t;

typedef struct {
	uint8_t 	_reserved1[3];
	uint32_t 	checksum;
	uint8_t 	_reserved2[12];
	uint32_t 	firstc;
	uint64_t 	size;
} __attribute__((packed)) exfat_uctable_dentry_t;

typedef struct {
	uint8_t 	count; /* Always zero */
	uint16_t 	checksum;
	uint16_t 	flags;
	uint8_t 	guid[16];
	uint8_t 	_reserved[10];
} __attribute__((packed)) exfat_guid_dentry_t;

typedef struct {
	uint8_t 	count;
	uint16_t 	checksum;
	uint16_t 	attr;
	uint8_t 	_reserved1[2];
	uint32_t 	ctime;
	uint32_t 	mtime;
	uint32_t 	atime;
	uint8_t 	ctime_fine;
	uint8_t 	mtime_fine;
	uint8_t 	ctime_tz;
	uint8_t 	mtime_tz;
	uint8_t 	atime_tz;
	uint8_t 	_reserved2[7];
} __attribute__((packed)) exfat_file_dentry_t;

typedef struct {
	uint8_t 	flags;
	uint8_t 	_reserved1[1];
	uint8_t 	name_size;
	uint16_t 	hash;
	uint8_t 	_reserved2[2];
	uint64_t 	valid_data_size;
	uint8_t 	_reserved3[4];
	uint32_t 	firstc;
	uint64_t 	data_size;
} __attribute__((packed)) exfat_stream_dentry_t;

typedef struct {
	uint8_t 	flags;
	uint16_t 	name[EXFAT_NAME_PART_LEN];
} __attribute__((packed)) exfat_name_dentry_t;

typedef struct {
	uint8_t type;
	union {
		exfat_vollabel_dentry_t	vollabel;
		exfat_bitmap_dentry_t 	bitmap;
		exfat_uctable_dentry_t 	uctable;
		exfat_guid_dentry_t 	guid;
		exfat_file_dentry_t 	file;
		exfat_stream_dentry_t 	stream;
		exfat_name_dentry_t 	name;
	};
} __attribute__((packed)) exfat_dentry_t;

typedef enum {
	EXFAT_DENTRY_SKIP,
	EXFAT_DENTRY_LAST,
	EXFAT_DENTRY_FREE,
	EXFAT_DENTRY_VOLLABEL,
	EXFAT_DENTRY_BITMAP,
	EXFAT_DENTRY_UCTABLE,
	EXFAT_DENTRY_GUID,
	EXFAT_DENTRY_FILE,
	EXFAT_DENTRY_STREAM,
	EXFAT_DENTRY_NAME
} exfat_dentry_clsf_t;

typedef struct exfat_bs {
	uint8_t jump[3];            /* 0x00 jmp and nop instructions */
	uint8_t oem_name[8];        /* 0x03 "EXFAT   " */
	uint8_t	__reserved[53];     /* 0x0B always 0 */
	uint64_t volume_start;      /* 0x40 partition first sector */
	uint64_t volume_count;      /* 0x48 partition sectors count */
	uint32_t fat_sector_start;  /* 0x50 FAT first sector */
	uint32_t fat_sector_count;  /* 0x54 FAT sectors count */
	uint32_t data_start_sector; /* 0x58 Data region first cluster sector */
	uint32_t data_clusters;     /* 0x5C total clusters count */
	uint32_t rootdir_cluster;   /* 0x60 first cluster of the root dir */
	uint32_t volume_serial;     /* 0x64 volume serial number */
	struct {                    /* 0x68 FS version */
		uint8_t minor;
		uint8_t major;
	} __attribute__((packed)) version;
	uint16_t volume_flags;     /* 0x6A volume state flags */
	uint8_t bytes_per_sector;  /* 0x6C sector size as (1 << n) */
	uint8_t sec_per_cluster;   /* 0x6D sectors per cluster as (1 << n) */
	uint8_t fat_count;         /* 0x6E always 1 */
	uint8_t drive_no;          /* 0x6F always 0x80 */
	uint8_t allocated_percent; /* 0x70 percentage of allocated space */
	uint8_t _reserved2[7];     /* 0x71 reserved */
	uint8_t bootcode[390];     /* Boot code */
	uint16_t signature;        /* the value of 0xAA55 */
} __attribute__((__packed__)) exfat_bs_t;

/**
 * @}
 */
