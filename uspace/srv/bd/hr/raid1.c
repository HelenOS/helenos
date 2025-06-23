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
#include <inttypes.h>
#include <io/log.h>
#include <ipc/hr.h>
#include <ipc/services.h>
#include <loc.h>
#include <task.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <str_error.h>

#include "fge.h"
#include "io.h"
#include "superblock.h"
#include "util.h"
#include "var.h"

static void hr_raid1_vol_state_eval_forced(hr_volume_t *);
static size_t hr_raid1_count_good_extents(hr_volume_t *, uint64_t, size_t,
    uint64_t);
static errno_t hr_raid1_bd_op(hr_bd_op_type_t, hr_volume_t *, aoff64_t, size_t,
    void *, const void *, size_t);
static errno_t hr_raid1_rebuild(void *);

/* bdops */
static errno_t hr_raid1_bd_open(bd_srvs_t *, bd_srv_t *);
static errno_t hr_raid1_bd_close(bd_srv_t *);
static errno_t hr_raid1_bd_read_blocks(bd_srv_t *, aoff64_t, size_t, void *,
    size_t);
static errno_t hr_raid1_bd_sync_cache(bd_srv_t *, aoff64_t, size_t);
static errno_t hr_raid1_bd_write_blocks(bd_srv_t *, aoff64_t, size_t,
    const void *, size_t);
static errno_t hr_raid1_bd_get_block_size(bd_srv_t *, size_t *);
static errno_t hr_raid1_bd_get_num_blocks(bd_srv_t *, aoff64_t *);

static bd_ops_t hr_raid1_bd_ops = {
	.open = hr_raid1_bd_open,
	.close = hr_raid1_bd_close,
	.sync_cache = hr_raid1_bd_sync_cache,
	.read_blocks = hr_raid1_bd_read_blocks,
	.write_blocks = hr_raid1_bd_write_blocks,
	.get_block_size = hr_raid1_bd_get_block_size,
	.get_num_blocks = hr_raid1_bd_get_num_blocks
};

extern loc_srv_t *hr_srv;

errno_t hr_raid1_create(hr_volume_t *new_volume)
{
	HR_DEBUG("%s()", __func__);

	if (new_volume->level != HR_LVL_1)
		return EINVAL;

	if (new_volume->extent_no < 2) {
		HR_ERROR("RAID 1 volume needs at least 2 devices\n");
		return EINVAL;
	}

	bd_srvs_init(&new_volume->hr_bds);
	new_volume->hr_bds.ops = &hr_raid1_bd_ops;
	new_volume->hr_bds.sarg = new_volume;

	hr_raid1_vol_state_eval_forced(new_volume);

	fibril_rwlock_read_lock(&new_volume->states_lock);
	hr_vol_state_t state = new_volume->state;
	fibril_rwlock_read_unlock(&new_volume->states_lock);
	if (state == HR_VOL_FAULTY || state == HR_VOL_NONE) {
		HR_NOTE("\"%s\": unusable state, not creating\n",
		    new_volume->devname);
		return EINVAL;
	}

	return EOK;
}

/*
 * Called only once in volume's lifetime.
 */
errno_t hr_raid1_init(hr_volume_t *vol)
{
	HR_DEBUG("%s()", __func__);

	if (vol->level != HR_LVL_1)
		return EINVAL;

	vol->data_offset = vol->meta_ops->get_data_offset();
	vol->data_blkno = vol->truncated_blkno - vol->meta_ops->get_size();
	vol->strip_size = 0;

	return EOK;
}

errno_t hr_raid1_add_hotspare(hr_volume_t *vol, service_id_t hotspare)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc = hr_util_add_hotspare(vol, hotspare);

	hr_raid1_vol_state_eval(vol);

	return rc;
}

void hr_raid1_vol_state_eval(hr_volume_t *vol)
{
	HR_DEBUG("%s()", __func__);

	bool exp = true;
	if (!atomic_compare_exchange_strong(&vol->state_dirty, &exp, false))
		return;

	vol->meta_ops->inc_counter(vol);
	vol->meta_ops->save(vol, WITH_STATE_CALLBACK);

	hr_raid1_vol_state_eval_forced(vol);
}

void hr_raid1_ext_state_cb(hr_volume_t *vol, size_t extent, errno_t rc)
{
	HR_DEBUG("%s()", __func__);

	assert(fibril_rwlock_is_locked(&vol->extents_lock));

	if (rc == EOK)
		return;

	fibril_rwlock_write_lock(&vol->states_lock);

	switch (rc) {
	case ENOENT:
		hr_update_ext_state(vol, extent, HR_EXT_MISSING);
		break;
	default:
		hr_update_ext_state(vol, extent, HR_EXT_FAILED);
	}

	hr_mark_vol_state_dirty(vol);

	fibril_rwlock_write_unlock(&vol->states_lock);
}

static void hr_raid1_vol_state_eval_forced(hr_volume_t *vol)
{
	HR_DEBUG("%s()", __func__);

	fibril_rwlock_read_lock(&vol->extents_lock);
	fibril_rwlock_read_lock(&vol->states_lock);

	hr_vol_state_t old_state = vol->state;
	size_t healthy = hr_count_extents(vol, HR_EXT_ONLINE);

	size_t invalid_no = hr_count_extents(vol, HR_EXT_INVALID);

	size_t rebuild_no = hr_count_extents(vol, HR_EXT_REBUILD);

	fibril_mutex_lock(&vol->hotspare_lock);
	size_t hs_no = vol->hotspare_no;
	fibril_mutex_unlock(&vol->hotspare_lock);

	fibril_rwlock_read_unlock(&vol->states_lock);
	fibril_rwlock_read_unlock(&vol->extents_lock);

	if (healthy == 0) {
		if (old_state != HR_VOL_FAULTY) {
			fibril_rwlock_write_lock(&vol->states_lock);
			hr_update_vol_state(vol, HR_VOL_FAULTY);
			fibril_rwlock_write_unlock(&vol->states_lock);
		}
	} else if (healthy < vol->extent_no) {
		if (old_state != HR_VOL_REBUILD &&
		    old_state != HR_VOL_DEGRADED) {
			fibril_rwlock_write_lock(&vol->states_lock);
			hr_update_vol_state(vol, HR_VOL_DEGRADED);
			fibril_rwlock_write_unlock(&vol->states_lock);
		}

		if (old_state != HR_VOL_REBUILD) {
			if (hs_no > 0 || invalid_no > 0 || rebuild_no > 0) {
				fid_t fib = fibril_create(hr_raid1_rebuild,
				    vol);
				if (fib == 0)
					return;
				fibril_start(fib);
				fibril_detach(fib);
			}
		}
	} else {
		if (old_state != HR_VOL_OPTIMAL) {
			fibril_rwlock_write_lock(&vol->states_lock);
			hr_update_vol_state(vol, HR_VOL_OPTIMAL);
			fibril_rwlock_write_unlock(&vol->states_lock);
		}
	}
}

static errno_t hr_raid1_bd_open(bd_srvs_t *bds, bd_srv_t *bd)
{
	HR_DEBUG("%s()", __func__);

	hr_volume_t *vol = bd->srvs->sarg;

	atomic_fetch_add_explicit(&vol->open_cnt, 1, memory_order_relaxed);

	return EOK;
}

static errno_t hr_raid1_bd_close(bd_srv_t *bd)
{
	HR_DEBUG("%s()", __func__);

	hr_volume_t *vol = bd->srvs->sarg;

	atomic_fetch_sub_explicit(&vol->open_cnt, 1, memory_order_relaxed);

	return EOK;
}

static errno_t hr_raid1_bd_sync_cache(bd_srv_t *bd, aoff64_t ba, size_t cnt)
{
	hr_volume_t *vol = bd->srvs->sarg;

	return hr_sync_extents(vol);
}

static errno_t hr_raid1_bd_read_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    void *buf, size_t size)
{
	hr_volume_t *vol = bd->srvs->sarg;

	return hr_raid1_bd_op(HR_BD_READ, vol, ba, cnt, buf, NULL, size);
}

static errno_t hr_raid1_bd_write_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    const void *data, size_t size)
{
	hr_volume_t *vol = bd->srvs->sarg;

	return hr_raid1_bd_op(HR_BD_WRITE, vol, ba, cnt, NULL, data, size);
}

static errno_t hr_raid1_bd_get_block_size(bd_srv_t *bd, size_t *rsize)
{
	hr_volume_t *vol = bd->srvs->sarg;

	*rsize = vol->bsize;
	return EOK;
}

static errno_t hr_raid1_bd_get_num_blocks(bd_srv_t *bd, aoff64_t *rnb)
{
	hr_volume_t *vol = bd->srvs->sarg;

	*rnb = vol->data_blkno;
	return EOK;
}

static size_t hr_raid1_count_good_extents(hr_volume_t *vol, uint64_t ba,
    size_t cnt, uint64_t rebuild_blk)
{
	assert(fibril_rwlock_is_locked(&vol->extents_lock));
	assert(fibril_rwlock_is_locked(&vol->states_lock));

	size_t count = 0;
	for (size_t i = 0; i < vol->extent_no; i++) {
		if (vol->extents[i].state == HR_EXT_ONLINE ||
		    (vol->extents[i].state == HR_EXT_REBUILD &&
		    rebuild_blk >= ba)) {
			count++;
		}
	}

	return count;
}

static errno_t hr_raid1_bd_op(hr_bd_op_type_t type, hr_volume_t *vol,
    aoff64_t ba, size_t cnt, void *data_read, const void *data_write,
    size_t size)
{
	HR_DEBUG("%s()", __func__);

	hr_range_lock_t *rl = NULL;
	errno_t rc;
	size_t i;
	uint64_t rebuild_blk;

	if (size < cnt * vol->bsize)
		return EINVAL;

	fibril_rwlock_read_lock(&vol->states_lock);
	hr_vol_state_t vol_state = vol->state;
	fibril_rwlock_read_unlock(&vol->states_lock);

	if (vol_state == HR_VOL_FAULTY || vol_state == HR_VOL_NONE)
		return EIO;

	/* increment metadata counter only on first write */
	bool exp = false;
	if (type == HR_BD_WRITE &&
	    atomic_compare_exchange_strong(&vol->first_write, &exp, true)) {
		vol->meta_ops->inc_counter(vol);
		vol->meta_ops->save(vol, WITH_STATE_CALLBACK);
	}

	rc = hr_check_ba_range(vol, cnt, ba);
	if (rc != EOK)
		return rc;

	hr_add_data_offset(vol, &ba);

	/*
	 * extent order has to be locked for the whole IO duration,
	 * so that workers have consistent targets
	 */
	fibril_rwlock_read_lock(&vol->extents_lock);

	size_t successful = 0;
	switch (type) {
	case HR_BD_READ:
		rebuild_blk = atomic_load_explicit(&vol->rebuild_blk,
		    memory_order_relaxed);

		for (i = 0; i < vol->extent_no; i++) {
			fibril_rwlock_read_lock(&vol->states_lock);
			hr_ext_state_t state = vol->extents[i].state;
			fibril_rwlock_read_unlock(&vol->states_lock);

			if (state != HR_EXT_ONLINE &&
			    (state != HR_EXT_REBUILD ||
			    ba + cnt - 1 >= rebuild_blk)) {
				continue;
			}

			rc = hr_read_direct(vol->extents[i].svc_id, ba, cnt,
			    data_read);
			if (rc != EOK) {
				hr_raid1_ext_state_cb(vol, i, rc);
			} else {
				successful++;
				break;
			}
		}
		break;
	case HR_BD_WRITE:
		rl = hr_range_lock_acquire(vol, ba, cnt);

		fibril_rwlock_read_lock(&vol->states_lock);

		rebuild_blk = atomic_load_explicit(&vol->rebuild_blk,
		    memory_order_relaxed);

		size_t good = hr_raid1_count_good_extents(vol, ba, cnt,
		    rebuild_blk);

		hr_fgroup_t *group = hr_fgroup_create(vol->fge, good);

		for (i = 0; i < vol->extent_no; i++) {
			if (vol->extents[i].state != HR_EXT_ONLINE &&
			    (vol->extents[i].state != HR_EXT_REBUILD ||
			    ba > rebuild_blk)) {
				/*
				 * When the extent is being rebuilt,
				 * we only write to the part that is already
				 * rebuilt. If IO starts after vol->rebuild_blk
				 * we do not proceed, the write is going to
				 * be replicated later in the rebuild.
				 */
				continue;
			}

			hr_io_t *io = hr_fgroup_alloc(group);
			io->extent = i;
			io->data_write = data_write;
			io->data_read = data_read;
			io->ba = ba;
			io->cnt = cnt;
			io->type = type;
			io->vol = vol;

			hr_fgroup_submit(group, hr_io_worker, io);
		}

		fibril_rwlock_read_unlock(&vol->states_lock);

		(void)hr_fgroup_wait(group, &successful, NULL);

		hr_range_lock_release(rl);

		break;
	default:
		assert(0);
	}

	if (successful > 0)
		rc = EOK;
	else
		rc = EIO;

	fibril_rwlock_read_unlock(&vol->extents_lock);

	hr_raid1_vol_state_eval(vol);

	return rc;
}

static errno_t hr_raid1_rebuild(void *arg)
{
	HR_DEBUG("%s()", __func__);

	hr_volume_t *vol = arg;
	void *buf = NULL;
	size_t rebuild_idx;
	hr_extent_t *rebuild_ext = NULL;
	errno_t rc;

	rc = hr_init_rebuild(vol, &rebuild_idx);
	if (rc != EOK)
		return rc;

	rebuild_ext = &vol->extents[rebuild_idx];

	size_t left = vol->data_blkno - vol->rebuild_blk;
	size_t max_blks = DATA_XFER_LIMIT / vol->bsize;
	buf = hr_malloc_waitok(max_blks * vol->bsize);

	size_t cnt;
	uint64_t ba = vol->rebuild_blk;
	hr_add_data_offset(vol, &ba);

	/*
	 * this is not necessary because a rebuild is
	 * protected by itself, i.e. there can be only
	 * one REBUILD at a time
	 */
	fibril_rwlock_read_lock(&vol->extents_lock);

	/* increment metadata counter only on first write */
	bool exp = false;
	if (atomic_compare_exchange_strong(&vol->first_write, &exp, true)) {
		vol->meta_ops->inc_counter(vol);
		vol->meta_ops->save(vol, WITH_STATE_CALLBACK);
	}

	hr_range_lock_t *rl = NULL;

	HR_NOTE("\"%s\": REBUILD started on extent no. %zu at block %lu.\n",
	    vol->devname, rebuild_idx, ba);

	uint64_t written = 0;
	unsigned int percent, old_percent = 100;
	while (left != 0) {
		cnt = min(max_blks, left);

		rl = hr_range_lock_acquire(vol, ba, cnt);

		atomic_store_explicit(&vol->rebuild_blk, ba,
		    memory_order_relaxed);

		rc = hr_raid1_bd_op(HR_BD_READ, vol, ba, cnt, buf, NULL,
		    cnt * vol->bsize);
		if (rc != EOK) {
			hr_range_lock_release(rl);
			goto end;
		}

		rc = hr_write_direct(rebuild_ext->svc_id, ba, cnt, buf);
		if (rc != EOK) {
			hr_raid1_ext_state_cb(vol, rebuild_idx, rc);
			hr_range_lock_release(rl);
			goto end;
		}

		percent = ((ba + cnt) * 100) / vol->data_blkno;
		if (percent != old_percent) {
			if (percent % 5 == 0)
				HR_DEBUG("\"%s\" REBUILD progress: %u%%\n",
				    vol->devname, percent);
		}

		if (written * vol->bsize > HR_REBUILD_SAVE_BYTES) {
			vol->meta_ops->save(vol, WITH_STATE_CALLBACK);
			written = 0;
		}

		hr_range_lock_release(rl);

		written += cnt;
		ba += cnt;
		left -= cnt;
		old_percent = percent;
	}

	HR_DEBUG("hr_raid1_rebuild(): rebuild finished on \"%s\" (%" PRIun "), "
	    "extent no. %zu\n", vol->devname, vol->svc_id, rebuild_idx);

	fibril_rwlock_write_lock(&vol->states_lock);

	hr_update_ext_state(vol, rebuild_idx, HR_EXT_ONLINE);

	atomic_store_explicit(&vol->rebuild_blk, 0, memory_order_relaxed);

	hr_mark_vol_state_dirty(vol);

	fibril_rwlock_write_unlock(&vol->states_lock);
end:
	fibril_rwlock_read_unlock(&vol->extents_lock);

	hr_raid1_vol_state_eval(vol);

	free(buf);

	return rc;
}

/** @}
 */
