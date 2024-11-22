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
#include <mem.h>
#include <task.h>
#include <stdio.h>
#include <stdlib.h>
#include <str_error.h>

#include "superblock.h"
#include "util.h"
#include "var.h"

extern loc_srv_t *hr_srv;

static errno_t hr_raid5_vol_usable(hr_volume_t *);
static ssize_t hr_raid5_get_bad_ext(hr_volume_t *);
static errno_t hr_raid5_update_vol_status(hr_volume_t *);
static void xor(void *, const void *, size_t);
static errno_t hr_raid5_read_degraded(hr_volume_t *, uint64_t, uint64_t,
    void *, size_t);
static errno_t hr_raid5_write(hr_volume_t *, uint64_t, uint64_t, aoff64_t,
    const void *, size_t);
static errno_t hr_raid5_write_parity(hr_volume_t *, uint64_t, uint64_t,
    uint64_t, const void *, size_t);
static errno_t hr_raid5_bd_op(hr_bd_op_type_t, bd_srv_t *, aoff64_t, size_t,
    void *, const void *, size_t);

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

errno_t hr_raid5_create(hr_volume_t *new_volume)
{
	errno_t rc;

	assert(new_volume->level == HR_LVL_5);

	if (new_volume->dev_no < 3) {
		HR_ERROR("RAID 5 array needs at least 3 devices\n");
		return EINVAL;
	}

	rc = hr_raid5_update_vol_status(new_volume);
	if (rc != EOK)
		return rc;

	bd_srvs_init(&new_volume->hr_bds);
	new_volume->hr_bds.ops = &hr_raid5_bd_ops;
	new_volume->hr_bds.sarg = new_volume;

	rc = hr_register_volume(new_volume);

	return rc;
}

errno_t hr_raid5_init(hr_volume_t *vol)
{
	errno_t rc;
	size_t bsize;
	uint64_t total_blkno;

	assert(vol->level == HR_LVL_5);

	rc = hr_check_devs(vol, &total_blkno, &bsize);
	if (rc != EOK)
		return rc;

	vol->nblocks = total_blkno;
	vol->bsize = bsize;
	vol->data_offset = HR_DATA_OFF;
	vol->data_blkno = vol->nblocks - (vol->data_offset * vol->dev_no) -
	    (vol->nblocks / vol->dev_no);
	vol->strip_size = HR_STRIP_SIZE;

	return EOK;
}

void hr_raid5_status_event(hr_volume_t *vol)
{
	fibril_mutex_lock(&vol->lock);
	(void) hr_raid5_update_vol_status(vol);
	fibril_mutex_unlock(&vol->lock);
}

static errno_t hr_raid5_bd_open(bd_srvs_t *bds, bd_srv_t *bd)
{
	HR_DEBUG("hr_bd_open()\n");
	return EOK;
}

static errno_t hr_raid5_bd_close(bd_srv_t *bd)
{
	HR_DEBUG("hr_bd_close()\n");
	return EOK;
}

static errno_t hr_raid5_bd_sync_cache(bd_srv_t *bd, aoff64_t ba, size_t cnt)
{
	return hr_raid5_bd_op(HR_BD_SYNC, bd, ba, cnt, NULL, NULL, 0);
}

static errno_t hr_raid5_bd_read_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    void *buf, size_t size)
{
	return hr_raid5_bd_op(HR_BD_READ, bd, ba, cnt, buf, NULL, size);
}

static errno_t hr_raid5_bd_write_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    const void *data, size_t size)
{
	return hr_raid5_bd_op(HR_BD_WRITE, bd, ba, cnt, NULL, data, size);
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

static errno_t hr_raid5_vol_usable(hr_volume_t *vol)
{
	if (vol->status == HR_VOL_ONLINE ||
	    vol->status == HR_VOL_DEGRADED)
		return EOK;
	return EINVAL;
}

/*
 * Returns (-1) if all extents are online,
 * else returns index of first bad one.
 */
static ssize_t hr_raid5_get_bad_ext(hr_volume_t *vol)
{
	for (size_t i = 0; i < vol->dev_no; i++)
		if (vol->extents[i].status != HR_EXT_ONLINE)
			return i;
	return -1;
}

static errno_t hr_raid5_update_vol_status(hr_volume_t *vol)
{
	hr_vol_status_t old_state = vol->status;
	size_t bad = 0;
	for (size_t i = 0; i < vol->dev_no; i++)
		if (vol->extents[i].status != HR_EXT_ONLINE)
			bad++;

	switch (bad) {
	case 0:
		if (old_state != HR_VOL_ONLINE) {
			HR_WARN("RAID 5 has all extents online, "
			    "marking \"%s\" (%lu) as ONLINE",
			    vol->devname, vol->svc_id);
			vol->status = HR_VOL_ONLINE;
		}
		return EOK;
	case 1:
		if (old_state != HR_VOL_DEGRADED) {
			HR_WARN("RAID 5 array \"%s\" (%lu) has 1 extent "
			    "inactive, marking as DEGRADED",
			    vol->devname, vol->svc_id);
			vol->status = HR_VOL_DEGRADED;
		}
		return EOK;
	default:
		if (old_state != HR_VOL_FAULTY) {
			HR_WARN("RAID 5 array \"%s\" (%lu) has more "
			    "than one 1 extent inactive, marking as FAULTY",
			    vol->devname, vol->svc_id);
			vol->status = HR_VOL_FAULTY;
		}
		return EINVAL;
	}
}

static void xor(void *dst, const void *src, size_t size)
{
	size_t i;
	uint64_t *d = dst;
	const uint64_t *s = src;

	for (i = 0; i < size / sizeof(uint64_t); ++i)
		*d++ ^= *s++;
}

static errno_t hr_raid5_read_degraded(hr_volume_t *vol, uint64_t bad,
    uint64_t block, void *data, size_t cnt)
{
	errno_t rc;
	size_t i;
	void *xorbuf;
	void *buf;
	uint64_t len = vol->bsize * cnt;

	xorbuf = malloc(len);
	if (xorbuf == NULL)
		return ENOMEM;

	buf = malloc(len);
	if (buf == NULL) {
		free(xorbuf);
		return ENOMEM;
	}

	/* read all other extents in the stripe */
	memset(xorbuf, 0, len);
	for (i = 0; i < vol->dev_no; i++) {
		if (i == bad) {
			continue;
		} else {
			rc = block_read_direct(vol->extents[i].svc_id, block,
			    cnt, buf);
			if (rc != EOK)
				goto end;
			xor(xorbuf, buf, len);
		}
	}

	memcpy(data, xorbuf, len);
end:
	free(xorbuf);
	free(buf);
	return rc;
}

static errno_t hr_raid5_write(hr_volume_t *vol, uint64_t p_extent,
    uint64_t extent, aoff64_t ba, const void *data, size_t cnt)
{
	errno_t rc;
	size_t i;
	void *xorbuf;
	void *buf;
	uint64_t len = vol->bsize * cnt;

	ssize_t bad = hr_raid5_get_bad_ext(vol);
	if (bad == -1 || (size_t)bad == p_extent) {
		rc = block_write_direct(vol->extents[extent].svc_id, ba, cnt,
		    data);
		if (rc != EOK)
			return rc;
		/*
		 * DEGRADED parity - skip parity write
		 */
		if ((size_t)bad == p_extent)
			return EOK;

		rc = hr_raid5_write_parity(vol, p_extent, extent, ba, data,
		    cnt);
		return rc;
	}

	xorbuf = malloc(len);
	if (xorbuf == NULL)
		return ENOMEM;

	buf = malloc(len);
	if (buf == NULL) {
		free(xorbuf);
		return ENOMEM;
	}

	if (extent == (size_t) bad) {
		/*
		 * new parity = read other and xor in new data
		 *
		 * write new parity
		 */
		memset(xorbuf, 0, len);
		for (i = 1; i < vol->dev_no; i++) {
			if (i == (size_t) bad) {
				continue;
			} else {
				rc = block_read_direct(vol->extents[i].svc_id,
				    ba, cnt, buf);
				if (rc != EOK)
					goto end;
				xor(xorbuf, buf, len);
			}
		}
		xor(xorbuf, data, len);
		rc = block_write_direct(vol->extents[p_extent].svc_id, ba, cnt,
		    xorbuf);
		if (rc != EOK)
			goto end;
	} else {
		/*
		 * new parity = xor original data and old parity and new data
		 *
		 * write parity, new data
		 */
		rc = block_read_direct(vol->extents[extent].svc_id, ba, cnt,
		    xorbuf);
		if (rc != EOK)
			goto end;
		rc = block_read_direct(vol->extents[p_extent].svc_id, ba, cnt,
		    buf);
		if (rc != EOK)
			goto end;

		xor(xorbuf, buf, len);

		xor(xorbuf, data, len);

		rc = block_write_direct(vol->extents[p_extent].svc_id, ba, cnt,
		    xorbuf);
		if (rc != EOK)
			goto end;
		rc = block_write_direct(vol->extents[extent].svc_id, ba, cnt,
		    data);
		if (rc != EOK)
			goto end;
	}
end:
	free(xorbuf);
	free(buf);
	return rc;
}

static errno_t hr_raid5_write_parity(hr_volume_t *vol, uint64_t p_extent,
    uint64_t extent, uint64_t block, const void *data, size_t cnt)
{
	errno_t rc;
	size_t i;
	void *xorbuf;
	void *buf;
	uint64_t len = vol->bsize * cnt;

	xorbuf = malloc(len);
	if (xorbuf == NULL)
		return ENOMEM;

	buf = malloc(len);
	if (buf == NULL) {
		free(xorbuf);
		return ENOMEM;
	}

	memset(xorbuf, 0, len);
	for (i = 0; i < vol->dev_no; i++) {
		if (i == p_extent)
			continue;
		if (i == extent) {
			xor(xorbuf, data, len);
		} else {
			rc = block_read_direct(vol->extents[i].svc_id,
			    block, cnt, buf);
			if (rc != EOK)
				goto end;
			xor(xorbuf, buf, len);
		}
	}

	rc = block_write_direct(vol->extents[p_extent].svc_id, block, cnt,
	    xorbuf);
end:
	free(xorbuf);
	free(buf);
	return rc;
}

static errno_t hr_raid5_bd_op(hr_bd_op_type_t type, bd_srv_t *bd, aoff64_t ba,
    size_t cnt, void *dst, const void *src, size_t size)
{
	hr_volume_t *vol = bd->srvs->sarg;
	errno_t rc;
	uint64_t phys_block, len;
	size_t left;
	const uint8_t *data_write = src;
	uint8_t *data_read = dst;

	/* propagate sync */
	if (type == HR_BD_SYNC && ba == 0 && cnt == 0) {
		hr_sync_all_extents(vol);
		rc = hr_raid5_update_vol_status(vol);
		return rc;
	}

	if (type == HR_BD_READ || type == HR_BD_WRITE)
		if (size < cnt * vol->bsize)
			return EINVAL;

	rc = hr_check_ba_range(vol, cnt, ba);
	if (rc != EOK)
		return rc;

	uint64_t strip_size = vol->strip_size / vol->bsize; /* in blocks */
	uint64_t stripe = (ba / strip_size); /* stripe number */
	uint64_t p_extent = (stripe / (vol->dev_no - 1)) % vol->dev_no; /* parity extent */
	uint64_t extent;
	if ((stripe % (vol->dev_no - 1)) < p_extent)
		extent = (stripe % (vol->dev_no - 1));
	else
		extent = ((stripe % (vol->dev_no - 1)) + 1);
	uint64_t ext_stripe = stripe / (vol->dev_no - 1); /* stripe level */
	uint64_t strip_off = ba % strip_size; /* strip offset */

	fibril_mutex_lock(&vol->lock);

	rc = hr_raid5_vol_usable(vol);
	if (rc != EOK) {
		fibril_mutex_unlock(&vol->lock);
		return EIO;
	}

	left = cnt;
	while (left != 0) {
		phys_block = ext_stripe * strip_size + strip_off;
		cnt = min(left, strip_size - strip_off);
		len = vol->bsize * cnt;
		hr_add_ba_offset(vol, &phys_block);
		switch (type) {
		case HR_BD_SYNC:
			if (vol->extents[extent].status != HR_EXT_ONLINE)
				break;
			rc = block_sync_cache(vol->extents[extent].svc_id,
			    phys_block, cnt);
			/* allow unsupported sync */
			if (rc == ENOTSUP)
				rc = EOK;
			break;
		case HR_BD_READ:
		retry_read:
			ssize_t bad = hr_raid5_get_bad_ext(vol);
			if (bad > 0 && extent == (size_t) bad) {
				rc = hr_raid5_read_degraded(vol, bad,
				    phys_block, data_read, cnt);
			} else {
				rc = block_read_direct(vol->extents[extent].svc_id,
				    phys_block, cnt, data_read);
			}
			data_read += len;
			break;
		case HR_BD_WRITE:
		retry_write:
			rc = hr_raid5_write(vol, p_extent, extent, phys_block,
			    data_write, cnt);
			data_write += len;
			break;
		default:
			rc = EINVAL;
			goto error;
		}

		if (rc == ENOMEM)
			goto error;

		if (rc == ENOENT)
			hr_update_ext_status(vol, extent, HR_EXT_MISSING);
		else if (rc != EOK)
			hr_update_ext_status(vol, extent, HR_EXT_FAILED);

		if (rc != EOK) {
			rc = hr_raid5_update_vol_status(vol);
			if (rc == EOK) {
				/*
				 * State changed from ONLINE -> DEGRADED,
				 * rewind and retry
				 */
				if (type == HR_BD_WRITE) {
					data_write -= len;
					goto retry_write;
				} else if (type == HR_BD_WRITE) {
					data_read -= len;
					goto retry_read;
				}
			} else {
				rc = EIO;
				goto error;
			}
		}

		left -= cnt;
		strip_off = 0;
		if (extent + 1 >= vol->dev_no ||
		    (extent + 1 == p_extent && p_extent + 1 >= vol->dev_no))
			ext_stripe++;
		stripe++;
		p_extent = (stripe / (vol->dev_no - 1)) % vol->dev_no; /* parity extent */
		if ((stripe % (vol->dev_no - 1)) < p_extent)
			extent = (stripe % (vol->dev_no - 1));
		else
			extent = ((stripe % (vol->dev_no - 1)) + 1);
	}

error:
	(void) hr_raid5_update_vol_status(vol);
	fibril_mutex_unlock(&vol->lock);
	return rc;
}

/** @}
 */
