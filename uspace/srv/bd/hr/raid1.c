/*
 * Copyright (c) 2024 Miroslav Cimerman
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
#include <stdio.h>
#include <stdlib.h>
#include <str_error.h>

#include "superblock.h"
#include "util.h"
#include "var.h"

extern loc_srv_t *hr_srv;

static errno_t hr_raid1_check_vol_status(hr_volume_t *);
static errno_t hr_raid1_update_vol_status(hr_volume_t *);
static void handle_extent_error(hr_volume_t *, size_t, errno_t);
static errno_t hr_raid1_bd_op(hr_bd_op_type_t, bd_srv_t *, aoff64_t, size_t,
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

errno_t hr_raid1_create(hr_volume_t *new_volume)
{
	errno_t rc;

	assert(new_volume->level == HR_LVL_1);

	if (new_volume->dev_no < 2) {
		HR_ERROR("RAID 1 array needs at least 2 devices\n");
		return EINVAL;
	}

	rc = hr_raid1_update_vol_status(new_volume);
	if (rc != EOK)
		return rc;

	bd_srvs_init(&new_volume->hr_bds);
	new_volume->hr_bds.ops = &hr_raid1_bd_ops;
	new_volume->hr_bds.sarg = new_volume;

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

	vol->nblocks = total_blkno / vol->dev_no;
	vol->bsize = bsize;
	vol->data_offset = HR_DATA_OFF;
	vol->data_blkno = vol->nblocks - vol->data_offset;
	vol->strip_size = 0;

	return EOK;
}

void hr_raid1_status_event(hr_volume_t *vol)
{
	fibril_mutex_lock(&vol->lock);
	(void) hr_raid1_update_vol_status(vol);
	fibril_mutex_unlock(&vol->lock);
}

errno_t hr_raid1_add_hotspare(hr_volume_t *vol, service_id_t hotspare)
{
	HR_DEBUG("hr_raid1_add_hotspare()\n");

	fibril_mutex_lock(&vol->lock);

	if (vol->hotspare_no >= HR_MAX_HOTSPARES) {
		HR_ERROR("hr_raid1_add_hotspare(): cannot add more hotspares "
		    "to \"%s\"\n", vol->devname);
		fibril_mutex_unlock(&vol->lock);
		return ELIMIT;
	}

	vol->hotspares[vol->hotspare_no].svc_id = hotspare;
	vol->hotspares[vol->hotspare_no].status = HR_EXT_HOTSPARE;
	vol->hotspare_no++;

	/*
	 * If the volume is degraded, start rebuild right away.
	 */
	if (vol->status == HR_VOL_DEGRADED) {
		HR_DEBUG("hr_raid1_add_hotspare(): volume in DEGRADED state, "
		    "spawning new rebuild fibril\n");
		fid_t fib = fibril_create(hr_raid1_rebuild, vol);
		if (fib == 0)
			return EINVAL;
		fibril_start(fib);
		fibril_detach(fib);
	}

	fibril_mutex_unlock(&vol->lock);

	return EOK;
}

static errno_t hr_raid1_bd_open(bd_srvs_t *bds, bd_srv_t *bd)
{
	HR_DEBUG("hr_bd_open()\n");
	return EOK;
}

static errno_t hr_raid1_bd_close(bd_srv_t *bd)
{
	HR_DEBUG("hr_bd_close()\n");
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

static errno_t hr_raid1_check_vol_status(hr_volume_t *vol)
{
	if (vol->status == HR_VOL_ONLINE ||
	    vol->status == HR_VOL_DEGRADED ||
	    vol->status == HR_VOL_REBUILD)
		return EOK;
	return EINVAL;
}

/*
 * Update vol->status and return EOK if volume
 * is usable
 */
static errno_t hr_raid1_update_vol_status(hr_volume_t *vol)
{
	hr_vol_status_t old_state = vol->status;
	size_t healthy = hr_count_extents(vol, HR_EXT_ONLINE);

	if (healthy == 0) {
		if (old_state != HR_VOL_FAULTY) {
			HR_WARN("RAID 1 needs at least 1 extent to be"
			    "ONLINE, marking \"%s\" (%lu) volume as FAULTY",
			    vol->devname, vol->svc_id);
			vol->status = HR_VOL_FAULTY;
		}
		return EINVAL;
	} else if (healthy < vol->dev_no) {
		if (old_state != HR_VOL_DEGRADED &&
		    old_state != HR_VOL_REBUILD) {
			HR_WARN("RAID 1 array \"%s\" (%lu) has some "
			    "unusable extent(s), marking volume as DEGRADED",
			    vol->devname, vol->svc_id);
			vol->status = HR_VOL_DEGRADED;
			if (vol->hotspare_no > 0) {
				fid_t fib = fibril_create(hr_raid1_rebuild,
				    vol);
				if (fib == 0) {
					return EINVAL;
				}
				fibril_start(fib);
				fibril_detach(fib);
			}
		}
		return EOK;
	} else {
		if (old_state != HR_VOL_ONLINE) {
			HR_WARN("RAID 1 array \"%s\" (%lu) has all extents "
			    "active, marking volume as ONLINE",
			    vol->devname, vol->svc_id);
			vol->status = HR_VOL_ONLINE;
		}
		return EOK;
	}
}

static void handle_extent_error(hr_volume_t *vol, size_t extent,
    errno_t rc)
{
	if (rc == ENOENT)
		hr_update_ext_status(vol, extent, HR_EXT_MISSING);
	else if (rc != EOK)
		hr_update_ext_status(vol, extent, HR_EXT_FAILED);
}

static errno_t hr_raid1_bd_op(hr_bd_op_type_t type, bd_srv_t *bd, aoff64_t ba,
    size_t cnt, void *data_read, const void *data_write, size_t size)
{
	hr_volume_t *vol = bd->srvs->sarg;
	errno_t rc;
	size_t i;

	if (type == HR_BD_READ || type == HR_BD_WRITE)
		if (size < cnt * vol->bsize)
			return EINVAL;

	rc = hr_check_ba_range(vol, cnt, ba);
	if (rc != EOK)
		return rc;

	hr_add_ba_offset(vol, &ba);

	fibril_mutex_lock(&vol->lock);

	rc = hr_raid1_check_vol_status(vol);
	if (rc != EOK) {
		fibril_mutex_unlock(&vol->lock);
		return EIO;
	}

	size_t successful = 0;
	switch (type) {
	case HR_BD_SYNC:
		for (i = 0; i < vol->dev_no; i++) {
			if (vol->extents[i].status != HR_EXT_ONLINE)
				continue;
			rc = block_sync_cache(vol->extents[i].svc_id, ba, cnt);
			if (rc != EOK && rc != ENOTSUP)
				handle_extent_error(vol, i, rc);
			else
				successful++;
		}
		break;
	case HR_BD_READ:
		for (i = 0; i < vol->dev_no; i++) {
			if (vol->extents[i].status != HR_EXT_ONLINE)
				continue;
			rc = block_read_direct(vol->extents[i].svc_id, ba, cnt,
			    data_read);
			if (rc != EOK) {
				handle_extent_error(vol, i, rc);
			} else {
				successful++;
				break;
			}
		}
		break;
	case HR_BD_WRITE:
		for (i = 0; i < vol->dev_no; i++) {
			if (vol->extents[i].status != HR_EXT_ONLINE ||
			    (vol->extents[i].status == HR_EXT_REBUILD &&
			    ba >= vol->rebuild_blk))
				/*
				 * When the extent is being rebuilt,
				 * we only write to the part that is already
				 * rebuilt. If ba is more than vol->rebuild_blk,
				 * the write is going to be replicated later
				 * in the rebuild. TODO: test
				 */
				continue;
			rc = block_write_direct(vol->extents[i].svc_id, ba, cnt,
			    data_write);
			if (rc != EOK)
				handle_extent_error(vol, i, rc);
			else
				successful++;
		}
		break;
	default:
		rc = EINVAL;
	}

	if (successful > 0)
		rc = EOK;
	else
		rc = EIO;

	(void) hr_raid1_update_vol_status(vol);
	fibril_mutex_unlock(&vol->lock);
	return rc;
}

/*
 * Put the last HOTSPARE extent in place
 * of first DEGRADED, and start the rebuild.
 */
static errno_t hr_raid1_rebuild(void *arg)
{
	HR_DEBUG("hr_raid1_rebuild()\n");

	hr_volume_t *vol = arg;
	void *buf = NULL;
	errno_t rc = EOK;

	fibril_mutex_lock(&vol->lock);

	if (vol->hotspare_no == 0) {
		HR_WARN("hr_raid1_rebuild(): no free hotspares on \"%s\", "
		    "aborting rebuild\n", vol->devname);
		/* retval isn't checked for now */
		goto end;
	}

	size_t bad = vol->dev_no;
	for (size_t i = 0; i < vol->dev_no; i++) {
		if (vol->extents[i].status == HR_EXT_FAILED) {
			bad = i;
			break;
		}
	}

	if (bad == vol->dev_no) {
		HR_WARN("hr_raid1_rebuild(): no bad extent on \"%s\", "
		    "aborting rebuild\n", vol->devname);
		/* retval isn't checked for now */
		goto end;
	}

	block_fini(vol->extents[bad].svc_id);

	size_t hotspare_idx = vol->hotspare_no - 1;

	vol->rebuild_blk = 0;
	vol->extents[bad].svc_id = vol->hotspares[hotspare_idx].svc_id;
	hr_update_ext_status(vol, bad, HR_EXT_REBUILD);

	vol->hotspares[hotspare_idx].svc_id = 0;
	vol->hotspares[hotspare_idx].status = HR_EXT_MISSING;
	vol->hotspare_no--;

	HR_WARN("hr_raid1_rebuild(): changing volume \"%s\" (%lu) state "
	    "from %s to %s\n", vol->devname, vol->svc_id,
	    hr_get_vol_status_msg(vol->status),
	    hr_get_vol_status_msg(HR_VOL_REBUILD));
	vol->status = HR_VOL_REBUILD;

	hr_extent_t *hotspare = &vol->extents[bad];

	HR_DEBUG("hr_raid1_rebuild(): initing (%lu)\n", hotspare->svc_id);

	rc = block_init(hotspare->svc_id);
	if (rc != EOK) {
		HR_ERROR("hr_raid1_rebuild(): initing (%lu) failed, "
		    "aborting rebuild\n", hotspare->svc_id);
		goto end;
	}

	size_t left = vol->data_blkno;
	size_t max_blks = DATA_XFER_LIMIT / vol->bsize;
	buf = malloc(max_blks * vol->bsize);

	hr_extent_t *ext;
	size_t rebuild_ext_idx = bad;

	size_t cnt;
	uint64_t ba = 0;
	hr_add_ba_offset(vol, &ba);
	while (left != 0) {
		vol->rebuild_blk = ba;
		cnt = min(max_blks, left);
		for (size_t i = 0; i < vol->dev_no; i++) {
			ext = &vol->extents[i];
			if (ext->status == HR_EXT_ONLINE) {
				rc = block_read_direct(ext->svc_id, ba, cnt,
				    buf);
				if (rc != EOK) {
					handle_extent_error(vol, i, rc);
					if (i + 1 < vol->dev_no) {
						/* still might have one ONLINE */
						continue;
					} else {
						HR_ERROR("rebuild on \"%s\" (%lu), failed due to "
						    "too many failed extents\n",
						    vol->devname, vol->svc_id);
						goto end;
					}
				}
				break;
			}
		}

		rc = block_write_direct(hotspare->svc_id, ba, cnt, buf);
		if (rc != EOK) {
			handle_extent_error(vol, rebuild_ext_idx, rc);
			HR_ERROR("rebuild on \"%s\" (%lu), extent number %lu\n",
			    vol->devname, vol->svc_id, rebuild_ext_idx);
			goto end;

		}

		ba += cnt;
		left -= cnt;
	}

	HR_DEBUG("hr_raid1_rebuild(): rebuild finished on \"%s\" (%lu), "
	    "extent number %lu\n", vol->devname, vol->svc_id, hotspare_idx);

	hr_update_ext_status(vol, hotspare_idx, HR_EXT_ONLINE);
	/*
	 * For now write metadata at the end, because
	 * we don't sync metada accross extents yet.
	 */
	hr_write_meta_to_ext(vol, bad);
end:
	(void) hr_raid1_update_vol_status(vol);

	fibril_mutex_unlock(&vol->lock);

	if (buf != NULL)
		free(buf);

	/* retval isn't checked anywhere for now */
	return rc;
}

/** @}
 */
