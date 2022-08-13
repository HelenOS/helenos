/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup volsrv
 * @{
 */
/**
 * @file
 * @brief
 */

#ifndef TYPES_PART_H_
#define TYPES_PART_H_

#include <adt/list.h>
#include <refcount.h>
#include <fibril_synch.h>
#include <stdbool.h>
#include <types/label.h>

/** Partition */
typedef struct {
	/** Containing partition list */
	struct vol_parts *parts;
	/** Link to vol_parts */
	link_t lparts;
	/** Reference count */
	atomic_refcount_t refcnt;
	/** Service ID */
	service_id_t svc_id;
	/** Service name */
	char *svc_name;
	/** Partition contents */
	vol_part_cnt_t pcnt;
	/** Filesystem type */
	vol_fstype_t fstype;
	/** Volume label */
	char *label;
	/** Where volume is currently mounted */
	char *cur_mp;
	/** Mounted at automatic mount point */
	bool cur_mp_auto;
	/** Volume */
	struct vol_volume *volume;
} vol_part_t;

/** Partitions */
typedef struct vol_parts {
	/** Synchronize access to list of partitions */
	fibril_mutex_t lock;
	/** Partitions (list of vol_part_t) */
	list_t parts;
	/** Underlying volumes */
	struct vol_volumes *volumes;
} vol_parts_t;

#endif

/** @}
 */
