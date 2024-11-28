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

static errno_t hr_raid4_vol_usable(hr_volume_t *);
static ssize_t hr_raid4_get_bad_ext(hr_volume_t *);
static errno_t hr_raid4_update_vol_status(hr_volume_t *);
static void hr_raid4_handle_extent_error(hr_volume_t *, size_t, errno_t);
static void xor(void *, const void *, size_t);
static errno_t hr_raid4_read_degraded(hr_volume_t *, uint64_t, uint64_t,
    void *, size_t);
static errno_t hr_raid4_write(hr_volume_t *, uint64_t, aoff64_t, const void *,
    size_t);
static errno_t hr_raid4_write_parity(hr_volume_t *, uint64_t, uint64_t,
    const void *, size_t);
static errno_t hr_raid4_bd_op(hr_bd_op_type_t, bd_srv_t *, aoff64_t, size_t,
    void *, const void *, size_t);
static errno_t hr_raid4_rebuild(void *);

/* bdops */
static errno_t hr_raid4_bd_open(bd_srvs_t *, bd_srv_t *);
static errno_t hr_raid4_bd_close(bd_srv_t *);
static errno_t hr_raid4_bd_read_blocks(bd_srv_t *, aoff64_t, size_t, void *,
    size_t);
static errno_t hr_raid4_bd_sync_cache(bd_srv_t *, aoff64_t, size_t);
static errno_t hr_raid4_bd_write_blocks(bd_srv_t *, aoff64_t, size_t,
    const void *, size_t);
static errno_t hr_raid4_bd_get_block_size(bd_srv_t *, size_t *);
static errno_t hr_raid4_bd_get_num_blocks(bd_srv_t *, aoff64_t *);

static errno_t hr_raid4_write_parity(hr_volume_t *, uint64_t, uint64_t,
    const void *, size_t);

static bd_ops_t hr_raid4_bd_ops = {
	.open = hr_raid4_bd_open,
	.close = hr_raid4_bd_close,
	.sync_cache = hr_raid4_bd_sync_cache,
	.read_blocks = hr_raid4_bd_read_blocks,
	.write_blocks = hr_raid4_bd_write_blocks,
	.get_block_size = hr_raid4_bd_get_block_size,
	.get_num_blocks = hr_raid4_bd_get_num_blocks
};

errno_t hr_raid4_create(hr_volume_t *new_volume)
{
	errno_t rc;

	assert(new_volume->level == HR_LVL_4);

	if (new_volume->extent_no < 3) {
		HR_ERROR("RAID 4 array needs at least 3 devices\n");
		return EINVAL;
	}

	rc = hr_raid4_update_vol_status(new_volume);
	if (rc != EOK)
		return rc;

	bd_srvs_init(&new_volume->hr_bds);
	new_volume->hr_bds.ops = &hr_raid4_bd_ops;
	new_volume->hr_bds.sarg = new_volume;

	rc = hr_register_volume(new_volume);

	return rc;
}

errno_t hr_raid4_init(hr_volume_t *vol)
{
	errno_t rc;
	size_t bsize;
	uint64_t total_blkno;

	assert(vol->level == HR_LVL_4);

	rc = hr_check_devs(vol, &total_blkno, &bsize);
	if (rc != EOK)
		return rc;

	vol->nblocks = total_blkno;
	vol->bsize = bsize;
	vol->data_offset = HR_DATA_OFF;
	vol->data_blkno = vol->nblocks - (vol->data_offset * vol->extent_no) -
	    (vol->nblocks / vol->extent_no);
	vol->strip_size = HR_STRIP_SIZE;

	return EOK;
}

void hr_raid4_status_event(hr_volume_t *vol)
{
	fibril_mutex_lock(&vol->lock);
	(void)hr_raid4_update_vol_status(vol);
	fibril_mutex_unlock(&vol->lock);
}

errno_t hr_raid4_add_hotspare(hr_volume_t *vol, service_id_t hotspare)
{
	HR_DEBUG("hr_raid4_add_hotspare()\n");

	fibril_mutex_lock(&vol->lock);

	if (vol->hotspare_no >= HR_MAX_HOTSPARES) {
		HR_ERROR("hr_raid4_add_hotspare(): cannot add more hotspares "
		    "to \"%s\"\n", vol->devname);
		fibril_mutex_unlock(&vol->lock);
		return ELIMIT;
	}

	vol->hotspares[vol->hotspare_no].svc_id = hotspare;
	hr_update_hotspare_status(vol, vol->hotspare_no, HR_EXT_HOTSPARE);

	vol->hotspare_no++;

	/*
	 * If the volume is degraded, start rebuild right away.
	 */
	if (vol->status == HR_VOL_DEGRADED) {
		HR_DEBUG("hr_raid4_add_hotspare(): volume in DEGRADED state, "
		    "spawning new rebuild fibril\n");
		fid_t fib = fibril_create(hr_raid4_rebuild, vol);
		if (fib == 0)
			return ENOMEM;
		fibril_start(fib);
		fibril_detach(fib);
	}

	fibril_mutex_unlock(&vol->lock);

	return EOK;
}

static errno_t hr_raid4_bd_open(bd_srvs_t *bds, bd_srv_t *bd)
{
	HR_DEBUG("hr_bd_open()\n");
	return EOK;
}

static errno_t hr_raid4_bd_close(bd_srv_t *bd)
{
	HR_DEBUG("hr_bd_close()\n");
	return EOK;
}

static errno_t hr_raid4_bd_sync_cache(bd_srv_t *bd, aoff64_t ba, size_t cnt)
{
	return hr_raid4_bd_op(HR_BD_SYNC, bd, ba, cnt, NULL, NULL, 0);
}

static errno_t hr_raid4_bd_read_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    void *buf, size_t size)
{
	return hr_raid4_bd_op(HR_BD_READ, bd, ba, cnt, buf, NULL, size);
}

static errno_t hr_raid4_bd_write_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    const void *data, size_t size)
{
	return hr_raid4_bd_op(HR_BD_WRITE, bd, ba, cnt, NULL, data, size);
}

static errno_t hr_raid4_bd_get_block_size(bd_srv_t *bd, size_t *rsize)
{
	hr_volume_t *vol = bd->srvs->sarg;

	*rsize = vol->bsize;
	return EOK;
}

static errno_t hr_raid4_bd_get_num_blocks(bd_srv_t *bd, aoff64_t *rnb)
{
	hr_volume_t *vol = bd->srvs->sarg;

	*rnb = vol->data_blkno;
	return EOK;
}

static errno_t hr_raid4_vol_usable(hr_volume_t *vol)
{
	if (vol->status == HR_VOL_ONLINE ||
	    vol->status == HR_VOL_DEGRADED ||
	    vol->status == HR_VOL_REBUILD)
		return EOK;
	return EIO;
}

/*
 * Returns (-1) if all extents are online,
 * else returns index of first bad one.
 */
static ssize_t hr_raid4_get_bad_ext(hr_volume_t *vol)
{
	for (size_t i = 0; i < vol->extent_no; i++)
		if (vol->extents[i].status != HR_EXT_ONLINE)
			return i;
	return -1;
}

static errno_t hr_raid4_update_vol_status(hr_volume_t *vol)
{
	hr_vol_status_t old_state = vol->status;
	size_t bad = 0;
	for (size_t i = 0; i < vol->extent_no; i++)
		if (vol->extents[i].status != HR_EXT_ONLINE)
			bad++;

	switch (bad) {
	case 0:
		if (old_state != HR_VOL_ONLINE)
			hr_update_vol_status(vol, HR_VOL_ONLINE);
		return EOK;
	case 1:
		if (old_state != HR_VOL_DEGRADED &&
		    old_state != HR_VOL_REBUILD) {

			hr_update_vol_status(vol, HR_VOL_DEGRADED);

			if (vol->hotspare_no > 0) {
				fid_t fib = fibril_create(hr_raid4_rebuild,
				    vol);
				if (fib == 0)
					return ENOMEM;
				fibril_start(fib);
				fibril_detach(fib);
			}
		}
		return EOK;
	default:
		if (old_state != HR_VOL_FAULTY)
			hr_update_vol_status(vol, HR_VOL_FAULTY);
		return EIO;
	}
}

static void hr_raid4_handle_extent_error(hr_volume_t *vol, size_t extent,
    errno_t rc)
{
	if (rc == ENOENT)
		hr_update_ext_status(vol, extent, HR_EXT_MISSING);
	else if (rc != EOK)
		hr_update_ext_status(vol, extent, HR_EXT_FAILED);
}

static void xor(void *dst, const void *src, size_t size)
{
	size_t i;
	uint64_t *d = dst;
	const uint64_t *s = src;

	for (i = 0; i < size / sizeof(uint64_t); ++i)
		*d++ ^= *s++;
}

static errno_t hr_raid4_read_degraded(hr_volume_t *vol, uint64_t bad,
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
	bool first = true;
	for (i = 0; i < vol->extent_no; i++) {
		if (i == bad)
			continue;

		if (first) {
			rc = block_read_direct(vol->extents[i].svc_id, block,
			    cnt, xorbuf);
			if (rc != EOK)
				goto end;

			first = false;
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

static errno_t hr_raid4_write(hr_volume_t *vol, uint64_t extent, aoff64_t ba,
    const void *data, size_t cnt)
{
	errno_t rc;
	size_t i;
	void *xorbuf;
	void *buf;
	uint64_t len = vol->bsize * cnt;

	ssize_t bad = hr_raid4_get_bad_ext(vol);
	if (bad < 1) {
		rc = block_write_direct(vol->extents[extent].svc_id, ba, cnt,
		    data);
		if (rc != EOK)
			return rc;
		/*
		 * DEGRADED parity - skip parity write
		 */
		if (bad == 0)
			return EOK;

		rc = hr_raid4_write_parity(vol, extent, ba, data, cnt);
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

	if (extent == (size_t)bad) {
		/*
		 * new parity = read other and xor in new data
		 *
		 * write new parity
		 */
		bool first = true;
		for (i = 1; i < vol->extent_no; i++) {
			if (i == (size_t)bad)
				continue;

			if (first) {
				rc = block_read_direct(vol->extents[i].svc_id,
				    ba, cnt, xorbuf);
				if (rc != EOK)
					goto end;

				first = false;
			} else {
				rc = block_read_direct(vol->extents[i].svc_id,
				    ba, cnt, buf);
				if (rc != EOK)
					goto end;

				xor(xorbuf, buf, len);
			}
		}
		xor(xorbuf, data, len);
		rc = block_write_direct(vol->extents[0].svc_id, ba, cnt,
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
		rc = block_read_direct(vol->extents[0].svc_id, ba, cnt, buf);
		if (rc != EOK)
			goto end;

		xor(xorbuf, buf, len);

		xor(xorbuf, data, len);

		rc = block_write_direct(vol->extents[0].svc_id, ba, cnt,
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

static errno_t hr_raid4_write_parity(hr_volume_t *vol, uint64_t extent,
    uint64_t block, const void *data, size_t cnt)
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

	/*
	 * parity = read and xor all other data extents, xor in new data
	 *
	 * XXX: subtract method
	 */
	bool first = true;
	for (i = 1; i < vol->extent_no; i++) {
		if (first) {
			if (i == extent) {
				memcpy(xorbuf, data, len);
			} else {
				rc = block_read_direct(vol->extents[i].svc_id,
				    block, cnt, xorbuf);
				if (rc != EOK)
					goto end;
			}

			first = false;
		} else {
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
	}

	rc = block_write_direct(vol->extents[0].svc_id, block, cnt, xorbuf);
end:
	free(xorbuf);
	free(buf);
	return rc;
}

static errno_t hr_raid4_bd_op(hr_bd_op_type_t type, bd_srv_t *bd, aoff64_t ba,
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
		rc = hr_raid4_update_vol_status(vol);
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
	uint64_t extent = (stripe % (vol->extent_no - 1)) + 1;
	uint64_t ext_stripe = stripe / (vol->extent_no - 1); /* stripe level */
	uint64_t strip_off = ba % strip_size; /* strip offset */

	fibril_mutex_lock(&vol->lock);

	rc = hr_raid4_vol_usable(vol);
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
			ssize_t bad = hr_raid4_get_bad_ext(vol);
			if (bad > 0 && extent == (size_t)bad) {
				rc = hr_raid4_read_degraded(vol, bad,
				    phys_block, data_read, cnt);
			} else {
				rc = block_read_direct(vol->extents[extent].svc_id,
				    phys_block, cnt, data_read);
			}
			data_read += len;
			break;
		case HR_BD_WRITE:
		retry_write:
			rc = hr_raid4_write(vol, extent, phys_block,
			    data_write, cnt);
			data_write += len;
			break;
		default:
			rc = EINVAL;
			goto error;
		}

		if (rc == ENOMEM)
			goto error;

		hr_raid4_handle_extent_error(vol, extent, rc);

		if (rc != EOK) {
			rc = hr_raid4_update_vol_status(vol);
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
		extent++;
		if (extent >= vol->extent_no) {
			ext_stripe++;
			extent = 1;
		}
	}

error:
	(void)hr_raid4_update_vol_status(vol);
	fibril_mutex_unlock(&vol->lock);
	return rc;
}

static errno_t hr_raid4_rebuild(void *arg)
{
	HR_DEBUG("hr_raid4_rebuild()\n");

	hr_volume_t *vol = arg;
	errno_t rc = EOK;
	void *buf = NULL, *xorbuf = NULL;

	fibril_mutex_lock(&vol->lock);

	if (vol->hotspare_no == 0) {
		HR_WARN("hr_raid4_rebuild(): no free hotspares on \"%s\", "
		    "aborting rebuild\n", vol->devname);
		/* retval isn't checked for now */
		goto end;
	}

	size_t bad = vol->extent_no;
	for (size_t i = 0; i < vol->extent_no; i++) {
		if (vol->extents[i].status == HR_EXT_FAILED) {
			bad = i;
			break;
		}
	}

	if (bad == vol->extent_no) {
		HR_WARN("hr_raid4_rebuild(): no bad extent on \"%s\", "
		    "aborting rebuild\n", vol->devname);
		/* retval isn't checked for now */
		goto end;
	}

	size_t hotspare_idx = vol->hotspare_no - 1;

	hr_ext_status_t hs_state = vol->hotspares[hotspare_idx].status;
	if (hs_state != HR_EXT_HOTSPARE) {
		HR_ERROR("hr_raid4_rebuild(): invalid hotspare state \"%s\", "
		    "aborting rebuild\n", hr_get_ext_status_msg(hs_state));
		rc = EINVAL;
		goto end;
	}

	HR_DEBUG("hr_raid4_rebuild(): swapping in hotspare\n");

	block_fini(vol->extents[bad].svc_id);

	vol->extents[bad].svc_id = vol->hotspares[hotspare_idx].svc_id;
	hr_update_ext_status(vol, bad, HR_EXT_HOTSPARE);

	vol->hotspares[hotspare_idx].svc_id = 0;
	hr_update_hotspare_status(vol, hotspare_idx, HR_EXT_MISSING);

	vol->hotspare_no--;

	hr_extent_t *rebuild_ext = &vol->extents[bad];

	rc = block_init(rebuild_ext->svc_id);
	if (rc != EOK) {
		HR_ERROR("hr_raid4_rebuild(): initing (%lu) failed, "
		    "aborting rebuild\n", rebuild_ext->svc_id);
		goto end;
	}

	HR_DEBUG("hr_raid4_rebuild(): starting rebuild on (%lu)\n",
	    rebuild_ext->svc_id);

	hr_update_ext_status(vol, bad, HR_EXT_REBUILD);
	hr_update_vol_status(vol, HR_VOL_REBUILD);

	uint64_t max_blks = DATA_XFER_LIMIT / vol->bsize;
	uint64_t left = vol->data_blkno / (vol->extent_no - 1);
	buf = malloc(max_blks * vol->bsize);
	xorbuf = malloc(max_blks * vol->bsize);

	uint64_t ba = 0, cnt;
	hr_add_ba_offset(vol, &ba);

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
			rc = block_read_direct(vol->extents[i].svc_id, ba, cnt,
			    buf);
			if (rc != EOK) {
				hr_raid4_handle_extent_error(vol, i, rc);
				HR_ERROR("rebuild on \"%s\" (%lu), failed due "
				    "to a failed ONLINE extent, number %lu\n",
				    vol->devname, vol->svc_id, i);
				goto end;
			}

			if (first)
				memcpy(xorbuf, buf, cnt * vol->bsize);
			else
				xor(xorbuf, buf, cnt * vol->bsize);

			first = false;
		}

		rc = block_write_direct(rebuild_ext->svc_id, ba, cnt, xorbuf);
		if (rc != EOK) {
			hr_raid4_handle_extent_error(vol, bad, rc);
			HR_ERROR("rebuild on \"%s\" (%lu), failed due to "
			    "the rebuilt extent number %lu failing\n",
			    vol->devname, vol->svc_id, bad);
			goto end;
		}

		ba += cnt;
		left -= cnt;

		/*
		 * Let other IO requests be served
		 * during rebuild.
		 */
		fibril_mutex_unlock(&vol->lock);
		fibril_mutex_lock(&vol->lock);
	}

	HR_DEBUG("hr_raid4_rebuild(): rebuild finished on \"%s\" (%lu), "
	    "extent number %lu\n", vol->devname, vol->svc_id, hotspare_idx);

	hr_update_ext_status(vol, bad, HR_EXT_ONLINE);
	/*
	 * For now write metadata at the end, because
	 * we don't sync metada accross extents yet.
	 */
	hr_write_meta_to_ext(vol, bad);
end:
	(void)hr_raid4_update_vol_status(vol);

	fibril_mutex_unlock(&vol->lock);

	if (buf != NULL)
		free(buf);

	if (xorbuf != NULL)
		free(xorbuf);

	return rc;
}

/** @}
 */
