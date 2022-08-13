/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdevice
 * @{
 */
/** @file
 */

#ifndef LIBDEVICE_TYPES_LABEL_H
#define LIBDEVICE_TYPES_LABEL_H

#include <types/uuid.h>

/** Partition contents */
typedef enum {
	/** Partition is considered empty */
	ptc_empty = 0,
	/** Partition contains a recognized filesystem */
	ptc_fs,
	/** Partition contains unrecognized data */
	ptc_unknown
} label_part_cnt_t;

/** Disk label type */
typedef enum {
	/** No label */
	lt_none,
	/** BIOS Master Boot Record */
	lt_mbr,
	/** UEFI GUID Partition Table */
	lt_gpt
} label_type_t;

#define LT_FIRST lt_mbr
#define LT_LIMIT (lt_gpt + 1)

#define LT_DEFAULT lt_mbr

/** Partition kind */
typedef enum {
	/** Primary partition */
	lpk_primary,
	/** Extended partition */
	lpk_extended,
	/** Logical partition */
	lpk_logical
} label_pkind_t;

/** Label flags */
typedef enum {
	/** Label supports extended (and logical) partitions */
	lf_ext_supp = 0x1,
	/** Partition type is in UUID format (otherwise in small number format) */
	lf_ptype_uuid = 0x2,
	/** Currently it is possible to create a primary partition */
	lf_can_create_pri = 0x4,
	/** Currently it is possible to create an extended partition */
	lf_can_create_ext = 0x8,
	/** Currently it is possible to create a logical partition */
	lf_can_create_log = 0x10,
	/** Currently it is possible to delete a partition */
	lf_can_delete_part = 0x20,
	/** Currently it is possible to modify a partition */
	lf_can_modify_part = 0x40
} label_flags_t;

/** Partition type format */
typedef enum {
	/** Small number */
	lptf_num,
	/** UUID */
	lptf_uuid
} label_pt_fmt;

/** Partition type */
typedef struct {
	/** Type format */
	label_pt_fmt fmt;
	/** Depending on @c fmt */
	union {
		/* Small number */
		uint8_t num;
		/** UUID */
		uuid_t uuid;
	} t;
} label_ptype_t;

/** Partition content (used to get partition type suggestion) */
typedef enum {
	/** ExFAT */
	lpc_exfat,
	/** Ext4 */
	lpc_ext4,
	/** FAT12 or FAT16 */
	lpc_fat12_16,
	/** FAT32 */
	lpc_fat32,
	/** Minix file system */
	lpc_minix
} label_pcnt_t;

#define LPC_LIMIT (lpc_minix + 1)

#endif

/** @}
 */
