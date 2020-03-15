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
#include <capa.h>
#include <loc.h>
#include <stdint.h>
#include <types/label.h>
#include <types/vol.h>
#include <vbd.h>

typedef enum {
	/** Primary partition space */
	spc_pri,
	/** Logical partition space */
	spc_log
} fdisk_spc_t;

/** Fdisk device flags */
typedef enum {
	/** Currently we can create a label */
	fdf_can_create_label = 0x1,
	/** Currently we can delete the label */
	fdf_can_delete_label = 0x2,
	/** Currently we can erase unknown data from disk */
	fdf_can_erase_dev = 0x4
} fdisk_dev_flags_t;

#define CU_LIMIT (cu_ybyte + 1)

/** List of devices available for managing by fdisk */
typedef struct {
	/** Fdisk instance */
	struct fdisk *fdisk;
	/** List of device info structures */
	list_t devinfos; /* of fdisk_dev_info_t */
} fdisk_dev_list_t;

/** Device information */
typedef struct {
	/** Containing device list */
	fdisk_dev_list_t *devlist;
	/** Link in fdisk_dev_list_t.devinfos */
	link_t ldevlist;
	/** Service ID */
	service_id_t svcid;
	/** Service name or NULL if not determined yet */
	char *svcname;
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
	capa_spec_t capacity;
	/** Partition kind */
	label_pkind_t pkind;
	/** Partition contents */
	vol_part_cnt_t pcnt;
	/** File system type */
	vol_fstype_t fstype;
	/** Partition ID */
	vbd_part_id_t part_id;
	/** Partition index */
	int index;
	/** First block */
	aoff64_t block0;
	/** Number of blocks */
	aoff64_t nblocks;
	/** Service ID */
	service_id_t svc_id;
	/** Volume label */
	char *label;
} fdisk_part_t;

/** Specification of new partition */
typedef struct {
	/** Desired capacity */
	capa_spec_t capacity;
	/** Partition kind */
	label_pkind_t pkind;
	/** File system type */
	vol_fstype_t fstype;
	/** Volume label */
	const char *label;
	/** Mount point */
	const char *mountp;
} fdisk_part_spec_t;

/** Partition info */
typedef struct {
	/** Capacity */
	capa_spec_t capacity;
	/** Partition kind */
	label_pkind_t pkind;
	/** Partition contents */
	vol_part_cnt_t pcnt;
	/** File system type */
	vol_fstype_t fstype;
	/** Volume label */
	char *label;
	/** Service ID */
	service_id_t svc_id;
} fdisk_part_info_t;

/** Free range iterator */
typedef struct {
	/** Device */
	fdisk_dev_t *dev;
	/** Primary or logical partition space */
	fdisk_spc_t spc;
	/** First block of free range */
	aoff64_t b0;
	/** Next partition following the free range or @c NULL if the range
	 * is at the end.
	 */
	fdisk_part_t *npart;
} fdisk_free_range_t;

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
