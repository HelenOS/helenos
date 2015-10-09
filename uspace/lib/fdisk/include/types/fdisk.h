/*
 * Copyright (c) 2015 Jiri Svoboda
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

/** @addtogroup libfdisk
 * @{
 */
/**
 * @file Disk management library types.
 */

#ifndef LIBFDISK_TYPES_H_
#define LIBFDISK_TYPES_H_

#include <adt/list.h>
#include <loc.h>
#include <stdint.h>
#include <types/label.h>
#include <vbd.h>
#include <vol.h>

typedef enum {
	cu_byte = 0,
	cu_kbyte,
	cu_mbyte,
	cu_gbyte,
	cu_tbyte,
	cu_pbyte,
	cu_ebyte,
	cu_zbyte,
	cu_ybyte
} fdisk_cunit_t;

#define CU_LIMIT (cu_ybyte + 1)

/** File system type */
typedef enum {
	fdfs_none = 0,
	fdfs_unknown,
	fdfs_exfat,
	fdfs_fat,
	fdfs_minix,
	fdfs_ext4
} fdisk_fstype_t;

/** Highest fstype value + 1 */
#define FDFS_LIMIT (fdfs_ext4 + 1)
/** Lowest fstype allowed for creation */
#define FDFS_CREATE_LO fdfs_exfat
/** Highest fstype allowed for creation + 1 */
#define FDFS_CREATE_HI (fdfs_ext4 + 1)

/** Partition capacity */
typedef struct {
	uint64_t value;
	fdisk_cunit_t cunit;
} fdisk_cap_t;

/** List of devices available for managing by fdisk */
typedef struct {
	list_t devinfos; /* of fdisk_dev_info_t */
} fdisk_dev_list_t;

/** Device information */
typedef struct {
	/** Containing device list */
	fdisk_dev_list_t *devlist;
	/** Link in fdisk_dev_list_t.devinfos */
	link_t ldevlist;
	service_id_t svcid;
	/** Service name or NULL if not determined yet */
	char *svcname;
	bool blk_inited;
} fdisk_dev_info_t;

/** Open fdisk device */
typedef struct {
	/** Fdisk instance */
	struct fdisk *fdisk;
	/** Service ID */
	service_id_t sid;
	/** All partitions */
	list_t parts;
	/** Primary partitions sorted by index */
	list_t pri_idx; /* of fdisk_part_t */
	/** Primary partitions sorted by block address */
	list_t pri_ba;
	/** Logical partitions sorted by block address */
	list_t log_ba;
	/** Extended partition or NULL */
	struct fdisk_part *ext_part;
	/** Disk info */
	vbd_disk_info_t dinfo;
	/** Alignment in blocks */
	uint64_t align;
} fdisk_dev_t;

typedef struct {
	/** Label type */
	label_type_t ltype;
	/** Label flags */
	label_flags_t flags;
} fdisk_label_info_t;

/** Partition */
typedef struct fdisk_part {
	/** Containing device */
	fdisk_dev_t *dev;
	/** Link to fdisk_dev_t.parts */
	link_t lparts;
	/** Link to fdisk_dev_t.pri_idx */
	link_t lpri_idx;
	/** Link to fdisk_dev_t.pri_ba */
	link_t lpri_ba;
	/** Link to fdisk_dev_t.log_ba */
	link_t llog_ba;
	/** Capacity */
	fdisk_cap_t capacity;
	/** Partition kind */
	label_pkind_t pkind;
	/** File system type */
	fdisk_fstype_t fstype;
	/** Partition ID */
	vbd_part_id_t part_id;
	/** Partition index */
	int index;
	/** First block */
	aoff64_t block0;
	/** Number of blocks */
	aoff64_t nblocks;
} fdisk_part_t;

/** Specification of new partition */
typedef struct {
	/** Desired capacity */
	fdisk_cap_t capacity;
	/** Partition kind */
	label_pkind_t pkind;
	/** File system type */
	fdisk_fstype_t fstype;
} fdisk_part_spec_t;

/** Partition info */
typedef struct {
	/** Capacity */
	fdisk_cap_t capacity;
	/** Partition kind */
	label_pkind_t pkind;
	/** File system type */
	fdisk_fstype_t fstype;
} fdisk_part_info_t;

/** Fdisk instance */
typedef struct fdisk {
	/** Volume service */
	vol_t *vol;
	/** Virtual Block Device */
	vbd_t *vbd;
} fdisk_t;

#endif

/** @}
 */
