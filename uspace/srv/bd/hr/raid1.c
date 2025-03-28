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

#include <bd_srv.h>
#include <block.h>
#include <errno.h>
#include <hr.h>
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

extern loc_srv_t *hr_srv;

static void hr_raid1_update_vol_status(hr_volume_t *);
static void hr_raid1_ext_state_callback(hr_volume_t *, size_t, errno_t);
static size_t hr_raid1_count_good_extents(hr_volume_t *, uint64_t, size_t,
    uint64_t);
static errno_t hr_raid1_bd_op(hr_bd_op_type_t, bd_srv_t *, aoff64_t, size_t,
    void *, const void *, size_t);
static errno_t hr_raid1_rebuild(void *);
static errno_t init_rebuild(hr_volume_t *, size_t *);
static errno_t swap_hs(hr_volume_t *, size_t, size_t);
static errno_t hr_raid1_restore_blocks(hr_volume_t *, size_t, uint64_t, size_t,
    void *);

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

errno_t hr_raid1_create(hr_volume_t *new_volume)
{
	errno_t rc;

	assert(new_volume->level == HR_LVL_1);

	if (new_volume->extent_no < 2) {
		HR_ERROR("RAID 1 array needs at least 2 devices\n");
		return EINVAL;
	}

	bd_srvs_init(&new_volume->hr_bds);
	new_volume->hr_bds.ops = &hr_raid1_bd_ops;
	new_volume->hr_bds.sarg = new_volume;

	/* force volume state update */
	hr_mark_vol_state_dirty(new_volume);
	hr_raid1_update_vol_status(new_volume);

	fibril_rwlock_read_lock(&new_volume->states_lock);
	hr_vol_status_t state = new_volume->status;
	fibril_rwlock_read_unlock(&new_volume->states_lock);
	if (state == HR_VOL_FAULTY || state == HR_VOL_NONE)
		return EINVAL;

	rc = hr_register_volume(new_volume);

	return rc;
}

errno_t hr_raid1_init(hr_volume_t *vol)
{
	errno_t rc;
	size_t bsize;
	uint64_t total_blkno;

	assert(vol->level == HR_LVL_1);

	rc = hr_check_devs(vol, &total_blkno, &bsize);
	if (rc != EOK)
		return rc;

	vol->nblocks = total_blkno / vol->extent_no;
	vol->bsize = bsize;
	vol->data_offset = HR_DATA_OFF;
	vol->data_blkno = vol->nblocks - vol->data_offset;
	vol->strip_size = 0;

	return EOK;
}

void hr_raid1_status_event(hr_volume_t *vol)
{
	hr_raid1_update_vol_status(vol);
}

errno_t hr_raid1_add_hotspare(hr_volume_t *vol, service_id_t hotspare)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc = EOK;

	fibril_mutex_lock(&vol->hotspare_lock);

	if (vol->hotspare_no >= HR_MAX_HOTSPARES) {
		HR_ERROR("hr_raid1_add_hotspare(): cannot add more hotspares "
		    "to \"%s\"\n", vol->devname);
		rc = ELIMIT;
		goto error;
	}

	size_t hs_idx = vol->hotspare_no;

	vol->hotspare_no++;

	hr_update_hotspare_svc_id(vol, hs_idx, hotspare);
	hr_update_hotspare_status(vol, hs_idx, HR_EXT_HOTSPARE);

	hr_mark_vol_state_dirty(vol);
error:
	fibril_mutex_unlock(&vol->hotspare_lock);

	hr_raid1_update_vol_status(vol);

	return rc;
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
	return hr_raid1_bd_op(HR_BD_SYNC, bd, ba, cnt, NULL, NULL, 0);
}

static errno_t hr_raid1_bd_read_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    void *buf, size_t size)
{
	return hr_raid1_bd_op(HR_BD_READ, bd, ba, cnt, buf, NULL, size);
}

static errno_t hr_raid1_bd_write_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    const void *data, size_t size)
{
	return hr_raid1_bd_op(HR_BD_WRITE, bd, ba, cnt, NULL, data, size);
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

static void hr_raid1_update_vol_status(hr_volume_t *vol)
{
	bool exp = true;

	/* TODO: could also wrap this */
	if (!atomic_compare_exchange_strong(&vol->state_dirty, &exp, false))
		return;

	fibril_rwlock_read_lock(&vol->extents_lock);
	fibril_rwlock_read_lock(&vol->states_lock);

	hr_vol_status_t old_state = vol->status;
	size_t healthy = hr_count_extents(vol, HR_EXT_ONLINE);

	fibril_rwlock_read_unlock(&vol->states_lock);
	fibril_rwlock_read_unlock(&vol->extents_lock);

	if (healthy == 0) {
		if (old_state != HR_VOL_FAULTY) {
			fibril_rwlock_write_lock(&vol->states_lock);
			hr_update_vol_status(vol, HR_VOL_FAULTY);
			fibril_rwlock_write_unlock(&vol->states_lock);
		}
	} else if (healthy < vol->extent_no) {
		if (old_state != HR_VOL_REBUILD &&
		    old_state != HR_VOL_DEGRADED) {
			fibril_rwlock_write_lock(&vol->states_lock);
			hr_update_vol_status(vol, HR_VOL_DEGRADED);
			fibril_rwlock_write_unlock(&vol->states_lock);
		}

		if (old_state != HR_VOL_REBUILD) {
			if (vol->hotspare_no > 0) {
				fid_t fib = fibril_create(hr_raid1_rebuild,
				    vol);
				if (fib == 0)
					return;
				fibril_start(fib);
				fibril_detach(fib);
			}
		}
	} else {
		if (old_state != HR_VOL_ONLINE) {
			fibril_rwlock_write_lock(&vol->states_lock);
			hr_update_vol_status(vol, HR_VOL_ONLINE);
			fibril_rwlock_write_unlock(&vol->states_lock);
		}
	}
}

static void hr_raid1_ext_state_callback(hr_volume_t *vol, size_t extent,
    errno_t rc)
{
	if (rc == EOK)
		return;

	assert(fibril_rwlock_is_locked(&vol->extents_lock));

	fibril_rwlock_write_lock(&vol->states_lock);

	switch (rc) {
	case ENOMEM:
		hr_update_ext_status(vol, extent, HR_EXT_INVALID);
		break;
	case ENOENT:
		hr_update_ext_status(vol, extent, HR_EXT_MISSING);
		break;
	default:
		hr_update_ext_status(vol, extent, HR_EXT_FAILED);
	}

	hr_mark_vol_state_dirty(vol);

	fibril_rwlock_write_unlock(&vol->states_lock);
}

static size_t hr_raid1_count_good_extents(hr_volume_t *vol, uint64_t ba,
    size_t cnt, uint64_t rebuild_blk)
{
	assert(fibril_rwlock_is_locked(&vol->extents_lock));
	assert(fibril_rwlock_is_locked(&vol->states_lock));

	size_t count = 0;
	for (size_t i = 0; i < vol->extent_no; i++) {
		if (vol->extents[i].status == HR_EXT_ONLINE ||
		    (vol->extents[i].status == HR_EXT_REBUILD &&
		    ba < rebuild_blk)) {
			count++;
		}
	}

	return count;

}

static errno_t hr_raid1_bd_op(hr_bd_op_type_t type, bd_srv_t *bd, aoff64_t ba,
    size_t cnt, void *data_read, const void *data_write, size_t size)
{
	hr_volume_t *vol = bd->srvs->sarg;
	hr_range_lock_t *rl = NULL;
	errno_t rc;
	size_t i;
	uint64_t rebuild_blk;

	fibril_rwlock_read_lock(&vol->states_lock);
	hr_vol_status_t vol_state = vol->status;
	fibril_rwlock_read_unlock(&vol->states_lock);

	if (vol_state == HR_VOL_FAULTY || vol_state == HR_VOL_NONE)
		return EIO;

	if (type == HR_BD_READ || type == HR_BD_WRITE)
		if (size < cnt * vol->bsize)
			return EINVAL;

	rc = hr_check_ba_range(vol, cnt, ba);
	if (rc != EOK)
		return rc;

	/* allow full dev sync */
	if (type != HR_BD_SYNC || ba != 0)
		hr_add_ba_offset(vol, &ba);

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
			hr_ext_status_t state = vol->extents[i].status;
			fibril_rwlock_read_unlock(&vol->states_lock);

			if (state != HR_EXT_ONLINE &&
			    (state != HR_EXT_REBUILD ||
			    ba + cnt - 1 >= rebuild_blk)) {
				continue;
			}

			rc = block_read_direct(vol->extents[i].svc_id, ba, cnt,
			    data_read);

			if (rc == ENOMEM && i + 1 == vol->extent_no)
				goto end;

			if (rc == ENOMEM)
				continue;

			if (rc != EOK) {
				hr_raid1_ext_state_callback(vol, i, rc);
			} else {
				successful++;
				break;
			}
		}
		break;
	case HR_BD_SYNC:
	case HR_BD_WRITE:
		if (type == HR_BD_WRITE) {
			rl = hr_range_lock_acquire(vol, ba, cnt);
			if (rl == NULL) {
				rc = ENOMEM;
				goto end;
			}
		}

		fibril_rwlock_read_lock(&vol->states_lock);

		rebuild_blk = atomic_load_explicit(&vol->rebuild_blk,
		    memory_order_relaxed);

		size_t good = hr_raid1_count_good_extents(vol, ba, cnt,
		    rebuild_blk);

		hr_fgroup_t *group = hr_fgroup_create(vol->fge, good);
		if (group == NULL) {
			if (type == HR_BD_WRITE)
				hr_range_lock_release(rl);
			rc = ENOMEM;
			fibril_rwlock_read_unlock(&vol->states_lock);
			goto end;
		}

		for (i = 0; i < vol->extent_no; i++) {
			if (vol->extents[i].status != HR_EXT_ONLINE &&
			    (vol->extents[i].status != HR_EXT_REBUILD ||
			    ba >= rebuild_blk)) {
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
			io->state_callback = hr_raid1_ext_state_callback;

			hr_fgroup_submit(group, hr_io_worker, io);
		}

		fibril_rwlock_read_unlock(&vol->states_lock);

		(void)hr_fgroup_wait(group, &successful, NULL);

		if (type == HR_BD_WRITE)
			hr_range_lock_release(rl);

		break;
	default:
		rc = EINVAL;
		goto end;
	}

	if (successful > 0)
		rc = EOK;
	else
		rc = EIO;

end:
	fibril_rwlock_read_unlock(&vol->extents_lock);

	hr_raid1_update_vol_status(vol);

	return rc;
}

/*
 * Put the last HOTSPARE extent in place
 * of first that != ONLINE, and start the rebuild.
 */
static errno_t hr_raid1_rebuild(void *arg)
{
	HR_DEBUG("%s()", __func__);

	hr_volume_t *vol = arg;
	void *buf = NULL;
	size_t rebuild_idx;
	errno_t rc;

	rc = init_rebuild(vol, &rebuild_idx);
	if (rc != EOK)
		return rc;

	size_t left = vol->data_blkno;
	size_t max_blks = DATA_XFER_LIMIT / vol->bsize;
	buf = malloc(max_blks * vol->bsize);

	size_t cnt;
	uint64_t ba = 0;
	hr_add_ba_offset(vol, &ba);

	/*
	 * XXX: this is useless here after simplified DI, because
	 * rebuild cannot be triggered while ongoing rebuild
	 */
	fibril_rwlock_read_lock(&vol->extents_lock);

	hr_range_lock_t *rl = NULL;

	unsigned int percent, old_percent = 100;
	while (left != 0) {
		cnt = min(max_blks, left);

		rl = hr_range_lock_acquire(vol, ba, cnt);
		if (rl == NULL) {
			rc = ENOMEM;
			goto end;
		}

		atomic_store_explicit(&vol->rebuild_blk, ba,
		    memory_order_relaxed);

		rc = hr_raid1_restore_blocks(vol, rebuild_idx, ba, cnt, buf);

		percent = ((ba + cnt) * 100) / vol->data_blkno;
		if (percent != old_percent) {
			if (percent % 5 == 0)
				HR_DEBUG("\"%s\" REBUILD progress: %u%%\n",
				    vol->devname, percent);
		}

		hr_range_lock_release(rl);

		if (rc != EOK)
			goto end;

		ba += cnt;
		left -= cnt;
		old_percent = percent;
	}

	HR_DEBUG("hr_raid1_rebuild(): rebuild finished on \"%s\" (%lu), "
	    "extent no. %lu\n", vol->devname, vol->svc_id, rebuild_idx);

	fibril_rwlock_write_lock(&vol->states_lock);

	hr_update_ext_status(vol, rebuild_idx, HR_EXT_ONLINE);

	/*
	 * We can be optimistic here, if some extents are
	 * still INVALID, FAULTY or MISSING, the update vol
	 * function will pick them up, and set the volume
	 * state accordingly.
	 */
	hr_update_vol_status(vol, HR_VOL_ONLINE);
	hr_mark_vol_state_dirty(vol);

	fibril_rwlock_write_unlock(&vol->states_lock);

	/*
	 * For now write metadata at the end, because
	 * we don't sync metada accross extents yet.
	 */
	hr_write_meta_to_ext(vol, rebuild_idx);
end:
	if (rc != EOK) {
		/*
		 * We can fail either because:
		 * - the rebuild extent failing or invalidation
		 * - there is are no ONLINE extents (vol is FAULTY)
		 * - we got ENOMEM on all READs (we also invalidate the
		 *   rebuild extent here, for now)
		 */
		fibril_rwlock_write_lock(&vol->states_lock);
		hr_update_vol_status(vol, HR_VOL_DEGRADED);
		hr_mark_vol_state_dirty(vol);
		fibril_rwlock_write_unlock(&vol->states_lock);
	}

	fibril_rwlock_read_unlock(&vol->extents_lock);

	hr_raid1_update_vol_status(vol);

	if (buf != NULL)
		free(buf);

	return rc;
}

static errno_t init_rebuild(hr_volume_t *vol, size_t *rebuild_idx)
{
	errno_t rc = EOK;

	fibril_rwlock_write_lock(&vol->extents_lock);
	fibril_rwlock_write_lock(&vol->states_lock);
	fibril_mutex_lock(&vol->hotspare_lock);

	if (vol->hotspare_no == 0) {
		HR_WARN("hr_raid1_rebuild(): no free hotspares on \"%s\", "
		    "aborting rebuild\n", vol->devname);
		rc = EINVAL;
		goto error;
	}

	size_t bad = vol->extent_no;
	for (size_t i = 0; i < vol->extent_no; i++) {
		if (vol->extents[i].status != HR_EXT_ONLINE) {
			bad = i;
			break;
		}
	}

	if (bad == vol->extent_no) {
		HR_WARN("hr_raid1_rebuild(): no bad extent on \"%s\", "
		    "aborting rebuild\n", vol->devname);
		rc = EINVAL;
		goto error;
	}

	size_t hotspare_idx = vol->hotspare_no - 1;

	hr_ext_status_t hs_state = vol->hotspares[hotspare_idx].status;
	if (hs_state != HR_EXT_HOTSPARE) {
		HR_ERROR("hr_raid1_rebuild(): invalid hotspare state \"%s\", "
		    "aborting rebuild\n", hr_get_ext_status_msg(hs_state));
		rc = EINVAL;
		goto error;
	}

	rc = swap_hs(vol, bad, hotspare_idx);
	if (rc != EOK) {
		HR_ERROR("hr_raid1_rebuild(): swapping hotspare failed, "
		    "aborting rebuild\n");
		goto error;
	}

	hr_extent_t *rebuild_ext = &vol->extents[bad];

	HR_DEBUG("hr_raid1_rebuild(): starting REBUILD on extent no. %lu (%lu)"
	    "\n", bad, rebuild_ext->svc_id);

	atomic_store_explicit(&vol->rebuild_blk, 0, memory_order_relaxed);

	hr_update_ext_status(vol, bad, HR_EXT_REBUILD);
	hr_update_vol_status(vol, HR_VOL_REBUILD);

	*rebuild_idx = bad;
error:
	fibril_mutex_unlock(&vol->hotspare_lock);
	fibril_rwlock_write_unlock(&vol->states_lock);
	fibril_rwlock_write_unlock(&vol->extents_lock);

	return rc;
}

static errno_t swap_hs(hr_volume_t *vol, size_t bad, size_t hs)
{
	HR_DEBUG("hr_raid1_rebuild(): swapping in hotspare\n");

	service_id_t faulty_svc_id = vol->extents[bad].svc_id;
	service_id_t hs_svc_id = vol->hotspares[hs].svc_id;

	/* TODO: if rc != EOK, try next hotspare */
	errno_t rc = block_init(hs_svc_id);
	if (rc != EOK) {
		HR_ERROR("hr_raid1_rebuild(): initing hotspare (%lu) failed\n",
		    hs_svc_id);
		return rc;
	}

	hr_update_ext_svc_id(vol, bad, hs_svc_id);
	hr_update_ext_status(vol, bad, HR_EXT_HOTSPARE);

	hr_update_hotspare_svc_id(vol, hs, 0);
	hr_update_hotspare_status(vol, hs, HR_EXT_MISSING);

	vol->hotspare_no--;

	if (faulty_svc_id != 0)
		block_fini(faulty_svc_id);

	return EOK;
}

static errno_t hr_raid1_restore_blocks(hr_volume_t *vol, size_t rebuild_idx,
    uint64_t ba, size_t cnt, void *buf)
{
	assert(fibril_rwlock_is_locked(&vol->extents_lock));

	errno_t rc = ENOENT;
	hr_extent_t *ext, *rebuild_ext = &vol->extents[rebuild_idx];

	fibril_rwlock_read_lock(&vol->states_lock);
	hr_ext_status_t rebuild_ext_status = rebuild_ext->status;
	fibril_rwlock_read_unlock(&vol->states_lock);

	if (rebuild_ext_status != HR_EXT_REBUILD)
		return EINVAL;

	for (size_t i = 0; i < vol->extent_no; i++) {
		fibril_rwlock_read_lock(&vol->states_lock);
		ext = &vol->extents[i];
		if (ext->status != HR_EXT_ONLINE) {
			fibril_rwlock_read_unlock(&vol->states_lock);
			continue;
		}
		fibril_rwlock_read_unlock(&vol->states_lock);

		rc = block_read_direct(ext->svc_id, ba, cnt, buf);
		if (rc == EOK)
			break;

		if (rc != ENOMEM)
			hr_raid1_ext_state_callback(vol, i, rc);

		if (i + 1 >= vol->extent_no) {
			if (rc != ENOMEM) {
				HR_ERROR("rebuild on \"%s\" (%lu), failed due "
				    "to too many failed extents\n",
				    vol->devname, vol->svc_id);
			}

			/* for now we have to invalidate the rebuild extent */
			if (rc == ENOMEM) {
				HR_ERROR("rebuild on \"%s\" (%lu), failed due "
				    "to too many failed reads, because of not "
				    "enough memory\n",
				    vol->devname, vol->svc_id);
				hr_raid1_ext_state_callback(vol, rebuild_idx,
				    ENOMEM);
			}

			return rc;
		}
	}

	rc = block_write_direct(rebuild_ext->svc_id, ba, cnt, buf);
	if (rc != EOK) {
		/*
		 * Here we dont handle ENOMEM, because maybe in the
		 * future, there is going to be M_WAITOK, or we are
		 * going to wait for more memory, so that we don't
		 * have to invalidate it...
		 *
		 * XXX: for now we do
		 */
		hr_raid1_ext_state_callback(vol, rebuild_idx, rc);

		HR_ERROR("rebuild on \"%s\" (%lu), failed due to "
		    "the rebuilt extent no. %lu WRITE (rc: %s)\n",
		    vol->devname, vol->svc_id, rebuild_idx, str_error(rc));

		return rc;
	}

	return EOK;
}

/** @}
 */
