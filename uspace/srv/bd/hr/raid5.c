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
#include <mem.h>
#include <task.h>
#include <stdio.h>
#include <stdlib.h>
#include <str_error.h>

#include "io.h"
#include "parity_stripe.h"
#include "superblock.h"
#include "util.h"
#include "var.h"

static void hr_raid5_vol_state_eval_forced(hr_volume_t *);

static size_t hr_raid5_parity_extent(hr_level_t, hr_layout_t, size_t,
    uint64_t);
static size_t hr_raid5_data_extent(hr_level_t, hr_layout_t, size_t, uint64_t,
    uint64_t);

static errno_t hr_raid5_rebuild(void *);

/* bdops */
static errno_t hr_raid5_bd_open(bd_srvs_t *, bd_srv_t *);
static errno_t hr_raid5_bd_close(bd_srv_t *);
static errno_t hr_raid5_bd_read_blocks(bd_srv_t *, aoff64_t, size_t, void *,
    size_t);
static errno_t hr_raid5_bd_sync_cache(bd_srv_t *, aoff64_t, size_t);
static errno_t hr_raid5_bd_write_blocks(bd_srv_t *, aoff64_t, size_t,
    const void *, size_t);
static errno_t hr_raid5_bd_get_block_size(bd_srv_t *, size_t *);
static errno_t hr_raid5_bd_get_num_blocks(bd_srv_t *, aoff64_t *);

static bd_ops_t hr_raid5_bd_ops = {
	.open = hr_raid5_bd_open,
	.close = hr_raid5_bd_close,
	.sync_cache = hr_raid5_bd_sync_cache,
	.read_blocks = hr_raid5_bd_read_blocks,
	.write_blocks = hr_raid5_bd_write_blocks,
	.get_block_size = hr_raid5_bd_get_block_size,
	.get_num_blocks = hr_raid5_bd_get_num_blocks
};

extern loc_srv_t *hr_srv;

errno_t hr_raid5_create(hr_volume_t *new_volume)
{
	HR_DEBUG("%s()", __func__);

	if (new_volume->level != HR_LVL_5 && new_volume->level != HR_LVL_4)
		return EINVAL;

	if (new_volume->extent_no < 3) {
		HR_ERROR("RAID 5 volume needs at least 3 devices\n");
		return EINVAL;
	}

	bd_srvs_init(&new_volume->hr_bds);
	new_volume->hr_bds.ops = &hr_raid5_bd_ops;
	new_volume->hr_bds.sarg = new_volume;

	hr_raid5_vol_state_eval_forced(new_volume);

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
errno_t hr_raid5_init(hr_volume_t *vol)
{
	HR_DEBUG("%s()", __func__);

	if (vol->level != HR_LVL_5 && vol->level != HR_LVL_4)
		return EINVAL;

	vol->data_offset = vol->meta_ops->get_data_offset();

	uint64_t single_sz = vol->truncated_blkno - vol->meta_ops->get_size();
	vol->data_blkno = single_sz * (vol->extent_no - 1);

	vol->strip_size = HR_STRIP_SIZE;

	if (vol->level == HR_LVL_4)
		vol->layout = HR_LAYOUT_RAID4_N;
	else
		vol->layout = HR_LAYOUT_RAID5_NR;

	return EOK;
}

void hr_raid5_vol_state_eval(hr_volume_t *vol)
{
	HR_DEBUG("%s()", __func__);

	bool exp = true;
	if (!atomic_compare_exchange_strong(&vol->state_dirty, &exp, false))
		return;

	vol->meta_ops->inc_counter(vol);
	(void)vol->meta_ops->save(vol, WITH_STATE_CALLBACK);

	hr_raid5_vol_state_eval_forced(vol);
}

errno_t hr_raid5_add_hotspare(hr_volume_t *vol, service_id_t hotspare)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc = hr_util_add_hotspare(vol, hotspare);

	hr_raid5_vol_state_eval(vol);

	return rc;
}

void hr_raid5_ext_state_cb(hr_volume_t *vol, size_t extent, errno_t rc)
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

static errno_t hr_raid5_bd_open(bd_srvs_t *bds, bd_srv_t *bd)
{
	HR_DEBUG("%s()\n", __func__);

	hr_volume_t *vol = bd->srvs->sarg;

	atomic_fetch_add_explicit(&vol->open_cnt, 1, memory_order_relaxed);

	return EOK;
}

static errno_t hr_raid5_bd_close(bd_srv_t *bd)
{
	HR_DEBUG("%s()\n", __func__);

	hr_volume_t *vol = bd->srvs->sarg;

	atomic_fetch_sub_explicit(&vol->open_cnt, 1, memory_order_relaxed);

	return EOK;
}

static errno_t hr_raid5_bd_sync_cache(bd_srv_t *bd, aoff64_t ba, size_t cnt)
{
	hr_volume_t *vol = bd->srvs->sarg;

	return hr_sync_extents(vol);
}

static errno_t hr_raid5_bd_read_blocks(bd_srv_t *bd, uint64_t ba, size_t cnt,
    void *data_read, size_t size)
{
	hr_volume_t *vol = bd->srvs->sarg;
	errno_t rc;

	if (size < cnt * vol->bsize)
		return EINVAL;

	fibril_rwlock_read_lock(&vol->states_lock);
	hr_vol_state_t vol_state = vol->state;
	fibril_rwlock_read_unlock(&vol->states_lock);

	if (vol_state == HR_VOL_FAULTY || vol_state == HR_VOL_NONE)
		return EIO;

	rc = hr_check_ba_range(vol, cnt, ba);
	if (rc != EOK)
		return rc;

	uint64_t strip_size = vol->strip_size / vol->bsize; /* in blocks */
	uint64_t strip_no = ba / strip_size;

	/* calculate number of stripes touched */
	uint64_t last_ba = ba + cnt - 1;
	uint64_t end_strip_no = last_ba / strip_size;
	uint64_t start_stripe = strip_no / (vol->extent_no - 1);
	uint64_t end_stripe = end_strip_no / (vol->extent_no - 1);
	size_t stripes_cnt = end_stripe - start_stripe + 1;

	hr_stripe_t *stripes = hr_create_stripes(vol, stripes_cnt, false);
	if (stripes == NULL)
		return ENOMEM;

	/*
	 * Pre-allocate range locks, because after group creation and
	 * firing off IO requests there is no easy consistent ENOMEM error
	 * path.
	 */
	hr_range_lock_t **rlps = malloc_waitok(stripes_cnt * sizeof(*rlps));
	for (size_t i = 0; i < stripes_cnt; i++)
		rlps[i] = malloc_waitok(sizeof(**rlps));

	/*
	 * extent order has to be locked for the whole IO duration,
	 * so that workers have consistent targets
	 */
	fibril_rwlock_read_lock(&vol->extents_lock);

	for (uint64_t s = start_stripe; s <= end_stripe; s++) {
		uint64_t relative = s - start_stripe;
		hr_range_lock_acquire_noalloc(rlps[relative], vol, s, 1);
	}

	uint64_t phys_block, len;
	size_t left;

	hr_layout_t layout = vol->layout;
	hr_level_t level = vol->level;

	/* parity extent */
	size_t p_extent = hr_raid5_parity_extent(level, layout,
	    vol->extent_no, strip_no);

	uint64_t strip_off = ba % strip_size;

	left = cnt;

	while (left != 0) {
		if (level == HR_LVL_5) {
			p_extent = hr_raid5_parity_extent(level, layout,
			    vol->extent_no, strip_no);
		}

		size_t extent = hr_raid5_data_extent(level, layout,
		    vol->extent_no, strip_no, p_extent);

		uint64_t stripe_no = strip_no / (vol->extent_no - 1);
		size_t relative_si = stripe_no - start_stripe; /* relative stripe index */
		hr_stripe_t *stripe = &stripes[relative_si];
		stripe->p_extent = p_extent;

		stripe->strips_touched++;

		phys_block = stripe_no * strip_size + strip_off;
		cnt = min(left, strip_size - strip_off);
		len = vol->bsize * cnt;
		hr_add_data_offset(vol, &phys_block);

		stripe->extent_span[extent].range.start = phys_block;
		stripe->extent_span[extent].range.end = phys_block + cnt - 1;
		stripe->extent_span[extent].cnt = cnt;
		stripe->extent_span[extent].data_read = data_read;
		stripe->extent_span[extent].strip_off = strip_off;

		data_read += len;
		left -= cnt;
		strip_off = 0;
		strip_no++;
	}

retry:
	size_t bad_extent = vol->extent_no;

	uint64_t rebuild_pos = atomic_load_explicit(&vol->rebuild_blk,
	    memory_order_relaxed);

	fibril_rwlock_read_lock(&vol->states_lock);

	for (size_t e = 0; e < vol->extent_no; e++) {
		hr_ext_state_t s = vol->extents[e].state;
		if ((vol->state == HR_VOL_DEGRADED && s != HR_EXT_ONLINE) ||
		    (s == HR_EXT_REBUILD && rebuild_pos < start_stripe)) {
			bad_extent = e;
			break;
		}
	}

	fibril_rwlock_read_unlock(&vol->states_lock);

	for (size_t s = 0; s < stripes_cnt; s++) {
		if (stripes[s].done)
			continue;
		execute_stripe(&stripes[s], bad_extent);
	}

	for (size_t s = 0; s < stripes_cnt; s++) {
		if (stripes[s].done)
			continue;
		wait_for_stripe(&stripes[s]);
	}

	hr_raid5_vol_state_eval(vol);

	rc = EOK;

	fibril_rwlock_read_lock(&vol->states_lock);

	if (vol->state == HR_VOL_FAULTY) {
		fibril_rwlock_read_unlock(&vol->states_lock);
		rc = EIO;
		goto end;
	}

	fibril_rwlock_read_unlock(&vol->states_lock);

	for (size_t s = 0; s < stripes_cnt; s++)
		if (stripes[s].rc == EAGAIN)
			goto retry;

	/* all stripes are done */
end:
	fibril_rwlock_read_unlock(&vol->extents_lock);

	for (size_t i = 0; i < stripes_cnt; i++)
		hr_range_lock_release(rlps[i]);

	hr_destroy_stripes(stripes, stripes_cnt);

	return rc;
}

static errno_t hr_raid5_bd_write_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    const void *data_write, size_t size)
{
	hr_volume_t *vol = bd->srvs->sarg;
	errno_t rc;

	if (size < cnt * vol->bsize)
		return EINVAL;

	fibril_rwlock_read_lock(&vol->states_lock);
	hr_vol_state_t vol_state = vol->state;
	fibril_rwlock_read_unlock(&vol->states_lock);

	if (vol_state == HR_VOL_FAULTY || vol_state == HR_VOL_NONE)
		return EIO;

	/* increment metadata counter only on first write */
	bool exp = false;
	if (atomic_compare_exchange_strong(&vol->first_write, &exp, true)) {
		vol->meta_ops->inc_counter(vol);
		vol->meta_ops->save(vol, WITH_STATE_CALLBACK);
	}

	rc = hr_check_ba_range(vol, cnt, ba);
	if (rc != EOK)
		return rc;

	uint64_t strip_size = vol->strip_size / vol->bsize; /* in blocks */
	uint64_t strip_no = ba / strip_size;

	/* calculate number of stripes touched */
	uint64_t last_ba = ba + cnt - 1;
	uint64_t end_strip_no = last_ba / strip_size;
	uint64_t start_stripe = strip_no / (vol->extent_no - 1);
	uint64_t end_stripe = end_strip_no / (vol->extent_no - 1);
	size_t stripes_cnt = end_stripe - start_stripe + 1;

	hr_stripe_t *stripes = hr_create_stripes(vol, stripes_cnt, true);
	if (stripes == NULL)
		return ENOMEM;

	uint64_t stripe_size = strip_size * (vol->extent_no - 1);

	for (uint64_t stripe = start_stripe; stripe <= end_stripe; stripe++) {
		uint64_t relative_stripe = stripe - start_stripe;

		uint64_t s_start = stripe * stripe_size;
		uint64_t s_end = s_start + stripe_size - 1;

		uint64_t overlap_start;
		if (ba > s_start)
			overlap_start = ba;
		else
			overlap_start = s_start;

		uint64_t overlap_end;
		if (last_ba < s_end)
			overlap_end = last_ba;
		else
			overlap_end = s_end;

		uint64_t start_strip_index =
		    (overlap_start - s_start) / strip_size;
		uint64_t end_strip_index = (overlap_end - s_start) / strip_size;
		size_t strips_touched = end_strip_index - start_strip_index + 1;

		stripes[relative_stripe].strips_touched = strips_touched;

		uint64_t first_offset = (overlap_start - s_start) % strip_size;
		uint64_t last_offset = (overlap_end - s_start) % strip_size;

		size_t partials = 0;
		if (first_offset != 0)
			partials++;
		if (last_offset != strip_size - 1)
			partials++;
		if (start_strip_index == end_strip_index && partials == 2)
			partials = 1;

		stripes[relative_stripe].strips_touched = strips_touched;
		stripes[relative_stripe].partial_strips_touched = partials;

		if (strips_touched < (vol->extent_no - 1) / 2)
			stripes[relative_stripe].subtract = true;
	}

	/*
	 * Pre-allocate range locks, because after group creation and
	 * firing off IO requests there is no easy consistent ENOMEM error
	 * path.
	 */
	hr_range_lock_t **rlps = malloc_waitok(stripes_cnt * sizeof(*rlps));
	for (size_t i = 0; i < stripes_cnt; i++)
		rlps[i] = malloc_waitok(sizeof(**rlps));

	/*
	 * extent order has to be locked for the whole IO duration,
	 * so that workers have consistent targets
	 */
	fibril_rwlock_read_lock(&vol->extents_lock);

	for (uint64_t s = start_stripe; s <= end_stripe; s++) {
		uint64_t relative = s - start_stripe;
		hr_range_lock_acquire_noalloc(rlps[relative], vol, s, 1);
	}

	uint64_t phys_block, len;
	size_t left;

	hr_layout_t layout = vol->layout;
	hr_level_t level = vol->level;

	/* parity extent */
	size_t p_extent = hr_raid5_parity_extent(level, layout,
	    vol->extent_no, strip_no);

	uint64_t strip_off = ba % strip_size;

	left = cnt;

	while (left != 0) {
		if (level == HR_LVL_5) {
			p_extent = hr_raid5_parity_extent(level, layout,
			    vol->extent_no, strip_no);
		}

		size_t extent = hr_raid5_data_extent(level, layout,
		    vol->extent_no, strip_no, p_extent);

		uint64_t stripe_no = strip_no / (vol->extent_no - 1);
		size_t relative_si = stripe_no - start_stripe; /* relative stripe index */
		hr_stripe_t *stripe = &stripes[relative_si];
		stripe->p_extent = p_extent;

		phys_block = stripe_no * strip_size + strip_off;
		cnt = min(left, strip_size - strip_off);
		len = vol->bsize * cnt;
		hr_add_data_offset(vol, &phys_block);

		stripe->extent_span[extent].range.start = phys_block;
		stripe->extent_span[extent].range.end = phys_block + cnt - 1;
		stripe->extent_span[extent].cnt = cnt;
		stripe->extent_span[extent].data_write = data_write;
		stripe->extent_span[extent].strip_off = strip_off;

		data_write += len;
		left -= cnt;
		strip_off = 0;
		strip_no++;
	}

retry:
	size_t bad_extent = vol->extent_no;

	uint64_t rebuild_pos = atomic_load_explicit(&vol->rebuild_blk,
	    memory_order_relaxed);

	fibril_rwlock_read_lock(&vol->states_lock);

	for (size_t e = 0; e < vol->extent_no; e++) {
		hr_ext_state_t s = vol->extents[e].state;
		if ((vol->state == HR_VOL_DEGRADED && s != HR_EXT_ONLINE) ||
		    (s == HR_EXT_REBUILD && rebuild_pos < start_stripe)) {
			bad_extent = e;
			break;
		}
	}

	fibril_rwlock_read_unlock(&vol->states_lock);

	for (size_t s = 0; s < stripes_cnt; s++) {
		if (stripes[s].done)
			continue;
		execute_stripe(&stripes[s], bad_extent);
	}

	for (size_t s = 0; s < stripes_cnt; s++) {
		if (stripes[s].done)
			continue;
		wait_for_stripe(&stripes[s]);
	}

	hr_raid5_vol_state_eval(vol);

	rc = EOK;

	fibril_rwlock_read_lock(&vol->states_lock);

	if (vol->state == HR_VOL_FAULTY) {
		fibril_rwlock_read_unlock(&vol->states_lock);
		rc = EIO;
		goto end;
	}

	fibril_rwlock_read_unlock(&vol->states_lock);

	for (size_t s = 0; s < stripes_cnt; s++)
		if (stripes[s].rc == EAGAIN)
			goto retry;

	/* all stripes are done */
end:
	fibril_rwlock_read_unlock(&vol->extents_lock);

	for (size_t i = 0; i < stripes_cnt; i++)
		hr_range_lock_release(rlps[i]);

	hr_destroy_stripes(stripes, stripes_cnt);

	return rc;
}

static errno_t hr_raid5_bd_get_block_size(bd_srv_t *bd, size_t *rsize)
{
	hr_volume_t *vol = bd->srvs->sarg;

	*rsize = vol->bsize;
	return EOK;
}

static errno_t hr_raid5_bd_get_num_blocks(bd_srv_t *bd, aoff64_t *rnb)
{
	hr_volume_t *vol = bd->srvs->sarg;

	*rnb = vol->data_blkno;
	return EOK;
}

static void hr_raid5_vol_state_eval_forced(hr_volume_t *vol)
{
	fibril_rwlock_read_lock(&vol->extents_lock);
	fibril_rwlock_write_lock(&vol->states_lock);

	hr_vol_state_t state = vol->state;

	size_t bad = 0;
	for (size_t i = 0; i < vol->extent_no; i++)
		if (vol->extents[i].state != HR_EXT_ONLINE)
			bad++;

	switch (bad) {
	case 0:
		if (state != HR_VOL_ONLINE)
			hr_update_vol_state(vol, HR_VOL_ONLINE);
		break;
	case 1:
		if (state != HR_VOL_DEGRADED && state != HR_VOL_REBUILD)
			hr_update_vol_state(vol, HR_VOL_DEGRADED);

		if (state != HR_VOL_REBUILD) {
			/* XXX: allow REBUILD on INVALID extents */
			fibril_mutex_lock(&vol->hotspare_lock);
			size_t hs_no = vol->hotspare_no;
			fibril_mutex_unlock(&vol->hotspare_lock);
			if (hs_no > 0) {
				fid_t fib = fibril_create(hr_raid5_rebuild,
				    vol);
				if (fib == 0)
					break;
				fibril_start(fib);
				fibril_detach(fib);
			}
		}
		break;
	default:
		if (state != HR_VOL_FAULTY)
			hr_update_vol_state(vol, HR_VOL_FAULTY);
		break;
	}

	fibril_rwlock_write_unlock(&vol->states_lock);
	fibril_rwlock_read_unlock(&vol->extents_lock);
}

static void xor(void *dst, const void *src, size_t size)
{
	size_t i;
	uint64_t *d = dst;
	const uint64_t *s = src;

	for (i = 0; i < size / sizeof(uint64_t); ++i)
		*d++ ^= *s++;
}

static size_t hr_raid5_parity_extent(hr_level_t level,
    hr_layout_t layout, size_t extent_no, uint64_t strip_no)
{
	switch (level) {
	case HR_LVL_4:
		switch (layout) {
		case HR_LAYOUT_RAID4_0:
			return (0);
		case HR_LAYOUT_RAID4_N:
			return (extent_no - 1);
		default:
			assert(0 && "invalid layout configuration");
		}
	case HR_LVL_5:
		switch (layout) {
		case HR_LAYOUT_RAID5_0R:
			return ((strip_no / (extent_no - 1)) % extent_no);
		case HR_LAYOUT_RAID5_NR:
		case HR_LAYOUT_RAID5_NC:
			return ((extent_no - 1) -
			    (strip_no / (extent_no - 1)) % extent_no);
		default:
			assert(0 && "invalid layout configuration");
		}
	default:
		assert(0 && "invalid layout configuration");
	}
}

static size_t hr_raid5_data_extent(hr_level_t level,
    hr_layout_t layout, size_t extent_no, uint64_t strip_no, size_t p_extent)
{
	switch (level) {
	case HR_LVL_4:
		switch (layout) {
		case HR_LAYOUT_RAID4_0:
			return ((strip_no % (extent_no - 1)) + 1);
		case HR_LAYOUT_RAID4_N:
			return (strip_no % (extent_no - 1));
		default:
			assert(0 && "invalid layout configuration");
		}
	case HR_LVL_5:
		switch (layout) {
		case HR_LAYOUT_RAID5_0R:
		case HR_LAYOUT_RAID5_NR:
			if ((strip_no % (extent_no - 1)) < p_extent)
				return (strip_no % (extent_no - 1));
			else
				return ((strip_no % (extent_no - 1)) + 1);
		case HR_LAYOUT_RAID5_NC:
			return (((strip_no % (extent_no - 1)) + p_extent + 1) %
			    extent_no);
		default:
			assert(0 && "invalid layout configuration");
		}
	default:
		assert(0 && "invalid layout configuration");
	}
}

static errno_t hr_raid5_rebuild(void *arg)
{
	HR_DEBUG("hr_raid5_rebuild()\n");

	hr_volume_t *vol = arg;
	errno_t rc = EOK;
	void *buf = NULL, *xorbuf = NULL;

	fibril_rwlock_read_lock(&vol->extents_lock);
	fibril_rwlock_write_lock(&vol->states_lock);

	if (vol->hotspare_no == 0) {
		HR_WARN("hr_raid5_rebuild(): no free hotspares on \"%s\", "
		    "aborting rebuild\n", vol->devname);
		/* retval isn't checked for now */
		goto end;
	}

	size_t bad = vol->extent_no;
	for (size_t i = 0; i < vol->extent_no; i++) {
		if (vol->extents[i].state == HR_EXT_FAILED) {
			bad = i;
			break;
		}
	}

	if (bad == vol->extent_no) {
		HR_WARN("hr_raid5_rebuild(): no bad extent on \"%s\", "
		    "aborting rebuild\n", vol->devname);
		/* retval isn't checked for now */
		goto end;
	}

	size_t hotspare_idx = vol->hotspare_no - 1;

	hr_ext_state_t hs_state = vol->hotspares[hotspare_idx].state;
	if (hs_state != HR_EXT_HOTSPARE) {
		HR_ERROR("hr_raid5_rebuild(): invalid hotspare state \"%s\", "
		    "aborting rebuild\n", hr_get_ext_state_str(hs_state));
		rc = EINVAL;
		goto end;
	}

	HR_DEBUG("hr_raid5_rebuild(): swapping in hotspare\n");

	block_fini(vol->extents[bad].svc_id);

	vol->extents[bad].svc_id = vol->hotspares[hotspare_idx].svc_id;
	hr_update_ext_state(vol, bad, HR_EXT_HOTSPARE);

	vol->hotspares[hotspare_idx].svc_id = 0;
	fibril_mutex_lock(&vol->hotspare_lock);
	hr_update_hotspare_state(vol, hotspare_idx, HR_EXT_MISSING);
	fibril_mutex_unlock(&vol->hotspare_lock);

	vol->hotspare_no--;

	hr_extent_t *rebuild_ext = &vol->extents[bad];

	HR_DEBUG("hr_raid5_rebuild(): starting rebuild on (%" PRIun ")\n",
	    rebuild_ext->svc_id);

	hr_update_ext_state(vol, bad, HR_EXT_REBUILD);
	hr_update_vol_state(vol, HR_VOL_REBUILD);

	uint64_t max_blks = DATA_XFER_LIMIT / vol->bsize;
	uint64_t left = vol->data_blkno / (vol->extent_no - 1);
	buf = malloc(max_blks * vol->bsize);
	xorbuf = malloc(max_blks * vol->bsize);

	uint64_t ba = 0, cnt;
	hr_add_data_offset(vol, &ba);

	while (left != 0) {
		cnt = min(left, max_blks);

		/*
		 * Almost the same as read_degraded,
		 * but we don't want to allocate new
		 * xorbuf each blk rebuild batch.
		 */
		bool first = true;
		for (size_t i = 0; i < vol->extent_no; i++) {
			if (i == bad)
				continue;
			if (first)
				rc = block_read_direct(vol->extents[i].svc_id,
				    ba, cnt, xorbuf);
			else
				rc = block_read_direct(vol->extents[i].svc_id,
				    ba, cnt, buf);
			if (rc != EOK) {
				hr_raid5_ext_state_cb(vol, i, rc);
				HR_ERROR("rebuild on \"%s\" (%" PRIun "), "
				    "failed due to a failed ONLINE extent, "
				    "number %zu\n",
				    vol->devname, vol->svc_id, i);
				goto end;
			}

			if (!first)
				xor(xorbuf, buf, cnt * vol->bsize);
			else
				first = false;
		}

		rc = block_write_direct(rebuild_ext->svc_id, ba, cnt, xorbuf);
		if (rc != EOK) {
			hr_raid5_ext_state_cb(vol, bad, rc);
			HR_ERROR("rebuild on \"%s\" (%" PRIun "), failed due to "
			    "the rebuilt extent number %zu failing\n",
			    vol->devname, vol->svc_id, bad);
			goto end;
		}

		ba += cnt;
		left -= cnt;

		/*
		 * Let other IO requests be served
		 * during rebuild.
		 */

		/*
		 * fibril_rwlock_write_unlock(&vol->states_lock);
		 * fibril_mutex_unlock(&vol->lock);
		 * fibril_mutex_lock(&vol->lock);
		 * fibril_rwlock_write_lock(&vol->states_lock);
		 */
	}

	HR_DEBUG("hr_raid5_rebuild(): rebuild finished on \"%s\" (%" PRIun "), "
	    "extent number %zu\n", vol->devname, vol->svc_id, hotspare_idx);

	hr_update_ext_state(vol, bad, HR_EXT_ONLINE);

	fibril_rwlock_write_unlock(&vol->states_lock);
	fibril_rwlock_read_unlock(&vol->extents_lock);

	rc = vol->meta_ops->save(vol, WITH_STATE_CALLBACK);

	fibril_rwlock_read_lock(&vol->extents_lock);
	fibril_rwlock_write_lock(&vol->states_lock);

end:
	hr_raid5_vol_state_eval_forced(vol);

	fibril_rwlock_write_unlock(&vol->states_lock);
	fibril_rwlock_read_unlock(&vol->extents_lock);

	if (buf != NULL)
		free(buf);

	if (xorbuf != NULL)
		free(xorbuf);

	return rc;
}

/** @}
 */
