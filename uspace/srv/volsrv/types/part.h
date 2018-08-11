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
