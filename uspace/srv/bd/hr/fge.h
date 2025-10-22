/*
 * Copyright (c) 2025 Miroslav Cimerman
 * Copyright (c) 2024 Vojtech Horky
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

#ifndef _HR_FGE_H
#define _HR_FGE_H

#include <adt/bitmap.h>
#include <adt/circ_buf.h>
#include <errno.h>
#include <stddef.h>

/* forward declarations */
typedef struct hr_fpool hr_fpool_t;
typedef struct hr_fgroup hr_fgroup_t;
typedef struct fge_fibril_data fge_fibril_data_t;
typedef struct wu_queue wu_queue_t;

typedef errno_t (*hr_wu_t)(void *);

struct fge_fibril_data {
	hr_wu_t wu; /* work unit function pointer */
	void *arg; /* work unit function argument */
	hr_fgroup_t *group; /* back-pointer to group */
	ssize_t memslot; /* index to pool bitmap slot */
};

struct wu_queue {
	fibril_mutex_t lock;
	fibril_condvar_t not_empty;
	fibril_condvar_t not_full;
	fge_fibril_data_t *fexecs; /* circ-buf memory */
	circ_buf_t cbuf;
};

struct hr_fpool {
	fibril_mutex_t lock;
	bitmap_t bitmap; /* memory slot bitmap */
	wu_queue_t queue;
	fid_t *fibrils;
	uint8_t *wu_storage; /* pre-allocated pool storage */
	size_t fibril_cnt;
	size_t max_wus;
	size_t active_groups;
	bool stop;
	size_t wu_size;
	size_t wu_storage_free_count;
	fibril_condvar_t all_wus_done;
};

struct hr_fgroup {
	hr_fpool_t *pool;/* back-pointer to pool */
	size_t wu_cnt;/* upper bound of work units */
	size_t submitted; /* number of submitted jobs */
	size_t reserved_cnt; /* no. of reserved wu storage slots */
	size_t reserved_avail;
	size_t *memslots; /* indices to pool bitmap */
	void *own_mem; /* own allocated memory */
	size_t own_used; /* own memory slots used counter */
	errno_t final_errno; /* agreggated errno */
	size_t finished_okay; /* no. of wus that ended with EOK */
	size_t finished_fail; /* no. of wus that ended with != EOK */
	fibril_mutex_t lock;
	fibril_condvar_t all_done;
};

extern hr_fpool_t *hr_fpool_create(size_t, size_t, size_t);
extern void hr_fpool_destroy(hr_fpool_t *);
extern hr_fgroup_t *hr_fgroup_create(hr_fpool_t *, size_t);
extern void *hr_fgroup_alloc(hr_fgroup_t *);
extern void hr_fgroup_submit(hr_fgroup_t *, hr_wu_t, void *);
extern errno_t hr_fgroup_wait(hr_fgroup_t *, size_t *, size_t *);

#endif

/** @}
 */
