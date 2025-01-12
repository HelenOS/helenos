/*
 * Copyright (c) 2025 Miroslav Cimerman
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

/** @addtogroup hr
 * @{
 */
/**
 * @file
 */

#ifndef _HR_VAR_H
#define _HR_VAR_H

#include <adt/list.h>
#include <bd_srv.h>
#include <errno.h>
#include <fibril_synch.h>
#include <hr.h>

#include "fge.h"

#define NAME "hr"

#define HR_STRIP_SIZE DATA_XFER_LIMIT

struct hr_volume;
typedef struct hr_volume hr_volume_t;

typedef struct hr_ops {
	errno_t (*create)(hr_volume_t *);
	errno_t (*init)(hr_volume_t *);
	void	(*status_event)(hr_volume_t *);
	errno_t	(*add_hotspare)(hr_volume_t *, service_id_t);
} hr_ops_t;

typedef struct hr_deferred_invalidation {
	link_t link;
	size_t index;
	service_id_t svc_id;
} hr_deferred_invalidation_t;

typedef struct hr_volume {
	hr_ops_t hr_ops;
	bd_srvs_t hr_bds;

	link_t lvolumes; /* protected by static hr_volumes_lock in hr.c */

	/*
	 * XXX: will be gone after all paralelization, but still used
	 * in yet-unparallelized levels
	 */
	fibril_mutex_t lock;

	list_t range_lock_list;
	fibril_mutex_t range_lock_list_lock;

	hr_fpool_t *fge;

	/* after assembly, these are invariant */
	size_t extent_no;
	size_t bsize;
	uint64_t nblocks;
	uint64_t data_blkno;
	uint64_t data_offset; /* in blocks */
	uint32_t strip_size;
	hr_level_t level;
	uint8_t layout; /* RAID Level Qualifier */
	service_id_t svc_id;
	char devname[HR_DEVNAME_LEN];

	size_t hotspare_no;
	hr_extent_t hotspares[HR_MAX_HOTSPARES + HR_MAX_EXTENTS];

	/* protects hotspares (hotspares.{svc_id,status}, hotspare_no) */
	fibril_mutex_t hotspare_lock;

	hr_extent_t extents[HR_MAX_EXTENTS];
	/* protects extents ordering (extents.svc_id) */
	fibril_rwlock_t extents_lock;
	/* protects states (extents.status, hr_volume_t.status) */
	fibril_rwlock_t states_lock;

	/* for halting IO requests when a REBUILD start waits */
	bool halt_please;
	fibril_mutex_t halt_lock;

	/*
	 * For deferring invalidations of extents. Used when
	 * an extent has to be invalidated (got ENOMEM on a WRITE),
	 * but workers - therefore state callbacks cannot lock
	 * extents for writing (they are readers), so invalidations
	 * are harvested later when we are able to.
	 */
	fibril_mutex_t deferred_list_lock;
	list_t deferred_invalidations_list;
	hr_deferred_invalidation_t deferred_inval[HR_MAX_EXTENTS];

	_Atomic uint64_t rebuild_blk;
	uint64_t counter; /* metadata syncing */
	hr_vol_status_t status;
} hr_volume_t;

typedef enum {
	HR_BD_SYNC,
	HR_BD_READ,
	HR_BD_WRITE
} hr_bd_op_type_t;

typedef struct hr_range_lock {
	fibril_mutex_t lock;
	link_t link;
	hr_volume_t *vol;
	uint64_t off;
	uint64_t len;
	size_t pending; /* protected by vol->range_lock_list_lock */
	bool ignore; /* protected by vol->range_lock_list_lock */
} hr_range_lock_t;

extern errno_t hr_init_devs(hr_volume_t *);
extern void hr_fini_devs(hr_volume_t *);

extern errno_t hr_raid0_create(hr_volume_t *);
extern errno_t hr_raid1_create(hr_volume_t *);
extern errno_t hr_raid5_create(hr_volume_t *);

extern errno_t hr_raid0_init(hr_volume_t *);
extern errno_t hr_raid1_init(hr_volume_t *);
extern errno_t hr_raid5_init(hr_volume_t *);

extern void hr_raid0_status_event(hr_volume_t *);
extern void hr_raid1_status_event(hr_volume_t *);
extern void hr_raid5_status_event(hr_volume_t *);

extern errno_t hr_raid1_add_hotspare(hr_volume_t *, service_id_t);
extern errno_t hr_raid5_add_hotspare(hr_volume_t *, service_id_t);

#endif

/** @}
 */
