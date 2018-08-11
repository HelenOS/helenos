/*
 * Copyright (c) 2016 Jiri Svoboda
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

/** @addtogroup vbd
 * @{
 */
/**
 * @file
 * @brief
 */

#ifndef TYPES_VBDS_H_
#define TYPES_VBDS_H_

#include <adt/list.h>
#include <bd_srv.h>
#include <label/label.h>
#include <loc.h>
#include <refcount.h>
#include <stdbool.h>
#include <stddef.h>
#include <types/label.h>

typedef sysarg_t vbds_part_id_t;

typedef enum {
	/** No flags */
	vrf_none = 0,
	/** Force removal */
	vrf_force = 0x1
} vbds_rem_flag_t;

/** Partition */
typedef struct {
	/** Reader held during I/O */
	fibril_rwlock_t lock;
	/** Disk this partition belongs to */
	struct vbds_disk *disk;
	/** Link to vbds_disk_t.parts */
	link_t ldisk;
	/** Link to vbds_parts */
	link_t lparts;
	/** Partition ID */
	vbds_part_id_t pid;
	/** Service ID */
	service_id_t svc_id;
	/** Index under which partition is registered */
	int reg_idx;
	/** Label partition */
	label_part_t *lpart;
	/** Block device service */
	bd_srvs_t bds;
	/** Number of times the device is open */
	int open_cnt;
	/** Address of first block */
	aoff64_t block0;
	/** Number of blocks */
	aoff64_t nblocks;
	/** Reference count */
	atomic_refcount_t refcnt;
} vbds_part_t;

/** Disk */
typedef struct vbds_disk {
	/** Link to vbds_disks */
	link_t ldisks;
	/** Service ID */
	service_id_t svc_id;
	/** Disk service name */
	char *svc_name;
	/** Label */
	label_t *label;
	/** Partitions */
	list_t parts; /* of vbds_part_t */
	/** Block size */
	size_t block_size;
	/** Total number of blocks */
	aoff64_t nblocks;
	/** Used to mark disks still present during re-discovery */
	bool present;
} vbds_disk_t;

#endif

/** @}
 */
