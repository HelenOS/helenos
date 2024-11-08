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

#include "superblock.h"
#include "util.h"
#include "var.h"

extern loc_srv_t *hr_srv;

static errno_t hr_raid0_bd_open(bd_srvs_t *, bd_srv_t *);
static errno_t hr_raid0_bd_close(bd_srv_t *);
static errno_t hr_raid0_bd_read_blocks(bd_srv_t *, aoff64_t, size_t, void *,
    size_t);
static errno_t hr_raid0_bd_sync_cache(bd_srv_t *, aoff64_t, size_t);
static errno_t hr_raid0_bd_write_blocks(bd_srv_t *, aoff64_t, size_t,
    const void *, size_t);
static errno_t hr_raid0_bd_get_block_size(bd_srv_t *, size_t *);
static errno_t hr_raid0_bd_get_num_blocks(bd_srv_t *, aoff64_t *);

static bd_ops_t hr_raid0_bd_ops = {
	.open = hr_raid0_bd_open,
	.close = hr_raid0_bd_close,
	.sync_cache = hr_raid0_bd_sync_cache,
	.read_blocks = hr_raid0_bd_read_blocks,
	.write_blocks = hr_raid0_bd_write_blocks,
	.get_block_size = hr_raid0_bd_get_block_size,
	.get_num_blocks = hr_raid0_bd_get_num_blocks
};

static errno_t hr_raid0_check_vol_status(hr_volume_t *vol)
{
	if (vol->status == HR_VOL_ONLINE)
		return EOK;
	return EINVAL;
}

/*
 * Update vol->status and return EOK if volume
 * is usable
 */
static errno_t hr_raid0_update_vol_status(hr_volume_t *vol)
{
	for (size_t i = 0; i < vol->dev_no; i++) {
		if (vol->extents[i].status != HR_EXT_ONLINE) {
			log_msg(LOG_DEFAULT, LVL_ERROR,
			    "RAID 0 needs all extents to be ONLINE, marking "
			    "\"%s\" (%lu) as FAULTY",
			    vol->devname, vol->svc_id);
			vol->status = HR_VOL_FAULTY;
			return EINVAL;
		}
	}

	vol->status = HR_VOL_ONLINE;
	return EOK;
}

static errno_t hr_raid0_bd_open(bd_srvs_t *bds, bd_srv_t *bd)
{
	log_msg(LOG_DEFAULT, LVL_NOTE, "hr_bd_open()");
	return EOK;
}

static errno_t hr_raid0_bd_close(bd_srv_t *bd)
{
	log_msg(LOG_DEFAULT, LVL_NOTE, "hr_bd_close()");
	return EOK;
}

static errno_t hr_raid0_bd_op(hr_bd_op_type_t type, bd_srv_t *bd, aoff64_t ba,
    size_t cnt, void *data_read, const void *data_write, size_t size)
{
	hr_volume_t *vol = bd->srvs->sarg;
	errno_t rc;
	uint64_t phys_block;
	size_t left;

	/* propagate sync */
	if (type == HR_BD_SYNC && ba == 0 && cnt == 0) {
		hr_sync_all_extents(vol);
		rc = hr_raid0_update_vol_status(vol);
		return rc;
	}

	if (type == HR_BD_READ || type == HR_BD_WRITE)
		if (size < cnt * vol->bsize)
			return EINVAL;

	rc = hr_check_ba_range(vol, cnt, ba);
	if (rc != EOK)
		return rc;

	uint64_t strip_size = vol->strip_size / vol->bsize; /* in blocks */
	uint64_t stripe = ba / strip_size; /* stripe number */
	uint64_t extent = stripe % vol->dev_no;
	uint64_t ext_stripe = stripe / vol->dev_no; /* stripe level */
	uint64_t strip_off = ba % strip_size; /* strip offset */

	fibril_mutex_lock(&vol->lock);

	rc = hr_raid0_check_vol_status(vol);
	if (rc != EOK) {
		fibril_mutex_unlock(&vol->lock);
		return EIO;
	}

	left = cnt;
	while (left != 0) {
		phys_block = ext_stripe * strip_size + strip_off;
		cnt = min(left, strip_size - strip_off);
		hr_add_ba_offset(vol, &phys_block);
		switch (type) {
		case HR_BD_SYNC:
			rc = block_sync_cache(vol->extents[extent].svc_id,
			    phys_block, cnt);
			/* allow unsupported sync */
			if (rc == ENOTSUP)
				rc = EOK;
			break;
		case HR_BD_READ:
			rc = block_read_direct(vol->extents[extent].svc_id,
			    phys_block, cnt, data_read);
			data_read = (void *) ((uintptr_t) data_read +
			    (vol->bsize * cnt));
			break;
		case HR_BD_WRITE:
			rc = block_write_direct(vol->extents[extent].svc_id,
			    phys_block, cnt, data_write);
			data_write = (void *) ((uintptr_t) data_write +
			    (vol->bsize * cnt));
			break;
		default:
			rc = EINVAL;
		}

		if (rc == ENOENT) {
			hr_update_ext_status(vol, extent, HR_EXT_MISSING);
			rc = EIO;
			goto error;
		} else if (rc != EOK) {
			hr_update_ext_status(vol, extent, HR_EXT_FAILED);
			rc = EIO;
			goto error;
		}

		left -= cnt;
		strip_off = 0;
		extent++;
		if (extent >= vol->dev_no) {
			ext_stripe++;
			extent = 0;
		}
	}

error:
	(void) hr_raid0_update_vol_status(vol);
	fibril_mutex_unlock(&vol->lock);
	return rc;
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

errno_t hr_raid0_create(hr_volume_t *new_volume)
{
	errno_t rc;

	assert(new_volume->level == HR_LVL_0);

	if (new_volume->dev_no < 2) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "RAID 0 array needs at least 2 devices");
		return EINVAL;
	}

	rc = hr_raid0_update_vol_status(new_volume);
	if (rc != EOK)
		return rc;

	bd_srvs_init(&new_volume->hr_bds);
	new_volume->hr_bds.ops = &hr_raid0_bd_ops;
	new_volume->hr_bds.sarg = new_volume;

	rc = hr_register_volume(new_volume);

	return rc;
}

errno_t hr_raid0_init(hr_volume_t *vol)
{
	errno_t rc;
	size_t bsize;
	uint64_t total_blkno;

	assert(vol->level == HR_LVL_0);

	rc = hr_check_devs(vol, &total_blkno, &bsize);
	if (rc != EOK)
		return rc;

	vol->nblocks = total_blkno;
	vol->bsize = bsize;
	vol->data_offset = HR_DATA_OFF;
	vol->data_blkno = vol->nblocks - (vol->data_offset * vol->dev_no);
	vol->strip_size = HR_STRIP_SIZE;

	return EOK;
}

/** @}
 */
