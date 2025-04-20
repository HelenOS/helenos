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
#include <stdatomic.h>

#include "fge.h"
#include "superblock.h"

#define NAME		"hr"
#define HR_STRIP_SIZE	DATA_XFER_LIMIT

struct hr_volume;
typedef struct hr_volume hr_volume_t;
typedef struct hr_metadata hr_metadata_t;
typedef struct hr_superblock_ops hr_superblock_ops_t;

typedef struct hr_ops {
	errno_t		(*create)(hr_volume_t *);
	errno_t		(*init)(hr_volume_t *);
	void		(*status_event)(hr_volume_t *);
	errno_t		(*add_hotspare)(hr_volume_t *, service_id_t);
} hr_ops_t;

typedef struct hr_volume {
	link_t		 lvolumes;		/* link to all volumes list */
	hr_ops_t	 hr_ops;		/* level init and create fcns */
	bd_srvs_t	 hr_bds;		/* block interface to the vol */
	service_id_t	 svc_id;		/* service id */

	fibril_mutex_t	 lock;			/* XXX: gone after para */
	list_t		 range_lock_list;	/* list of range locks */
	fibril_mutex_t	 range_lock_list_lock;	/* range locks list lock */
	hr_fpool_t	*fge;			/* fibril pool */

	void		*in_mem_md;
	fibril_mutex_t	 md_lock;		/* lock protecting in_mem_md */

	hr_superblock_ops_t *meta_ops;

	/* invariants */
	size_t		 extent_no;		/* number of extents */
	size_t		 bsize;			/* block size */
	uint64_t	 nblocks;		/* no. of all usable blocks */
	uint64_t	 truncated_blkno;	/* blkno per extent */
	uint64_t	 data_blkno;		/* no. of user usable blocks */
	uint64_t	 data_offset;		/* user data offset in blocks */
	uint32_t	 strip_size;		/* strip size */
	hr_level_t	 level;			/* volume level */
	uint8_t		 layout;		/* RAID Level Qualifier */
	char		 devname[HR_DEVNAME_LEN];

	hr_extent_t	 extents[HR_MAX_EXTENTS];
	fibril_rwlock_t	 extents_lock;		/* extent service id lock */

	size_t		 hotspare_no;		/* no. of available hotspares */
	hr_extent_t	 hotspares[HR_MAX_HOTSPARES];
	fibril_mutex_t	 hotspare_lock;		/* lock protecting hotspares */

	fibril_rwlock_t	 states_lock;		/* states lock */

	_Atomic bool	 state_dirty;		/* dirty state */

	/* XXX: atomic_uint_least64_t? */
	_Atomic uint64_t rebuild_blk;		/* rebuild position */
	_Atomic int	 open_cnt;		/* open/close() counter */
	hr_vol_status_t	 status;		/* volume status */
	void		 (*state_callback)(hr_volume_t *, size_t, errno_t);
} hr_volume_t;

typedef enum {
	HR_BD_SYNC,
	HR_BD_READ,
	HR_BD_WRITE
} hr_bd_op_type_t;

/* macros for hr_metadata_save() */
#define	WITH_STATE_CALLBACK true
#define	NO_STATE_CALLBACK false

typedef struct hr_range_lock {
	link_t		 link;
	fibril_mutex_t	 lock;
	hr_volume_t	*vol;		/* back-pointer to volume */
	uint64_t	 off;		/* start of the range */
	uint64_t	 len;		/* length of the range */

	size_t		 pending;	/* prot. by vol->range_lock_list_lock */
	bool		 ignore;	/* prot. by vol->range_lock_list_lock */
} hr_range_lock_t;

extern errno_t		hr_raid0_create(hr_volume_t *);
extern errno_t 		hr_raid1_create(hr_volume_t *);
extern errno_t 		hr_raid5_create(hr_volume_t *);

extern errno_t		hr_raid0_init(hr_volume_t *);
extern errno_t 		hr_raid1_init(hr_volume_t *);
extern errno_t 		hr_raid5_init(hr_volume_t *);

extern void		hr_raid0_status_event(hr_volume_t *);
extern void		hr_raid1_status_event(hr_volume_t *);
extern void		hr_raid5_status_event(hr_volume_t *);

extern errno_t		hr_raid1_add_hotspare(hr_volume_t *, service_id_t);
extern errno_t		hr_raid5_add_hotspare(hr_volume_t *, service_id_t);

#endif

/** @}
 */
