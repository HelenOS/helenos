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

#include <abi/ipc/ipc.h>
#include <bd_srv.h>
#include <block.h>
#include <errno.h>
#include <hr.h>
#include <io/log.h>
#include <ipc/hr.h>
#include <ipc/services.h>
#include <loc.h>
#include <task.h>
#include <stdio.h>
#include <stdlib.h>
#include <str_error.h>

#include "io.h"
#include "superblock.h"
#include "util.h"
#include "var.h"

static void	hr_raid0_update_vol_status(hr_volume_t *);
static void	hr_raid0_state_callback(hr_volume_t *, size_t, errno_t);
static errno_t	hr_raid0_bd_op(hr_bd_op_type_t, bd_srv_t *, aoff64_t, size_t,
    void *, const void *, size_t);

/* bdops */
static errno_t	hr_raid0_bd_open(bd_srvs_t *, bd_srv_t *);
static errno_t 	hr_raid0_bd_close(bd_srv_t *);
static errno_t 	hr_raid0_bd_read_blocks(bd_srv_t *, aoff64_t, size_t, void *,
    size_t);
static errno_t	hr_raid0_bd_sync_cache(bd_srv_t *, aoff64_t, size_t);
static errno_t	hr_raid0_bd_write_blocks(bd_srv_t *, aoff64_t, size_t,
    const void *, size_t);
static errno_t	hr_raid0_bd_get_block_size(bd_srv_t *, size_t *);
static errno_t	hr_raid0_bd_get_num_blocks(bd_srv_t *, aoff64_t *);

static bd_ops_t hr_raid0_bd_ops = {
	.open		= hr_raid0_bd_open,
	.close		= hr_raid0_bd_close,
	.sync_cache	= hr_raid0_bd_sync_cache,
	.read_blocks	= hr_raid0_bd_read_blocks,
	.write_blocks	= hr_raid0_bd_write_blocks,
	.get_block_size	= hr_raid0_bd_get_block_size,
	.get_num_blocks	= hr_raid0_bd_get_num_blocks
};

extern loc_srv_t *hr_srv;

errno_t hr_raid0_create(hr_volume_t *new_volume)
{
	HR_DEBUG("%s()", __func__);

	assert(new_volume->level == HR_LVL_0);

	if (new_volume->extent_no < 2) {
		HR_ERROR("RAID 0 array needs at least 2 devices\n");
		return EINVAL;
	}

	hr_raid0_update_vol_status(new_volume);
	if (new_volume->status != HR_VOL_ONLINE)
		return EINVAL;

	bd_srvs_init(&new_volume->hr_bds);
	new_volume->hr_bds.ops = &hr_raid0_bd_ops;
	new_volume->hr_bds.sarg = new_volume;

	new_volume->state_callback = hr_raid0_state_callback;

	return EOK;
}

/*
 * Called only once in volume's lifetime.
 */
errno_t hr_raid0_init(hr_volume_t *vol)
{
	HR_DEBUG("%s()", __func__);

	assert(vol->level == HR_LVL_0);

	uint64_t truncated_blkno = vol->extents[0].blkno;
	for (size_t i = 1; i < vol->extent_no; i++) {
		if (vol->extents[i].blkno < truncated_blkno)
			truncated_blkno = vol->extents[i].blkno;
	}

	uint64_t total_blkno = truncated_blkno * vol->extent_no;

	vol->truncated_blkno = truncated_blkno;
	vol->nblocks = total_blkno;
	vol->data_offset = HR_DATA_OFF;

	vol->data_blkno = total_blkno;
	vol->data_blkno -= HR_META_SIZE * vol->extent_no; /* count md blocks */

	vol->strip_size = HR_STRIP_SIZE;

	return EOK;
}

void hr_raid0_status_event(hr_volume_t *vol)
{
	HR_DEBUG("%s()", __func__);

	hr_raid0_update_vol_status(vol);
}

static errno_t hr_raid0_bd_open(bd_srvs_t *bds, bd_srv_t *bd)
{
	HR_DEBUG("%s()", __func__);

	hr_volume_t *vol = bd->srvs->sarg;

	atomic_fetch_add_explicit(&vol->open_cnt, 1, memory_order_relaxed);

	return EOK;
}

static errno_t hr_raid0_bd_close(bd_srv_t *bd)
{
	HR_DEBUG("%s()", __func__);

	hr_volume_t *vol = bd->srvs->sarg;

	atomic_fetch_sub_explicit(&vol->open_cnt, 1, memory_order_relaxed);

	return EOK;
}

static errno_t hr_raid0_bd_sync_cache(bd_srv_t *bd, aoff64_t ba, size_t cnt)
{
	return hr_raid0_bd_op(HR_BD_SYNC, bd, ba, cnt, NULL, NULL, 0);
}

static errno_t hr_raid0_bd_read_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    void *buf, size_t size)
{
	return hr_raid0_bd_op(HR_BD_READ, bd, ba, cnt, buf, NULL, size);
}

static errno_t hr_raid0_bd_write_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    const void *data, size_t size)
{
	return hr_raid0_bd_op(HR_BD_WRITE, bd, ba, cnt, NULL, data, size);
}

static errno_t hr_raid0_bd_get_block_size(bd_srv_t *bd, size_t *rsize)
{
	hr_volume_t *vol = bd->srvs->sarg;

	*rsize = vol->bsize;
	return EOK;
}

static errno_t hr_raid0_bd_get_num_blocks(bd_srv_t *bd, aoff64_t *rnb)
{
	hr_volume_t *vol = bd->srvs->sarg;

	*rnb = vol->data_blkno;
	return EOK;
}

static void hr_raid0_update_vol_status(hr_volume_t *vol)
{
	fibril_mutex_lock(&vol->md_lock);

	/* XXX: will be wrapped in md specific fcn ptrs */
	vol->in_mem_md->counter++;

	fibril_mutex_unlock(&vol->md_lock);

	fibril_rwlock_read_lock(&vol->states_lock);

	hr_vol_status_t old_state = vol->status;

	for (size_t i = 0; i < vol->extent_no; i++) {
		if (vol->extents[i].status != HR_EXT_ONLINE) {
			fibril_rwlock_read_unlock(&vol->states_lock);

			if (old_state != HR_VOL_FAULTY) {
				fibril_rwlock_write_lock(&vol->states_lock);
				hr_update_vol_status(vol, HR_VOL_FAULTY);
				fibril_rwlock_write_unlock(&vol->states_lock);
			}
			return;
		}
	}
	fibril_rwlock_read_unlock(&vol->states_lock);

	if (old_state != HR_VOL_ONLINE) {
		fibril_rwlock_write_lock(&vol->states_lock);
		hr_update_vol_status(vol, HR_VOL_ONLINE);
		fibril_rwlock_write_unlock(&vol->states_lock);
	}
}

static void hr_raid0_state_callback(hr_volume_t *vol, size_t extent, errno_t rc)
{
	if (rc == EOK)
		return;

	fibril_rwlock_write_lock(&vol->states_lock);

	switch (rc) {
	case ENOENT:
		hr_update_ext_status(vol, extent, HR_EXT_MISSING);
		break;
	default:
		hr_update_ext_status(vol, extent, HR_EXT_FAILED);
	}

	hr_update_vol_status(vol, HR_VOL_FAULTY);

	fibril_rwlock_write_unlock(&vol->states_lock);
}

static errno_t hr_raid0_bd_op(hr_bd_op_type_t type, bd_srv_t *bd, aoff64_t ba,
    size_t cnt, void *dst, const void *src, size_t size)
{
	hr_volume_t *vol = bd->srvs->sarg;
	errno_t rc;
	uint64_t phys_block, len;
	size_t left;
	const uint8_t *data_write = src;
	uint8_t *data_read = dst;

	fibril_rwlock_read_lock(&vol->states_lock);
	if (vol->status != HR_VOL_ONLINE) {
		fibril_rwlock_read_unlock(&vol->states_lock);
		return EIO;
	}
	fibril_rwlock_read_unlock(&vol->states_lock);

	/* propagate sync */
	if (type == HR_BD_SYNC && ba == 0 && cnt == 0) {
		hr_fgroup_t *group = hr_fgroup_create(vol->fge,
		    vol->extent_no);
		if (group == NULL)
			return ENOMEM;

		for (size_t i = 0; i < vol->extent_no; i++) {
			hr_io_t *io = hr_fgroup_alloc(group);
			io->extent = i;
			io->ba = ba;
			io->cnt = cnt;
			io->type = type;
			io->vol = vol;

			hr_fgroup_submit(group, hr_io_worker, io);
		}

		size_t bad;
		rc = hr_fgroup_wait(group, NULL, &bad);
		if (rc == ENOMEM)
			return ENOMEM;

		if (bad > 0)
			return EIO;

		return EOK;
	}

	if (type == HR_BD_READ || type == HR_BD_WRITE)
		if (size < cnt * vol->bsize)
			return EINVAL;

	rc = hr_check_ba_range(vol, cnt, ba);
	if (rc != EOK)
		return rc;

	uint64_t strip_size = vol->strip_size / vol->bsize; /* in blocks */
	uint64_t strip_no = ba / strip_size;
	uint64_t extent = strip_no % vol->extent_no;
	uint64_t stripe = strip_no / vol->extent_no;
	uint64_t strip_off = ba % strip_size;

	left = cnt;

	/* calculate how many strips does the IO span */
	size_t end_strip_no = (ba + cnt - 1) / strip_size;
	size_t span = end_strip_no - strip_no + 1;

	hr_fgroup_t *group = hr_fgroup_create(vol->fge, span);
	if (group == NULL)
		return ENOMEM;

	while (left != 0) {
		phys_block = stripe * strip_size + strip_off;
		cnt = min(left, strip_size - strip_off);
		len = vol->bsize * cnt;
		hr_add_ba_offset(vol, &phys_block);

		hr_io_t *io = hr_fgroup_alloc(group);
		io->extent = extent;
		io->data_write = data_write;
		io->data_read = data_read;
		io->ba = phys_block;
		io->cnt = cnt;
		io->type = type;
		io->vol = vol;

		hr_fgroup_submit(group, hr_io_worker, io);

		left -= cnt;
		if (left == 0)
			break;

		if (type == HR_BD_READ)
			data_read += len;
		else if (type == HR_BD_WRITE)
			data_write += len;

		strip_off = 0;
		extent++;
		if (extent >= vol->extent_no) {
			stripe++;
			extent = 0;
		}
	}

	size_t bad;
	rc = hr_fgroup_wait(group, NULL, &bad);
	if (rc == ENOMEM && type == HR_BD_READ)
		return ENOMEM;

	if (bad > 0)
		return EIO;

	return EOK;
}

/** @}
 */
